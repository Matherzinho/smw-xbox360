#include "config.h"

/* Xenon frontend defaults; desktop parsing and SDL key maps remain PC-only. */
Config g_config = {
  .new_renderer = true,
  .enable_audio = false,
  .save_playthrough = false,
  .enable_gamepad = { true, false },
};
