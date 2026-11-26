// Hallway 100 - original first-person survival horror for 3DS. Platform independent core.
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MW 9
#define MH 18
#define LAST_ROOM 100
#define SAVEMAGIC 0x48353030
#define ENDLESS (diff == 3)
#define NACH 14
#define NMON 10
#define NENT 15
#define PI 3.14159265f

Save sv; int g_saveDirty = 0;
enum { TITLE, BESTIARY, ACHS, CARD, PLAY, OPENING, EVENT, DEAD, WIN };
enum { EV_LURKER, EV_GLARE, EV_SNARE, EV_WHISPER, EV_SKITTER, EV_MIMIC, EV_FLOOD, EV_DEVOUR, EV_PENDULUM, EV_GALE, EV_BOSS };

static int state = TITLE, afterCard = PLAY;
static int ev, room = 1, t, fr, dodged, gold, lives = 3, maxLives = 3, diff = 1, sel;
static int bandages, skeletons, shields, battery = 100;
static bool lantern, hourglass, luckyCoin, locked, hasKey, dark, shop, chest, chestOpen, fountain, fountainUsed, hiding, pendingEvent;
static int lockType, drawer[3], mash, taps, warnLen = 150, hideCool;
static bool opened[3], pat[3], cur[3];
static uint32_t snareKey, galeKey = K_LEFT, bseq[8];
static int blen = 4, bpos, btime = 100, btimeMax = 100;
static char msg[64]; static int msgT, flash, achT, stKind, stx, sty;
static const char *achText = "";
static float px_ = 4.5f, py_ = 15.5f, pa = -PI / 2, bobPh, doorOpen, fade;
static bool moving, usedInput, light, eyesClosed;
static uint8_t map[MH][MW];
static uint32_t rs = 1;
static int rnd(int n) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (int)(rs % (uint32_t)n); }

static const char *FLOORS[5] = { "THE MANOR", "THE LIBRARY", "THE GREENHOUSE", "THE CELLAR", "THE VOID" };
static const char *FLORE[5] = { "An old house that forgot its owners.", "Books whisper if you stand too still.", "Plants grow where no sun has been.", "Damp stone. Dripping. Something below.", "The hallway ends. The dark does not." };
static const char *DIFFN[4] = { "EASY - 5 LIVES", "NORMAL - 3 LIVES", "HARD - 1 LIFE", "ENDLESS" };
static const char *EVN[NMON] = { "THE LURKER", "THE GLARE", "THE SNARE", "THE WHISPERER", "THE SKITTER", "THE MIMIC", "THE FLOOD", "THE DEVOURER", "THE PENDULUM", "THE GALE" };
static const char *EVT[NMON] = { "It rushes down the hall. HIDE in a closet!", "A giant eye stares. HOLD L: close eyes.", "A trap button flashes. Press it in time.", "It listens. Do NOT move or press anything.", "It scuttles at you. MASH A 12 times!", "The closet is a lie. Do NOT hide!", "Water rises. TAP the touch screen 10x.", "Hide in a closet AND hold L.", "Press A when the marker is in the green.", "Wind howls. HOLD the arrow shown." };
static const char *EVH[NMON] = { "RUN TO A CLOSET! (A)", "CLOSE YOUR EYES! HOLD L", "WATCH THE BUTTON PROMPT!", "FREEZE! DO NOTHING!", "GET READY TO MASH A!", "DON'T HIDE! STAY OUT!", "TAP THE TOUCH SCREEN!", "HIDE IN CLOSET + HOLD L!", "TIME YOUR PRESS OF A!", "BRACE AGAINST THE WIND!" };
static const char *BOSSN[5] = { "THE HOLLOW BUTLER", "THE ARCHIVIST", "THE GARDENER", "THE DROWNED KING", "THE VOID WARDEN" };
static const char *ACHN[NACH] = { "FIRST STEPS", "SURVIVOR", "BUTLER DOWN", "SHOPPER", "SKELETON CREW", "GREEN THUMB", "RICH", "GHOST HUNTER", "ESCAPE ARTIST", "HARD MODE HERO", "NEVER ENDING", "REFRESHED", "PUZZLE MASTER", "TREASURE HUNTER" };
static const char *ACHD[NACH] = { "Reach room 2", "Dodge 10 monsters in one run", "Beat the first boss", "Buy an item", "Use a skeleton key", "Reach the Greenhouse", "Hold 100 gold", "See every monster", "Beat the game", "Beat the game on HARD", "Reach room 120 in Endless", "Use a fountain", "Solve a lever lock", "Open a treasure chest" };
static const char *NOTES[30] = { "Wallpaper peels in the shape of a door.", "Scratched: 'Don't trust the quiet.'", "A child's drawing of this hall, crooked.", "The air smells like wet pennies.", "A clock ticks, but there are no clocks.", "A note: 'The closet is safe. Mostly.'", "Your footsteps echo a half-second late.", "A cold draft slips under the next door.", "Dust shows no footprints but yours.", "A music box plays one note, then stops.", "The lights hum a tune you almost know.", "Someone has rearranged the furniture.", "Sign: 'Please keep the doors shut.'", "You swear the painting blinked.", "Wet footprints lead into the wall.", "The hall feels longer than before.", "A voice in the wall counts down.", "Candle wax drips from the ceiling.", "A coin in the carpet. It is warm.", "The door numbers don't match the rooms.", "A mirror shows the room, but not you.", "A faucet drips in perfect rhythm.", "Sticky note: 'Count your lives.'", "The shadows lean toward the door.", "A fly circles the lamp. Only thing alive.", "The floor creaks: three, then two.", "A tiny door in the baseboard is bricked.", "You taste static at the back of your throat.", "Whoever lived here left in a hurry.", "The next door opens a crack. Then shuts." };
static const int FLC[5][3] = { {70,48,36}, {56,40,30}, {60,72,52}, {60,62,68}, {30,20,48} };
static const int CEC[5][3] = { {84,74,66}, {46,36,30}, {96,124,124}, {40,42,48}, {16,10,28} };
static const uint32_t BK[8] = { K_A, K_B, K_X, K_Y, K_UP, K_DOWN, K_LEFT, K_RIGHT };
static const uint32_t SK[7] = { K_X, K_Y, K_R, K_UP, K_DOWN, K_LEFT, K_RIGHT };

// ---------------------------------------------------------------- framebuffer + font
static uint8_t *FB; static int SW = 400;
static inline void pxl(int x, int y, int r, int g, int b) { if ((unsigned)x >= (unsigned)SW || (unsigned)y >= 240u) return; uint8_t *p = FB + (x * 240 + 239 - y) * 3; p[0] = b; p[1] = g; p[2] = r; }
static inline void pxa(int x, int y, int r, int g, int b, int a) { if ((unsigned)x >= (unsigned)SW || (unsigned)y >= 240u) return; uint8_t *p = FB + (x * 240 + 239 - y) * 3; p[0] = (p[0] * (255 - a) + b * a) / 255; p[1] = (p[1] * (255 - a) + g * a) / 255; p[2] = (p[2] * (255 - a) + r * a) / 255; }
static void box(int x, int y, int w, int h, int r, int g, int b, int a) { for (int j = y < 0 ? 0 : y; j < y + h && j < 240; j++) for (int i = x < 0 ? 0 : x; i < x + w && i < SW; i++) { if (a >= 255) pxl(i, j, r, g, b); else pxa(i, j, r, g, b, a); } }
#define G(a,b,c,d,e) (uint16_t)(((a)<<12)|((b)<<9)|((c)<<6)|((d)<<3)|(e))
static const char CS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!.:/-%+?<>,'()=*_#";
static const uint16_t GL[] = { G(0,0,0,0,0), G(2,5,7,5,5), G(6,5,6,5,6), G(3,4,4,4,3), G(6,5,5,5,6), G(7,4,6,4,7), G(7,4,6,4,4), G(3,4,5,5,3), G(5,5,7,5,5), G(7,2,2,2,7), G(1,1,1,5,2), G(5,5,6,5,5), G(4,4,4,4,7), G(5,7,7,5,5), G(6,5,5,5,5), G(2,5,5,5,2), G(6,5,6,4,4), G(2,5,5,6,3), G(6,5,6,5,5), G(3,4,2,1,6), G(7,2,2,2,2), G(5,5,5,5,7), G(5,5,5,5,2), G(5,5,7,7,5), G(5,5,2,5,5), G(5,5,2,2,2), G(7,1,2,4,7),
    G(7,5,5,5,7), G(2,6,2,2,7), G(6,1,2,4,7), G(6,1,2,1,6), G(5,5,7,1,1), G(7,4,6,1,6), G(3,4,7,5,7), G(7,1,2,2,2), G(7,5,7,5,7), G(7,5,7,1,6),
    G(2,2,2,0,2), G(0,0,0,0,2), G(0,2,0,2,0), G(1,1,2,4,4), G(0,0,7,0,0), G(5,1,2,4,5), G(0,2,7,2,0), G(6,1,2,0,2), G(1,2,4,2,1), G(4,2,1,2,4), G(0,0,0,2,4), G(2,2,0,0,0), G(1,2,2,2,1), G(4,2,2,2,4), G(0,7,0,7,0), G(5,2,7,2,5), G(0,0,0,0,7), G(5,7,5,7,5) };
