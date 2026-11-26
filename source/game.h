#pragma once
#include <stdint.h>
#include <stdbool.h>
// key bits match libctru's KEY_* values
#define K_A (1u<<0)
#define K_B (1u<<1)
#define K_SELECT (1u<<2)
#define K_START (1u<<3)
#define K_RIGHT (1u<<4)
#define K_LEFT (1u<<5)
#define K_UP (1u<<6)
#define K_DOWN (1u<<7)
#define K_R (1u<<8)
#define K_L (1u<<9)
#define K_X (1u<<10)
#define K_Y (1u<<11)
#define K_TOUCH (1u<<20)
typedef struct { int magic, best, bestEndless, deaths, wins, runs, seen, bosses, ach; } Save;
typedef struct { uint32_t down, held; int tx, ty; int cx, cy; } Input;   // cx,cy: circle pad -156..156 (cy>0 = up)
extern Save sv;
extern int g_saveDirty;
void game_init(uint32_t seed);
void game_update(const Input *in);
void game_draw_top(uint8_t *fb);      // 400x240, 3DS native: BGR8, column-major, origin bottom-left
void game_draw_bottom(uint8_t *fb);   // 320x240, same format
// debug / trailer hooks
typedef struct { float px, py, pa; int state, ev, t, room, hiding, stkind, stx, sty, lockType, hasKey, pat[3], closets[4][2], nclosets, blen, bpos, nearSt, locked, dark; unsigned bseq[8]; } DbgInfo;
void game_dbg_info(DbgInfo *d);
void game_dbg_room(int room, int diff);
void game_dbg_event(int ev);
void game_dbg_lock(int locked, int lockType);
