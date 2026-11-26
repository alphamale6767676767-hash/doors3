// Hallway 100 - Nintendo 3DS platform layer (raw framebuffer; no GPU libraries needed)
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "game.h"

static void logm(const char *m) { FILE *f = fopen("sdmc:/hallway100_log.txt", "a"); if (f) { fputs(m, f); fputc(10, f); fclose(f); } }
static const char *SAVEFILE = "sdmc:/hallway100.sav";

int main(void) {
    logm("--- launch ---");
    gfxInitDefault(); gfxSet3D(false); osSetSpeedupEnable(true);
    logm("gfx ok");
    game_init((uint32_t)osGetTime());
    { Save tmp; FILE *f = fopen(SAVEFILE, "rb"); if (f) { if (fread(&tmp, sizeof tmp, 1, f) == 1 && tmp.magic == sv.magic) sv = tmp; fclose(f); } }
    logm("init ok");
    bool first = true;
    while (aptMainLoop()) {
        hidScanInput();
        Input in; in.down = hidKeysDown(); in.held = hidKeysHeld();
        touchPosition tp; hidTouchRead(&tp); in.tx = tp.px; in.ty = tp.py;
        circlePosition cp; hidCircleRead(&cp); in.cx = cp.dx; in.cy = cp.dy;
        DbgInfo di; game_dbg_info(&di);
        if ((in.down & KEY_START) && di.state == 0) break;      // START quits from the title screen only
        game_update(&in);
        u16 w, h;
        game_draw_top(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h));
        game_draw_bottom(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h));
        gfxFlushBuffers(); gfxSwapBuffers();
        if (first) { first = false; logm("first frame ok"); }
        if (g_saveDirty) { g_saveDirty = 0; FILE *f = fopen(SAVEFILE, "wb"); if (f) { fwrite(&sv, sizeof sv, 1, f); fclose(f); } }
        gspWaitForVBlank();
    }
    logm("exiting normally");
    gfxExit();
    return 0;
}
