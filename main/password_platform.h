#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t password_platform_init(void);
bool password_platform_ready(void);
const char *password_platform_output(void);
void password_platform_clear_output(void);