static int tw(const char *s, int sc) { return (int)strlen(s) * 4 * sc; }
static void txt(const char *s, int x, int y, int sc, int r, int g, int b) {
    for (; *s; s++, x += 4 * sc) {
        char c = *s; if (c >= 'a' && c <= 'z') c -= 32;
        const char *q = c ? strchr(CS, c) : 0; uint16_t gl = GL[q ? q - CS : 0];
        for (int row = 0; row < 5; row++) for (int col = 0; col < 3; col++) if ((gl >> (12 - 3 * row)) & (4 >> col)) box(x + col * sc, y + row * sc, sc, sc, r, g, b, 255);
    }
}
static void txts(const char *s, int x, int y, int sc, int r, int g, int b) { txt(s, x + sc, y + sc, sc, 0, 0, 0); txt(s, x, y, sc, r, g, b); }
static void txtc(const char *s, int cx, int y, int sc, int r, int g, int b) { txts(s, cx - tw(s, sc) / 2, y, sc, r, g, b); }

// ---------------------------------------------------------------- textures (procedural, 64x64)
static uint8_t TEX[9][64 * 64 * 3];
static uint32_t hsh(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }
static int nz(int x, int y, int s) { return (int)(hsh((uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)s * 83492791u) & 255); }
static int c8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }
static void tset(int t_, int u, int v, int r, int g, int b) { uint8_t *p = &TEX[t_][(v * 64 + u) * 3]; p[0] = c8(r * 13 / 10); p[1] = c8(g * 13 / 10); p[2] = c8(b * 13 / 10); }
static void gen_textures(void) {
    for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int n = nz(u, v, 1) % 13 - 6, r, g, b;
        r = 112; g = 88; b = 66; if ((u / 8) & 1) { r += 9; g += 7; b += 6; }                      // 0 manor
        if (v < 4) { r -= 30; g -= 25; b -= 20; }
        else if (v > 42) { r = 86; g = 58; b = 40; if (v == 43 || v == 44 || v == 58 || v == 59 || u % 16 == 0) { r -= 24; g -= 18; b -= 12; } }
        else if (v >= 14 && v <= 34 && u >= 24 && u <= 40) { if (u < 27 || u > 37 || v < 17 || v > 31) { r = 150; g = 110; b = 50; } else { r = 60 + u * 2; g = 80 + v; b = 120; } }
        tset(0, u, v, r + n, g + n, b + n);
        { int row = v / 21, vy = v % 21; uint32_t s = (uint32_t)(u / 4 + row * 13);                  // 1 library
          if (vy >= 19) { r = 70; g = 45; b = 28; } else { r = 60 + (int)(hsh(s) & 127); g = 40 + (int)(hsh(s * 3) & 95); b = 40 + (int)(hsh(s * 7) & 95); if (vy < 2) { r -= 30; g -= 30; b -= 30; } if (u % 4 == 3) { r -= 25; g -= 25; b -= 25; } }
          tset(1, u, v, r, g, b); }
        r = 60; g = 100; b = 70; if (u % 16 < 2 || v % 16 < 2) { r = 40; g = 60; b = 50; } if (nz(u, v, 5) > 235) { r = 90; g = 170; b = 80; } if (((u + v) / 9) % 4 == 0) { r += 10; g += 14; b += 10; }   // 2 greenhouse
        tset(2, u, v, r + n / 2, g + n / 2, b + n / 2);
        { int row = v / 8, off = (row & 1) * 8, bu = (u + off) % 16; int var = (int)(hsh((uint32_t)((u + off) / 16 + row * 7)) % 40) - 20;   // 3 cellar bricks
          if (v % 8 == 0 || bu == 0) { r = 50; g = 52; b = 58; } else { r = 95 + var + n; g = 100 + var + n; b = 110 + var + n; } tset(3, u, v, r, g, b); }
        { float s = sinf(u * .2f + v * .13f) * 8; r = 36 + (int)s; g = 22; b = 64 + (int)(s * 1.5f); if (nz(u, v, 9) > 247) { r = 255; g = 240; b = 255; } tset(4, u, v, r, g, b); }  // 4 void
        { r = 130; g = 84; b = 42;                                                                    // 5 door
          if (u < 4 || u > 59 || v < 3) { r = 50; g = 32; b = 18; }
          else { int pu = (u - 8) % 28, pv = (v - 8) % 28; if (u >= 8 && u <= 56 && v >= 8 && v <= 58 && pu < 21 && pv < 23 && (u < 29 || u >= 36) && (v < 31 || v >= 36)) { r -= 14; g -= 10; b -= 6; if (pu < 2 || pv < 2) { r -= 28; g -= 22; b -= 14; } if (pu > 18 || pv > 20) { r += 20; g += 14; b += 8; } } }
          if ((u - 50) * (u - 50) + (v - 34) * (v - 34) < 12) { r = 230; g = 200; b = 80; } tset(5, u, v, r + n / 2, g + n / 2, b + n / 2); }
        { r = 74; g = 48; b = 34; if (u == 31 || u == 32) { r = 30; g = 20; b = 14; } if (v >= 8 && v <= 20 && v % 3 == 0 && ((u >= 8 && u <= 26) || (u >= 38 && u <= 56))) { r = 20; g = 12; b = 8; }   // 6 closet
          if ((u == 28 || u == 36) && v >= 32 && v <= 38) { r = 210; g = 180; b = 70; } tset(6, u, v, r + n / 2, g + n / 2, b + n / 2); }
        { r = 104; g = 68; b = 42; int dv = v - 6; if (dv >= 0 && dv < 57 && u >= 6 && u <= 57) { int d = dv % 19; if (d == 0 || d == 16 || u == 6 || u == 57) { r -= 36; g -= 26; b -= 16; } if (d == 8 && u >= 26 && u <= 37) { r = 215; g = 180; b = 70; } } tset(7, u, v, r + n, g + n, b + n); }   // 7 cabinet
        { r = 112; g = 82; b = 46; if (v % 16 == 0) { r -= 40; g -= 30; b -= 18; } if (abs(u - v) < 3 || abs(u + v - 63) < 3) { r = 80; g = 56; b = 30; } tset(8, u, v, r + n, g + n, b + n); }   // 8 crate
    }
}

