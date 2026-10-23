#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "assets/smw_assets.h"
#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "smw_rtl.h"
#include "smw_spc_player.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "renderer.h"

#define SMW_WIDTH 256
#define SMW_HEIGHT 224
#define SMW_ROW_BYTES (SMW_WIDTH * 4)

bool g_debug_flag;
bool g_other_image = true;
bool g_new_ppu = true;
int g_got_mismatch_count;
struct SpcPlayer *g_spc_player;

static uint8_t g_pixels[SMW_ROW_BYTES * 240];
static uint8_t g_my_pixels[SMW_ROW_BYTES * 240];

extern bool SmwXenonLoadAssets(void);

void Die(const char *error) {
  xsdk_log_write("smw: fatal: %s", error ? error : "(null)");
  exit(1);
}

void Warning(const char *error) {
  xsdk_log_write("smw: warning: %s", error ? error : "(null)");
}

void RtlApuLock(void) {
  /* No audio callback/thread is started by this frontend. */
}
void RtlApuUnlock(void) {
  /* APU calls are serialized on the game thread while audio is disabled. */
}

/* Match the upstream [GamepadMap] defaults and produce the same SNES bit layout. */
static uint32_t MapController(const INPUT_STATE *state) {
  const uint16_t buttons = state->gamepad.buttons;
  uint32_t input = 0;
  if ((buttons & INPUT_GAMEPAD_DPAD_UP) || state->gamepad.thumb_ly > 12000) input |= 1u << 4;
  if ((buttons & INPUT_GAMEPAD_DPAD_DOWN) || state->gamepad.thumb_ly < -12000) input |= 1u << 5;
  if ((buttons & INPUT_GAMEPAD_DPAD_LEFT) || state->gamepad.thumb_lx < -12000) input |= 1u << 6;
  if ((buttons & INPUT_GAMEPAD_DPAD_RIGHT) || state->gamepad.thumb_lx > 12000) input |= 1u << 7;

  /* Back=Select, Start=Start, B=A, A=B, Y=X, X=Y, LB=L, RB=R. */
  if (buttons & INPUT_GAMEPAD_BACK) input |= 1u << 2;
  if (buttons & INPUT_GAMEPAD_START) input |= 1u << 3;
  if (buttons & INPUT_GAMEPAD_B) input |= 1u << 8;
  if (buttons & INPUT_GAMEPAD_A) input |= 1u << 0;
  if (buttons & INPUT_GAMEPAD_Y) input |= 1u << 9;
  if (buttons & INPUT_GAMEPAD_X) input |= 1u << 1;
  if (buttons & INPUT_GAMEPAD_LEFT_SHOULDER) input |= 1u << 10;
  if (buttons & INPUT_GAMEPAD_RIGHT_SHOULDER) input |= 1u << 11;
  return input;
}

int main(void) {
  xsdk_log_write("smw: frontend start");
  if (!SmwXenonLoadAssets())
    return 2;

  xsdk_log_write("smw: game initialization begin");
  Snes *snes = SnesInit(NULL, 0);
  if (!snes) {
    xsdk_log_write("smw: SnesInit failed");
    return 3;
  }
  xsdk_log_write("smw: SnesInit ok mode=SMW");

  g_spc_player = SmwSpcPlayer_Create();
  if (!g_spc_player || !g_spc_player->initialize) {
    xsdk_log_write("smw: SPC player initialization unavailable");
    return 4;
  }
  g_spc_player->initialize(g_spc_player);
  xsdk_log_write("smw: SPC player state initialized; audio output disabled");

  PpuBeginDrawing(snes->ppu, g_pixels, SMW_ROW_BYTES, 0);
  PpuBeginDrawing(g_my_ppu, g_my_pixels, SMW_ROW_BYTES, 0);
  mkdir("saves", 0755);
  RtlReadSram();

  xsdk_log_write("smw: renderer initialization begin");
  if (!SmwXenonRendererOpen(SMW_WIDTH, SMW_HEIGHT)) {
    xsdk_log_write("smw: renderer initialization failed");
    return 5;
  }

  xsdk_log_write("smw: entering frame loop");
  bool logged_input_failure = false;
  uint32_t frame = 0;
  for (;;) {
    INPUT_STATE state = { 0 };
    const NTSTATUS input_status = XamInputGetState(0, 0, &state);
    if (!NT_SUCCESS(input_status)) {
      if (!logged_input_failure) {
        xsdk_log_write("smw: XamInputGetState failed status=0x%08X", (unsigned)input_status);
        logged_input_failure = true;
      }
      memset(&state, 0, sizeof state);
    }

    const uint16_t buttons = state.gamepad.buttons;
    if ((buttons & INPUT_GAMEPAD_BACK) && (buttons & INPUT_GAMEPAD_START)) {
      xsdk_log_write("smw: exit chord Back+Start");
      break;
    }

    const uint32_t inputs = MapController(&state) | (1u << 30);
    (void)RtlRunFrame(inputs);
    g_rtl_game_info->draw_ppu_frame();
    const uint8_t *pixels = g_other_image ? g_my_pixels : g_pixels;
    if (!SmwXenonRendererDraw(pixels, SMW_ROW_BYTES)) {
      xsdk_log_write("smw: frame presentation failed at frame=%u", (unsigned)frame);
      SmwXenonRendererClose();
      return 6;
    }
    if (frame == 0)
      xsdk_log_write("smw: first PPU frame drawn and submitted");
    frame++;
  }

  SmwXenonRendererClose();
  xsdk_log_write("smw: frontend exit after %u frames", (unsigned)frame);
  return 0;
}
