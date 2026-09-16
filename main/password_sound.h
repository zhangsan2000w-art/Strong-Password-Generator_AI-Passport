#pragma once

#include "esp_err.h"

/* Creates the low-priority playback worker. Audio failure is non-fatal. */
esp_err_t password_sound_init(void);

/* Non-blocking and coalescing: requests one short success chime. */
void password_sound_play_success(void);