// ---------------------------------------------------------------- world
static int floorOf(int r) { int f = (r - 1) / 20; return f < 0 ? 0 : (f > 4 ? 4 : f); }
static int bossNo(void) { int n = room / 20 - 1; return n < 0 ? 0 : n; }
static int bonus(void) { return (lantern ? 30 : 0) + (hourglass ? 30 : 0) + (diff == 0 ? 25 : (diff == 2 ? -15 : 0)); }
static int pendPos(int tt) { int p = (tt * 5) % 400; return p > 200 ? 400 - p : p; }
static void setMsg(const char *s) { strncpy(msg, s, sizeof msg - 1); msg[sizeof msg - 1] = 0; msgT = 170; }
static void ach(int i) { if ((sv.ach >> i) & 1) return; sv.ach |= 1 << i; achText = ACHN[i]; achT = 220; g_saveDirty = 1; }
static const char *keyName(uint32_t k) {
    switch (k) { case K_A: return "A"; case K_B: return "B"; case K_X: return "X"; case K_Y: return "Y"; case K_R: return "R"; case K_L: return "L"; case K_UP: return "UP"; case K_DOWN: return "DOWN"; case K_LEFT: return "LEFT"; case K_RIGHT: return "RIGHT"; default: return "?"; }
}
static int nclosets, closets[4][2];
static void genRoom(void) {
    memset(map, 0, sizeof map);
    for (int x = 0; x < MW; x++) { map[0][x] = 1; map[MH - 1][x] = 1; }
    for (int y = 0; y < MH; y++) { map[y][0] = 1; map[y][MW - 1] = 1; }
    map[0][4] = 5; nclosets = 0;
    for (int i = 0; i < 3; i++) { int y = 9 + rnd(7), x = rnd(2) ? 0 : MW - 1; if (map[y][x] == 3 || map[y - 1][x] == 3 || map[y + 1][x] == 3) continue; map[y][x] = 3; if (nclosets < 4) { closets[nclosets][0] = x; closets[nclosets][1] = y; nclosets++; } }
    bool crow[MH]; memset(crow, 0, sizeof crow); for (int i = 0; i < nclosets; i++) crow[closets[i][1]] = true;
    int nc = 3 + rnd(4);
    for (int i = 0; i < nc; i++) { int x = 1 + rnd(7), y = 3 + rnd(10); if (x == 4 || crow[y]) continue; map[y][x] = 2; }
    stKind = shop ? 2 : (locked && !hasKey ? 1 : (chest ? 3 : (fountain ? 4 : 0)));
    stx = rnd(2) ? 0 : MW - 1; sty = 4 + rnd(5); for (int x = 1; x < MW - 1; x++) if (map[sty][x] == 2) map[sty][x] = 0;
    if (stKind) map[sty][stx] = 4; else { int sx = rnd(2) ? 0 : MW - 1; map[4 + rnd(5)][sx] = 4; stx = -9; sty = -9; }
    px_ = 4.5f; py_ = 15.5f; pa = -PI / 2; hiding = false;
}
static void startBoss(void) { ev = EV_BOSS; t = 0; state = EVENT; int bn = bossNo(); if (bn > 4) bn = 4; blen = 4 + bn; for (int i = 0; i < blen; i++) bseq[i] = BK[rnd(8)]; bpos = 0; btimeMax = 110 - bn * 10 + bonus() / 2; if (btimeMax < 40) btimeMax = 40; btime = btimeMax; }
static void beginEvent(int e) {
    ev = e; warnLen = 220 - room; if (warnLen < 130) warnLen = 130; warnLen += bonus(); if (warnLen < 90) warnLen = 90;
    if (ev == EV_SNARE) snareKey = SK[rnd(7)];
    if (ev == EV_GALE) galeKey = rnd(2) ? K_LEFT : K_RIGHT;
    sv.seen |= 1 << ev; g_saveDirty = 1; mash = taps = 0; t = 0; state = EVENT;
    if ((sv.seen & ((1 << NMON) - 1)) == ((1 << NMON) - 1)) ach(7);
}
static void startEvent(void) {
    int f = floorOf(room), pool[16], n = 0;
    pool[n++] = EV_LURKER; pool[n++] = EV_LURKER;
    if (room >= 6) pool[n++] = EV_SNARE;
    if (room >= 10) pool[n++] = EV_GLARE;
    if (f >= 1) { pool[n++] = EV_WHISPER; pool[n++] = EV_SKITTER; }
    if (f >= 1 && room >= 25) pool[n++] = EV_PENDULUM;
    if (f >= 2) { pool[n++] = EV_MIMIC; pool[n++] = EV_GALE; }
    if (f >= 3) pool[n++] = EV_FLOOD;
    if (f >= 4) pool[n++] = EV_DEVOUR;
    beginEvent(pool[rnd(n)]);
}
static void enterRoom(void) {
    bool boss = (room % 20 == 0);
    t = 0; hasKey = false; chestOpen = false; fountainUsed = false; mash = taps = 0; hiding = false; hideCool = 0; pendingEvent = false; doorOpen = 0;
    shop = (room % 10 == 5);
    locked = !shop && ((room % 10 == 0 && !boss) || (room >= 5 && rnd(100) < 10));
    chest = !shop && !locked && !boss && rnd(100) < 15;
    fountain = !shop && !locked && !chest && !boss && room >= 8 && rnd(100) < 6;
    dark = floorOf(room) >= 1 && !shop && !boss && rnd(100) < 25;
    lockType = (floorOf(room) >= 1 && rnd(2)) ? 1 : 0;
    int k = rnd(3);
    for (int i = 0; i < 3; i++) { opened[i] = false; drawer[i] = (i == k) ? 1 : (rnd(2) ? 2 : 0); pat[i] = rnd(2); cur[i] = false; }
    if (!pat[0] && !pat[1] && !pat[2]) pat[rnd(3)] = true;
    genRoom(); fade = 1.0f; state = PLAY;
    if (rnd(100) < 25) setMsg(NOTES[rnd(30)]);
    if (boss) startBoss(); else if (!shop && room >= 3 && rnd(100) < 40) pendingEvent = true;
    if ((room - 1) % 20 == 0) { afterCard = state; state = CARD; t = 0; }
    if (room >= 2) ach(0);
    if (room >= 41) ach(5);
    if (ENDLESS && room >= 120) ach(10);
}
static void newGame(void) {
    maxLives = diff == 0 ? 5 : (diff == 2 ? 1 : 3); lives = maxLives;
    room = 1; dodged = 0; gold = 0; bandages = skeletons = shields = 0; lantern = hourglass = luckyCoin = false;
    battery = 100; msgT = 0; flash = 0; sv.runs++; g_saveDirty = 1; enterRoom();
}
static void checkBest(void) { if ((ENDLESS && room > sv.bestEndless) || (!ENDLESS && room <= LAST_ROOM && room > sv.best)) { if (ENDLESS) sv.bestEndless = room; else sv.best = room; g_saveDirty = 1; } }
static bool takeHit(void) {
    flash = 36;
    if (shields > 0) { shields--; setMsg("Your shield absorbed it!"); return false; }
    lives--; setMsg("Ouch! You lost a life.");
    if (lives <= 0) { state = DEAD; sv.deaths++; checkBest(); g_saveDirty = 1; return true; }
    return false;
}
static void resolve(bool ok) {
    if (ok) { dodged++; gold += luckyCoin ? 5 : 2; setMsg(luckyCoin ? "You survived it! +5 gold" : "You survived it! +2 gold"); state = PLAY; if (dodged >= 10) ach(1); }
    else if (!takeHit()) state = PLAY;
}
static void bossWin(void) {
    sv.bosses |= 1 << (bossNo() % 5); gold += 10; dodged++; ach(2); g_saveDirty = 1;
    if (!ENDLESS && room >= LAST_ROOM) { state = WIN; sv.wins++; ach(8); if (diff == 2) ach(9); checkBest(); }
    else { setMsg("Boss defeated! +10 gold"); state = PLAY; }
}
static void bossStep(uint32_t down) {
    if (t < 60) return;
    uint32_t pr = down & (K_A | K_B | K_X | K_Y | K_UP | K_DOWN | K_LEFT | K_RIGHT); bool miss = false;
    if (pr) { if (pr & bseq[bpos]) { bpos++; btime = btimeMax; if (bpos >= blen) { bossWin(); return; } } else miss = true; }
    else if (--btime <= 0) miss = true;
    if (miss && !takeHit()) { for (int i = 0; i < blen; i++) bseq[i] = BK[rnd(8)]; bpos = 0; btime = btimeMax; }
}
static bool solidAt(float x, float y) { int ix = (int)x, iy = (int)y; if (x < 0 || y < 0 || ix >= MW || iy >= MH) return true; return map[iy][ix] != 0; }
static bool blocked(float x, float y) { const float r = 0.22f; return solidAt(x - r, y - r) || solidAt(x + r, y - r) || solidAt(x - r, y + r) || solidAt(x + r, y + r); }
static int probe(float *dist, int *tx, int *ty) {
    float dx = cosf(pa), dy = sinf(pa), d = 0.05f;
    for (; d < 2.0f; d += 0.05f) { int ix = (int)(px_ + dx * d), iy = (int)(py_ + dy * d); if (ix < 0 || iy < 0 || ix >= MW || iy >= MH) break; if (map[iy][ix]) { *dist = d; *tx = ix; *ty = iy; return map[iy][ix]; } }
    return 0;
}
static const int cost[6] = { 10, 15, 20, 40, 30, 25 };
static void buy(int i) {
    if (gold < cost[i]) { setMsg("Not enough gold."); return; }
    if (i == 0) bandages++; else if (i == 1) skeletons++; else if (i == 2) shields++;
    else if (i == 3) { if (lantern) { setMsg("You already own that."); return; } lantern = true; }
    else if (i == 4) { if (hourglass) { setMsg("You already own that."); return; } hourglass = true; }
    else { if (luckyCoin) { setMsg("You already own that."); return; } luckyCoin = true; }
    gold -= cost[i]; setMsg("Purchased!"); ach(3);
}

// ---------------------------------------------------------------- update
static void doMove(const Input *in) {
    float f = 0, tr = 0, cx = in->cx / 156.f, cy = in->cy / 156.f;
    if (in->held & K_UP) f += 1; if (in->held & K_DOWN) f -= 1; if (in->held & K_LEFT) tr -= 1; if (in->held & K_RIGHT) tr += 1;
    if (fabsf(cx) < .15f) cx = 0; if (fabsf(cy) < .15f) cy = 0;
    f += cy; tr += cx; if (f > 1) f = 1; if (f < -1) f = -1; if (tr > 1) tr = 1; if (tr < -1) tr = -1;
    pa += tr * 0.05f; float sp = 0.07f * f, nx = px_ + cosf(pa) * sp, ny = py_ + sinf(pa) * sp;
    if (!blocked(nx, py_)) px_ = nx; if (!blocked(px_, ny)) py_ = ny;
    moving = fabsf(f) > .1f; usedInput = moving || fabsf(tr) > .1f; if (moving) bobPh += 0.2f;
}
void game_update(const Input *in) {
    fr++; uint32_t down = in->down, held = in->held;
    bool inGame = (state == PLAY || state == EVENT);
    if (msgT > 0) msgT--; if (achT > 0) achT--; if (hideCool > 0) hideCool--; if (fade > 0) fade -= 0.03f; if (flash > 0) flash--;
    if (inGame && gold >= 100) ach(6);
    light = dark && inGame && (held & K_SELECT) && battery > 0;
    if (light) { if (fr % 2 == 0) battery--; } else if (battery < 100 && fr % 6 == 0) battery++;
    eyesClosed = state == EVENT && (ev == EV_GLARE || ev == EV_DEVOUR) && (held & K_L);
    // station (touch UI) in 3D space
    bool nearSt = inGame && stKind > 0 && !hiding && hypotf(px_ - (stx + .5f), py_ - (sty + .5f)) < 2.3f;
    if (nearSt && (down & K_TOUCH) && !(state == EVENT && (ev == EV_FLOOD || ev == EV_BOSS))) {
        int tx = in->tx, ty = in->ty;
        if (stKind == 1) for (int i = 0; i < 3; i++) { int x = 15 + i * 100; if (tx >= x && tx < x + 90 && ty >= 120 && ty < 190) {
            if (lockType == 0) { if (!opened[i]) { opened[i] = true; if (drawer[i] == 1) { hasKey = true; setMsg("Found the key! Open the door."); stKind = 0; } else if (drawer[i] == 2) { gold += 1 + rnd(5); setMsg("Found some gold."); } else setMsg("Empty."); } }
            else { cur[i] = !cur[i]; if (cur[0] == pat[0] && cur[1] == pat[1] && cur[2] == pat[2]) { hasKey = true; stKind = 0; setMsg("Click! The lock opens."); ach(12); } } } }
        else if (stKind == 3 && !chestOpen && tx >= 110 && tx < 210 && ty >= 120 && ty < 180) { chestOpen = true; gold += 3 + rnd(10); setMsg("The chest holds gold!"); ach(13); }
        else if (stKind == 4 && !fountainUsed && tx >= 110 && tx < 210 && ty >= 120 && ty < 180) { fountainUsed = true; ach(11); if (lives < maxLives) { lives++; setMsg("The water restores you. +1 life"); } else { gold += 3; setMsg("Coins in the water. +3 gold"); } }
        else if (stKind == 2) for (int i = 0; i < 6; i++) { int x = 15 + (i % 2) * 160, y = 78 + (i / 2) * 40; if (tx >= x && tx < x + 145 && ty >= y && ty < y + 36) buy(i); }
    }
    switch (state) {
        case TITLE:
            pa = -PI / 2 + 0.35f * sinf(fr * 0.012f); bobPh += 0.04f;
            if (down & K_LEFT) diff = diff > 0 ? diff - 1 : 0; if (down & K_RIGHT) diff = diff < 3 ? diff + 1 : 3;
            if (down & K_X) { state = BESTIARY; sel = 0; } if (down & K_Y) state = ACHS; if (down & K_A) newGame(); break;
        case BESTIARY: if (down & K_LEFT) sel = sel > 0 ? sel - 1 : NENT - 1; if (down & K_RIGHT) sel = sel < NENT - 1 ? sel + 1 : 0; if (down & K_B) state = TITLE; break;
        case ACHS: if (down & K_B) state = TITLE; break;
        case CARD: if (++t >= 150 || (down & K_A)) { state = afterCard; t = 0; } break;
        case OPENING: doorOpen = ++t / 30.f; if (t >= 30) { room++; if (!ENDLESS && room > LAST_ROOM) { state = WIN; checkBest(); } else { enterRoom(); checkBest(); } } break;
        case PLAY: case EVENT: {
            if (!hiding) doMove(in); else { moving = false; usedInput = false; }
            if (state == PLAY && (down & K_Y) && bandages > 0 && lives < maxLives) { bandages--; lives++; setMsg("Bandage used. +1 life"); }
            bool hideOK = state == PLAY || ev == EV_LURKER || ev == EV_MIMIC || ev == EV_DEVOUR;
            if ((down & K_A) && hideCool == 0) {
                float d; int tx, ty; int tile = probe(&d, &tx, &ty);
                if (hiding) { hiding = false; hideCool = 15; }
                else if (tile == 3 && d < 1.6f && hideOK) { hiding = true; hideCool = 15; }
                else if (state == PLAY && tile == 5 && d < 1.7f) {
                    if (locked && !hasKey && skeletons > 0) { skeletons--; hasKey = true; stKind = 0; setMsg("Skeleton key used!"); ach(4); }
                    if (locked && !hasKey) setMsg(lockType == 0 ? "Locked! Search the cabinet." : "Locked! Match the lights.");
                    else { state = OPENING; t = 0; hideCool = 15; }
                }
            }
            if (state == PLAY && pendingEvent && py_ < 12.5f) { pendingEvent = false; startEvent(); }
            if (state == EVENT) {
                int b = bonus(); t++;
                switch (ev) {
                    case EV_LURKER: if (t == warnLen + 90) resolve(hiding); break;
                    case EV_GLARE: if (t == 210 + b) resolve(eyesClosed); break;
                    case EV_SNARE: { uint32_t pr = down & (K_X | K_Y | K_R | K_UP | K_DOWN | K_LEFT | K_RIGHT); if (t >= 60 && pr) resolve((pr & snareKey) != 0); else if (t >= 150 + b) resolve(false); break; }
                    case EV_WHISPER: { uint32_t any = down & (K_A | K_B | K_X | K_Y | K_L | K_R | K_UP | K_DOWN | K_LEFT | K_RIGHT); if (t >= 40 && (any || usedInput)) resolve(false); else if (t >= 180 + b) resolve(true); break; }
                    case EV_SKITTER: if (t >= 20 && (down & K_A)) mash++; if (mash >= 12) resolve(true); else if (t >= 170 + b) resolve(false); break;
                    case EV_MIMIC: if (t == 200) resolve(!hiding); break;
                    case EV_FLOOD: if (down & K_TOUCH) taps++; if (taps >= 10) resolve(true); else if (t >= 200 + b) resolve(false); break;
                    case EV_DEVOUR: if (t == 240 + b) resolve(hiding && (held & K_L)); break;
                    case EV_PENDULUM: if (t >= 30 && (down & K_A)) { int ps = pendPos(t); resolve(ps >= 70 && ps <= 130); } else if (t >= 210 + b) resolve(false); break;
                    case EV_GALE: if (t == 150 + b / 2) { uint32_t o = galeKey == K_LEFT ? K_RIGHT : K_LEFT; resolve((held & galeKey) && !(held & o)); } break;
                    case EV_BOSS: bossStep(down); break;
                }
            }
            break; }
        case DEAD: case WIN: if (down & K_A) state = TITLE; break;
    }
}

// ---------------------------------------------------------------- 3D rendering
static float zbuf[200]; static float ambK, fogK;
static bool shape(int type, float u, float v, int *r, int *g, int *b) {
    float x = u - .5f;
    switch (type) {
        case 1: { float y = v - .55f; if (x * x / .1764f + y * y / .2304f < 1) { float s = 1 - v * .5f; *r = (int)(34 * s); *g = (int)(8 * s); *b = (int)(52 * s);
            float ex = fabsf(x) - .17f, ey = v - .3f; if (ex * ex / .0064f + ey * ey / .0036f < 1) *r = *g = *b = 255;
            if (v > .5f && v < .66f && fabsf(x) < .25f) *r = *g = *b = sinf(u * 50) > 0 ? 235 : 10; return true; } return false; }
        case 2: { float y = v - .62f; if (x * x / .09f + y * y / .029f < 1) { *r = 52; *g = 30; *b = 22; float ex = fabsf(x) - .08f, ey = v - .58f; if (ex * ex + ey * ey < .0016f || (x * x + (v - .52f) * (v - .52f)) < .0009f) { *r = 255; *g = 30; *b = 30; } return true; }
            if (v > .55f && v < .96f - fabsf(x) * .5f && fabsf(fmodf(u * 8, 1) - .5f) < .09f) { *r = 28; *g = 16; *b = 12; return true; } return false; }
        case 3: { float y = v - .2f; if (x * x / .0289f + y * y / .0225f < 1) { *r = 205; *g = 212; *b = 235; float ex = fabsf(x) - .08f, ey = v - .2f; if (ex * ex / .0025f + ey * ey / .0064f < 1) *r = *g = *b = 5; return true; }
            if (v > .32f) { float hw = .1f + .38f * (v - .32f) / .68f; if (fabsf(x) < hw) { float s = 1 - v * .6f; *r = (int)(46 * s); *g = (int)(48 * s); *b = (int)(78 * s); return true; } } return false; }
        case 4: { float y = v - .5f; if (x * x / .25f + y * y / .1f < 1) { *r = 238; *g = 238; *b = 240; float d2 = x * x + y * y; if (d2 < .03f) { *r = 190; *g = 20; *b = 20; } if (d2 < .0045f) *r = *g = *b = 4; if (v < .3f) { *r = *r * 3 / 4; *g = *g * 3 / 4; *b = *b * 3 / 4; } return true; } return false; }
        case 5: { float y = v - .62f; if (x * x / .2f + y * y / .2f < 1) { float s = 1 - v * .45f; *r = (int)(80 * s) + bossNo() % 5 * 18; *g = (int)(10 * s); *b = (int)(30 * s) + bossNo() % 5 * 22;
            float ex = fabsf(x) - .14f, ey = v - .45f; if (ex * ex / .006f + ey * ey / .004f < 1) { *r = 255; *g = 240; *b = 120; } if (v > .62f && v < .7f && fabsf(x) < .22f) *r = *g = *b = 0; return true; }
            float h = .26f * (1 - fabsf(fmodf(u * 5, 1) * 2 - 1)); if (v > .3f - h && v < .3f && fabsf(x) < .36f) { *r = 220; *g = 180; *b = 60; return true; } return false; }
    }
    return false;
}
static void sprite(int type, float wx, float wy, float wsc, float hsc, int hor) {
    float dx = cosf(pa), dy = sinf(pa), plx = -dy * .66f, ply = dx * .66f, rx = wx - px_, ry = wy - py_, inv = 1.f / (plx * dy - dx * ply);
    float tX = inv * (dy * rx - dx * ry), tY = inv * (-ply * rx + plx * ry); if (tY < .25f) return;
    int sxp = (int)(200 * (1 + tX / tY)), h = (int)(240.f / tY * hsc), w = (int)(240.f / tY * wsc), y1 = hor + (int)(120.f / tY), y0 = y1 - h;
    float m = .55f + .45f * ambK;
    for (int x = sxp - w / 2; x < sxp + w / 2; x += 2) { if (x < 0 || x >= 400 || zbuf[x >> 1] < tY) continue;
        for (int y = y0 < 0 ? 0 : y0; y < y1 && y < 240; y++) { int r, g, b; if (shape(type, (x - (sxp - w / 2)) / (float)w, (y - y0) / (float)h, &r, &g, &b)) { r = (int)(r * m); g = (int)(g * m); b = (int)(b * m); pxl(x, y, r, g, b); pxl(x + 1, y, r, g, b); } } }
}
static void render_world(void) {
    float dx = cosf(pa), dy = sinf(pa), plx = -dy * .66f, ply = dx * .66f; int f = floorOf(room), hor = 120 + (int)(sinf(bobPh) * 3);
    static uint8_t rowc[240][3]; static float rowd[240];
    for (int y = 0; y < 240; y++) { float d = y < hor ? 120.f / (hor - y + .5f) : 120.f / (y - hor + .5f); float s = ambK / (1 + d * d * fogK); if (s > 1) s = 1; const int *c = y < hor ? CEC[f] : FLC[f];
        rowd[y] = d; float yv = 1 - 0.25f * fabsf((y - 120) / 120.f); s *= yv; rowc[y][0] = (uint8_t)c8((int)(c[0] * s * 1.5f)); rowc[y][1] = (uint8_t)c8((int)(c[1] * s * 1.5f)); rowc[y][2] = (uint8_t)c8((int)(c[2] * s * 1.5f)); }
    for (int rx = 0; rx < 200; rx++) {
        float cam = 2.f * rx / 200.f - 1.f, rdx = dx + plx * cam, rdy = dy + ply * cam; int mx = (int)px_, my = (int)py_;
        float ddx = fabsf(rdx) < 1e-6f ? 1e30f : fabsf(1.f / rdx), ddy = fabsf(rdy) < 1e-6f ? 1e30f : fabsf(1.f / rdy), sdx, sdy; int sx, sy, side = 0, tile = 1;
        if (rdx < 0) { sx = -1; sdx = (px_ - mx) * ddx; } else { sx = 1; sdx = (mx + 1.f - px_) * ddx; }
        if (rdy < 0) { sy = -1; sdy = (py_ - my) * ddy; } else { sy = 1; sdy = (my + 1.f - py_) * ddy; }
        for (int i = 0; i < 40; i++) { if (sdx < sdy) { sdx += ddx; mx += sx; side = 0; } else { sdy += ddy; my += sy; side = 1; } if (mx < 0 || my < 0 || mx >= MW || my >= MH) { tile = 1; break; } tile = map[my][mx]; if (tile) break; }
        float perp = side == 0 ? sdx - ddx : sdy - ddy; if (perp < .05f) perp = .05f; zbuf[rx] = perp;
        int lineH = (int)(240.f / perp), ds = hor - lineH / 2, de = hor + lineH / 2; float wx = side == 0 ? py_ + perp * rdy : px_ + perp * rdx; wx -= floorf(wx); int tx = (int)(wx * 64); if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) tx = 63 - tx;
        int ti = tile == 1 ? f : (tile == 5 ? 5 : (tile == 3 ? 6 : (tile == 4 ? 7 : 8)));
        float sh = ambK / (1 + perp * perp * fogK); if (side) sh *= .8f; sh *= 1.f - .35f * cam * cam; int shi = (int)(sh * 256); if (shi > 256) shi = 256;
        float tstep = 64.f / lineH, tpos = ds < 0 ? -ds * tstep : 0; int colf = (int)(256 * (1 - .35f * cam * cam));
        for (int k = 0; k < 2; k++) {
            uint8_t *col = FB + (rx * 2 + k) * 240 * 3; float tp = tpos;
            for (int y = 0; y < 240; y++) { uint8_t *p = col + (239 - y) * 3;
                if (y >= ds && y < de) { int tv = (int)tp; tp += tstep; if (tv > 63) tv = 63; if (tile == 5) tv += (int)(doorOpen * 64); int r, g, b;
                    if (tv > 63) { r = g = b = 0; } else { const uint8_t *tc = &TEX[ti][(tv * 64 + tx) * 3]; r = (tc[0] * shi) >> 8; g = (tc[1] * shi) >> 8; b = (tc[2] * shi) >> 8;
                        if (tile == 1 && ((mx * 7 + my * 3) & 3) == 0) { int ddx2 = tx - 32, ddy2 = tv - 22, d2 = ddx2 * ddx2 + ddy2 * ddy2; float lk = ambK * 1.4f; if (lk > 1.f) lk = 1.f;
                            if (d2 < 10) { r = (int)(255 * lk); g = (int)(214 * lk); b = (int)(140 * lk); } else if (d2 < 120) { int a = (int)((120 - d2) * 1.0f * lk); r += a; g += a * 3 / 4; b += a / 2; if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255; } }
                        if (tile == 5 && tx > 25 && tx < 39 && tv > 4 && tv < 12) { if (locked && !hasKey) { r = 230; g = 30; b = 30; } else { r = 40; g = 220; b = 70; } }
                        if (tile == 4 && mx == stx && my == sty && tx > 24 && tx < 40 && tv > 0 && tv < 5) { static const int SC[5][3] = { {0,0,0}, {240,40,40}, {250,200,60}, {250,140,40}, {60,160,255} }; r = SC[stKind][0]; g = SC[stKind][1]; b = SC[stKind][2]; }
                        if (tile == 3 && ev == EV_MIMIC && state == EVENT && t > 100 && tv > 10 && tv < 15 && ((tx > 18 && tx < 25) || (tx > 38 && tx < 45))) { r = 255; g = 20; b = 20; } }
                    p[0] = (uint8_t)b; p[1] = (uint8_t)g; p[2] = (uint8_t)r; }
                else { int fr_ = rowc[y][0], fg_ = rowc[y][1], fb_ = rowc[y][2]; float fx = px_ + rowd[y] * rdx, fy = py_ + rowd[y] * rdy;
                    if (y >= hor) { float cdx = fabsf(fx - 4.5f);
                        if (cdx < .8f) { fr_ = fr_ * 2; fg_ = fg_ * 2 / 5; fb_ = fb_ * 1 / 2; if (cdx > .66f) { fr_ = fr_ * 3 / 2; fg_ = fg_ * 3; fb_ = fb_ * 2 / 3; } }
                        else if ((((int)(fx * 2.f)) + ((int)(fy * 2.f))) & 1) { fr_ = fr_ * 7 / 10; fg_ = fg_ * 7 / 10; fb_ = fb_ * 7 / 10; } }
                    else if (fy - 3.f * (int)(fy * .3333f) < .35f) { fr_ = fr_ * 5 / 10; fg_ = fg_ * 5 / 10; fb_ = fb_ * 5 / 10; }
                    if (fr_ > 255) fr_ = 255; if (fg_ > 255) fg_ = 255; if (fb_ > 255) fb_ = 255;
                    p[0] = (uint8_t)((fb_ * colf) >> 8); p[1] = (uint8_t)((fg_ * colf) >> 8); p[2] = (uint8_t)((fr_ * colf) >> 8); } }
        }
    }
    if (state == EVENT) {
        if (ev == EV_LURKER && t >= warnLen) { float s = (t - warnLen) / 90.f; sprite(1, 4.5f + (px_ - 4.5f) * s, 1.5f + (py_ - 1.5f - .4f) * s, 1.1f, 1.45f, hor); }
        if (ev == EV_SKITTER && t >= 20) { float s = (t - 20) / 150.f; if (s > 1) s = 1; sprite(2, 4.5f + (px_ - 4.5f) * s, 2.f + (py_ - 2.f - .5f) * s, 1.3f, .9f, hor); }
        if (ev == EV_WHISPER && t >= 40) { float d = 7.f - (t - 40) * .025f; if (d < 2.6f) d = 2.6f; sprite(3, px_ + cosf(pa) * d, py_ + sinf(pa) * d, .9f, 1.5f, hor); }
    }
}
static float ambient(void) {
    if (state == TITLE || state == BESTIARY || state == ACHS) return .55f;
    float k = 1;
    if (state == EVENT) switch (ev) {
        case EV_LURKER: k = t < warnLen ? ((t / 5) % 2 ? 1.f : .14f) : .2f; break;
        case EV_GLARE: k = t < 120 ? ((t / 8) % 2 ? 1.f : .25f) : .3f; break;
        case EV_SNARE: k = t < 60 ? ((t / 3) % 2 ? 1.f : .15f) : .6f; break;
        case EV_WHISPER: k = t < 40 ? ((t / 6) % 2 ? 1.f : .3f) : .35f; break;
        case EV_SKITTER: k = t < 20 ? ((t / 4) % 2 ? 1.f : .2f) : .45f; break;
        case EV_MIMIC: k = t < 150 ? ((t / 10) % 2 ? 1.f : .4f) : .3f; break;
        case EV_FLOOD: k = .7f; break; case EV_DEVOUR: k = (t % 12 < 6) ? .25f : .1f; break;
        case EV_PENDULUM: k = t < 30 ? ((t / 5) % 2 ? 1.f : .3f) : .6f; break; case EV_GALE: k = .55f; break; default: k = .4f; }
    if (dark) k *= light ? 1.f : .3f;
    return k;
}
static void heart(int x, int y, bool on) { int r = on ? 230 : 70, g = on ? 30 : 30, b = on ? 50 : 40; box(x + 1, y, 2, 2, r, g, b, 255); box(x + 5, y, 2, 2, r, g, b, 255); box(x, y + 1, 8, 3, r, g, b, 255); box(x + 1, y + 4, 6, 1, r, g, b, 255); box(x + 2, y + 5, 4, 1, r, g, b, 255); box(x + 3, y + 6, 2, 1, r, g, b, 255); }
static void shapebox(int type, int x0, int y0, int w, int h, int a) { for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { int r, g, b; if (shape(type, x / (float)w, y / (float)h, &r, &g, &b)) { if (a >= 255) pxl(x0 + x, y0 + y, r, g, b); else pxa(x0 + x, y0 + y, r, g, b, a); } } }
static void overlays(void) {
    char b[64];
    if (state != EVENT) return;
    switch (ev) {
        case EV_GLARE: if (t >= 40) { int sz = t < 120 ? 20 + (t - 40) : 100 + (t - 120) * 2; shapebox(4, 200 - sz, 100 - sz / 3, sz * 2, sz * 2 / 3 + 10, 255); } break;
        case EV_DEVOUR: { int g = t * 7 / 10; if (g > 190) g = 190; box(0, 0, 400, g * 6 / 10, 0, 0, 0, 230); box(0, 240 - g * 6 / 10, 400, g * 6 / 10, 0, 0, 0, 230); box(0, 0, g, 240, 0, 0, 0, 230); box(400 - g, 0, g, 240, 0, 0, 0, 230); break; }
        case EV_SNARE: if (t >= 60) { snprintf(b, sizeof b, "PRESS %s!", keyName(snareKey)); txtc(b, 200, 70, 7, 255, 230, 60); } break;
        case EV_SKITTER: if (t >= 20) { snprintf(b, sizeof b, "MASH A!  %d/12", mash); txtc(b, 200, 62, 3, 255, 130, 60); } break;
        case EV_FLOOD: { int h = t * 6 / 10; if (h > 220) h = 220; box(0, 240 - h, 400, h, 30, 80, 200, 130); snprintf(b, sizeof b, "TAP THE SCREEN!  %d/10", taps); txtc(b, 200, 62, 3, 255, 255, 255); break; }
        case EV_PENDULUM: if (t >= 30) { int ps = pendPos(t); box(100, 196, 200, 12, 60, 60, 70, 255); box(170, 196, 60, 12, 60, 200, 80, 255); box(100 + ps - 2, 190, 5, 24, 255, 255, 255, 255); txtc("PRESS A IN THE GREEN!", 200, 160, 3, 255, 230, 120); } break;
        case EV_GALE: { bool l = galeKey == K_LEFT; for (int i = 0; i < 8; i++) { int x = (t * 14 + i * 53) % 400; if (l) x = 400 - x; box(x, 30 + i * 24, 36, 3, 255, 255, 255, 140); } snprintf(b, sizeof b, "HOLD %s!", l ? "LEFT" : "RIGHT"); txtc(b, 200, 90, 6, 255, 255, 255); break; }
        case EV_WHISPER: if (t >= 40) txtc("SHHH... STAY STILL...", 200, 196, 3, 190, 200, 255); break;
        case EV_BOSS: { int bn = bossNo() % 5; box(0, 0, 400, 240, 24, 0, 0, 190); shapebox(5, 120, 24, 160, 130, 255); txtc(BOSSN[bn], 200, 6, 2, 255, 100, 100);
            if (t < 60) txtc("GET READY!", 200, 170, 3, 255, 255, 255);
            else { box(20, 164, 360 * btime / (btimeMax > 0 ? btimeMax : 1), 6, 220, 40, 40, 255);
                for (int i = 0; i < blen; i++) { int cx = 200 - blen * 24 + i * 48 + 24; const char *k = keyName(bseq[i]); int sc = i == bpos ? 3 : 2; if (i < bpos) txtc(k, cx, 190, sc, 100, 255, 100); else if (i == bpos) txtc(k, cx, 186, sc, 255, 230, 60); else txtc(k, cx, 190, sc, 255, 255, 255); } }
            break; }
    }
}
void game_draw_top(uint8_t *fb) {
    FB = fb; SW = 400; char b[96];
    if (state == BESTIARY || state == ACHS) {
        box(0, 0, 400, 240, 10, 10, 15, 255);
        if (state == BESTIARY) {
            if (sel < NMON) { bool seen = (sv.seen >> sel) & 1; txt(seen ? EVN[sel] : "???", 16, 20, 4, 255, 255, 255); txt(seen ? EVT[sel] : "NOT YET ENCOUNTERED.", 16, 90, 2, 200, 200, 200); }
            else { int bi = sel - NMON; bool beat = (sv.bosses >> bi) & 1; txt(beat ? BOSSN[bi] : "??? (BOSS)", 16, 20, 4, 255, 120, 120); snprintf(b, sizeof b, "BOSS OF ROOM %d. REPEAT A %d-BUTTON", (bi + 1) * 20, 4 + bi); txt(beat ? b : "DEFEAT THIS BOSS TO LEARN MORE.", 16, 90, 2, 200, 200, 200); if (beat) txt("SEQUENCE BEFORE TIME RUNS OUT.", 16, 110, 2, 200, 200, 200); }
            snprintf(b, sizeof b, "ENTRY %d / %d", sel + 1, NENT); txt(b, 16, 210, 2, 150, 150, 150);
        } else { int got = 0; for (int i = 0; i < NACH; i++) if ((sv.ach >> i) & 1) got++; snprintf(b, sizeof b, "ACHIEVEMENTS  %d/%d", got, NACH); txt(b, 10, 4, 2, 255, 255, 255);
            for (int i = 0; i < NACH; i++) { bool gt = (sv.ach >> i) & 1; snprintf(b, sizeof b, "[%c] %s", gt ? 'X' : ' ', gt ? ACHN[i] : "???"); txt(b, 10, 22 + i * 14, 2, gt ? 120 : 110, gt ? 255 : 110, gt ? 120 : 110); if (gt) txt(ACHD[i], 190, 25 + i * 14, 1, 170, 170, 170); } }
        return;
    }
    if (state == CARD) { box(0, 0, 400, 240, 0, 0, 0, 255); txtc(FLOORS[floorOf(room)], 200, 80, 5, 255, 255, 255); txtc(FLORE[floorOf(room)], 200, 130, 2, 170, 170, 200); snprintf(b, sizeof b, "ROOMS %d - %d", floorOf(room) * 20 + 1, floorOf(room) * 20 + 20); txtc(b, 200, 160, 2, 140, 140, 140); return; }
    ambK = ambient(); fogK = (dark && !light) ? .2f : (light ? .012f : .016f); render_world();
    overlays();
    if (hiding) { box(0, 0, 400, 100, 0, 0, 0, 255); box(0, 140, 400, 100, 0, 0, 0, 255); for (int x = 0; x < 400; x += 40) box(x, 100, 14, 40, 0, 0, 0, 255); }
    if (eyesClosed) { box(0, 0, 400, 240, 0, 0, 0, 255); txtc("(EYES CLOSED)", 200, 110, 3, 110, 110, 110); }
    if (state == PLAY || state == EVENT || state == OPENING) {
        for (int i = 0; i < maxLives; i++) heart(8 + i * 11, 8, i < lives);
        if (shields) { snprintf(b, sizeof b, "SHIELD x%d", shields); txts(b, 8, 20, 1, 120, 200, 255); }
        snprintf(b, sizeof b, "ROOM %d", room); txts(b, 392 - tw(b, 2), 8, 2, 255, 255, 255);
        if (state != OPENING && !hiding && !eyesClosed) { float d; int tx, ty, tile = probe(&d, &tx, &ty); const char *p = 0;
            if (tile == 5 && d < 1.7f && state == PLAY) p = (locked && !hasKey) ? "LOCKED" : "A: OPEN DOOR"; else if (tile == 3 && d < 1.6f) p = "A: HIDE";
            if (p) txtc(p, 200, 150, 2, 255, 255, 255); }
        if (hiding) txtc("A: LEAVE CLOSET", 200, 150, 2, 255, 255, 255);
        if (state == EVENT && ev != EV_BOSS && t > 4 && t < 150 && ev != EV_SNARE) txtc(EVH[ev], 200, 36, 2, 255, 90, 90);
        if (stKind == 1 && lockType == 1 && hypotf(px_ - (stx + .5f), py_ - (sty + .5f)) < 2.3f) for (int i = 0; i < 3; i++) box(170 + i * 22, 190, 14, 14, pat[i] ? 255 : 50, pat[i] ? 230 : 50, pat[i] ? 80 : 50, 255);
        if (msgT > 0) txtc(msg, 200, 216, 2, 255, 255, 255);
        if (dark && !light && state == PLAY) txtc("DARK ROOM - HOLD SELECT FOR LIGHT", 200, 36, 1, 255, 230, 120);
    }
    if (flash > 0) box(0, 0, 400, 240, 200, 0, 0, flash * 5);
    if (fade > 0 && state != CARD) box(0, 0, 400, 240, 0, 0, 0, (int)(fade * 255));
    if (state == OPENING) box(0, 0, 400, 240, 0, 0, 0, t > 18 ? (t - 18) * 20 : 0);
    if (state == TITLE) { box(0, 0, 400, 240, 0, 0, 0, 120); txtc("HALLWAY 100", 200, 36, 7, 255, 255, 255); txtc("PRESS A TO BEGIN", 200, 112, 3, 230, 230, 230); snprintf(b, sizeof b, "< %s >", DIFFN[diff]); txtc(b, 200, 150, 2, 255, 200, 100); txtc("X: BESTIARY    Y: ACHIEVEMENTS", 200, 198, 2, 180, 180, 180); }
    if (state == DEAD) { box(0, 0, 400, 240, 120, 0, 0, 225); txtc("YOU DIED", 200, 70, 7, 255, 255, 255); snprintf(b, sizeof b, "REACHED ROOM %d   GOLD %d", room, gold); txtc(b, 200, 140, 2, 255, 220, 220); txtc("PRESS A", 200, 180, 2, 255, 255, 255); }
    if (state == WIN) { box(0, 0, 400, 240, 0, 0, 0, 255); txtc("YOU ESCAPED!", 200, 50, 6, 255, 255, 120); txtc("THE VOID WARDEN FALLS. THE HALL ENDS.", 200, 120, 2, 220, 220, 220); snprintf(b, sizeof b, "GOLD %d   DODGED %d", gold, dodged); txtc(b, 200, 150, 2, 220, 220, 220); txtc("TRY ENDLESS MODE!", 200, 190, 2, 180, 180, 255); }
    if (achT > 0) { box(40, 214, 320, 22, 0, 0, 0, 235); snprintf(b, sizeof b, "ACHIEVEMENT: %s", achText); txtc(b, 200, 220, 2, 255, 220, 100); }
}

// ---------------------------------------------------------------- bottom screen
void game_draw_bottom(uint8_t *fb) {
    FB = fb; SW = 320; char b[96]; box(0, 0, 320, 240, 15, 15, 20, 255);
    if (state == TITLE) { snprintf(b, sizeof b, "BEST ROOM %d/%d   ENDLESS %d", sv.best, LAST_ROOM, sv.bestEndless); txt(b, 10, 10, 2, 255, 255, 255); snprintf(b, sizeof b, "RUNS %d  DEATHS %d  ESCAPES %d", sv.runs, sv.deaths, sv.wins); txt(b, 10, 30, 2, 190, 190, 190);
        txt("CIRCLE PAD / D-PAD: MOVE + TURN", 10, 70, 2, 160, 160, 160); txt("A: OPEN DOORS, HIDE IN CLOSETS", 10, 88, 2, 160, 160, 160); txt("Y: BANDAGE  SELECT: FLASHLIGHT", 10, 106, 2, 160, 160, 160); txt("TOUCH: CABINETS, LEVERS, SHOP", 10, 124, 2, 160, 160, 160); txt("LEFT/RIGHT: DIFFICULTY", 10, 142, 2, 160, 160, 160); return; }
    if (state == BESTIARY) { txt("BESTIARY", 10, 10, 3, 255, 255, 255); txt("LEFT/RIGHT: BROWSE", 10, 60, 2, 180, 180, 180); txt("B: BACK", 10, 80, 2, 180, 180, 180); return; }
    if (state == ACHS) { txt("B: BACK", 10, 10, 2, 180, 180, 180); return; }
    if (state == CARD) { txtc("A: CONTINUE", 160, 110, 2, 180, 180, 180); return; }
    bool nearSt = (state == PLAY || state == EVENT) && stKind > 0 && !hiding && hypotf(px_ - (stx + .5f), py_ - (sty + .5f)) < 2.3f;
    snprintf(b, sizeof b, "ROOM %d/%d", room > LAST_ROOM ? LAST_ROOM : room, LAST_ROOM); if (ENDLESS) snprintf(b, sizeof b, "ROOM %d  ENDLESS", room);
    txt(b, 8, 6, 2, 255, 255, 255); txt(FLOORS[floorOf(room)], 200, 8, 1, 160, 160, 210);
    snprintf(b, sizeof b, "LIVES %d/%d  SHIELD %d  GOLD %d", lives, maxLives, shields, gold); txt(b, 8, 24, 1, 210, 210, 210);
    snprintf(b, sizeof b, "BANDAGE(Y) %d  KEYS %d  LANTERN %c  HOURGLASS %c", bandages, skeletons, lantern ? 'Y' : 'N', hourglass ? 'Y' : 'N'); txt(b, 8, 34, 1, 170, 170, 170);
    if (dark) { snprintf(b, sizeof b, "FLASHLIGHT %d%%  (HOLD SELECT)", battery); txt(b, 8, 44, 1, 255, 230, 120); }
    if (nearSt) {
        if (stKind == 1) { txt(lockType == 0 ? "SEARCH THE CABINET - TAP A DRAWER:" : "MATCH THE LIGHTS - TAP THE LEVERS:", 8, 80, 1, 255, 200, 90);
            for (int i = 0; i < 3; i++) { int x = 15 + i * 100;
                if (lockType == 0) { box(x, 120, 90, 70, opened[i] ? 40 : 110, opened[i] ? 28 : 72, opened[i] ? 20 : 40, 255); if (!opened[i]) box(x + 38, 150, 14, 6, 220, 200, 80, 255); else txtc(drawer[i] == 1 ? "KEY" : (drawer[i] == 2 ? "GOLD" : "EMPTY"), x + 45, 148, 2, 255, 255, 255); }
                else { box(x, 120, 90, 70, 45, 45, 60, 255); box(x + 40, cur[i] ? 126 : 152, 10, 34, 210, 210, 220, 255); txtc(cur[i] ? "ON" : "OFF", x + 45, 108, 2, cur[i] ? 255 : 140, cur[i] ? 230 : 140, cur[i] ? 80 : 140); } } }
        else if (stKind == 3) { txt(chestOpen ? "THE CHEST IS EMPTY." : "A CHEST! TAP IT:", 8, 80, 1, 255, 220, 120); box(110, 120, 100, 60, chestOpen ? 60 : 150, chestOpen ? 40 : 100, chestOpen ? 20 : 40, 255); box(110, 120, 100, 14, 90, 60, 25, 255); box(155, 135, 10, 10, 230, 210, 90, 255); }
        else if (stKind == 4) { txt(fountainUsed ? "THE FOUNTAIN HAS RUN DRY." : "A FOUNTAIN! TAP TO DRINK:", 8, 80, 1, 140, 200, 255); box(110, 150, 100, 30, 120, 120, 130, 255); box(118, 130, 84, 24, fountainUsed ? 40 : 60, fountainUsed ? 50 : 130, fountainUsed ? 70 : 230, 255); }
        else if (stKind == 2) { static const char *IN[6] = { "BANDAGE 10G", "SKEL KEY 15G", "SHIELD 20G", "LANTERN 40G", "HOURGLASS 30G", "LUCKY COIN 25G" }, *ID[6] = { "+1 LIFE (Y)", "OPENS A LOCK", "BLOCKS 1 HIT", "+30 REACTION", "+30 REACTION", "5 GOLD/DODGE" };
            txt("SHOP - TAP TO BUY:", 8, 66, 1, 255, 220, 120); for (int i = 0; i < 6; i++) { int x = 15 + (i % 2) * 160, y = 78 + (i / 2) * 40; box(x, y, 145, 36, 60, 45, 30, 255); txt(IN[i], x + 6, y + 6, 1, 255, 255, 255); txt(ID[i], x + 6, y + 20, 1, 200, 200, 170); } }
    } else {
        int ox = 12, oy = 62, cs = 7;                                       // minimap
        for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) { int tl = map[y][x], r = 28, g = 28, bl = 36; if (tl == 1) { r = 80; g = 80; bl = 96; } else if (tl == 2) { r = 120; g = 84; bl = 46; } else if (tl == 3) { r = 130; g = 70; bl = 170; } else if (tl == 4) { r = 220; g = 180; bl = 60; } else if (tl == 5) { r = 60; g = 200; bl = 90; if (locked && !hasKey) { r = 220; g = 50; bl = 50; } g = locked && !hasKey ? 50 : 200; }
            box(ox + x * cs, oy + y * cs, cs - 1, cs - 1, r, g, bl, 255); }
        int pxm = ox + (int)(px_ * cs), pym = oy + (int)(py_ * cs); box(pxm - 2, pym - 2, 5, 5, 255, 255, 255, 255); box(pxm + (int)(cosf(pa) * 6) - 1, pym + (int)(sinf(pa) * 6) - 1, 3, 3, 255, 90, 90, 255);
        txt("MAP", 12, 50, 1, 150, 150, 150);
        txt("WHITE: YOU   PURPLE: CLOSET", 90, 64, 1, 160, 160, 160); txt("GOLD: CABINET/SHOP/CHEST", 90, 76, 1, 160, 160, 160); txt("GREEN/RED: DOOR", 90, 88, 1, 160, 160, 160);
        if (state == EVENT && ev != EV_BOSS) txt(EVT[ev], 90, 116, 1, 255, 120, 120);
        if (state == EVENT && ev == EV_BOSS) txt("BOSS! PRESS THE BUTTONS IN ORDER!", 90, 116, 1, 255, 120, 120);
        txt("CIRCLE PAD: MOVE   A: INTERACT", 90, 150, 1, 130, 130, 130);
    }
    if (msgT > 0) txt(msg, 8, 224, 1, 255, 255, 255);
    if (state == DEAD || state == WIN) txt("PRESS A TO RETURN TO THE TITLE", 8, 200, 1, 255, 255, 255);
}

// ---------------------------------------------------------------- init + debug hooks
void game_init(uint32_t seed) {
    rs = seed ? seed : 1; gen_textures(); state = TITLE; room = 1; shop = locked = dark = chest = fountain = false; stKind = 0; genRoom(); pa = -PI / 2; fade = 0; sv.magic = SAVEMAGIC;
}
void game_dbg_info(DbgInfo *d) {
    d->px = px_; d->py = py_; d->pa = pa; d->state = state; d->ev = ev; d->t = t; d->room = room; d->hiding = hiding; d->stkind = stKind; d->stx = stx; d->sty = sty; d->lockType = lockType; d->hasKey = hasKey;
    for (int i = 0; i < 3; i++) d->pat[i] = pat[i]; d->nclosets = nclosets; for (int i = 0; i < 4; i++) { d->closets[i][0] = closets[i][0]; d->closets[i][1] = closets[i][1]; }
    d->blen = blen; d->bpos = bpos; d->nearSt = hypotf(px_ - (stx + .5f), py_ - (sty + .5f)) < 2.3f; d->locked = locked; d->dark = dark; for (int i = 0; i < 8; i++) d->bseq[i] = bseq[i];
}
void game_dbg_room(int r, int dif) { diff = dif; maxLives = dif == 0 ? 5 : (dif == 2 ? 1 : 3); if (lives <= 0 || state == TITLE) lives = maxLives; room = r; enterRoom(); if (state == CARD) state = afterCard; pendingEvent = false; msgT = 0; fade = 0; }
void game_dbg_event(int e) { if (e == EV_BOSS) startBoss(); else beginEvent(e); }
void game_dbg_lock(int l, int lt) { locked = l; lockType = lt; hasKey = false; stKind = l ? 1 : 0; genRoom(); }
