#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t password_platform_init(void);
bool password_platform_ready(void);
const char *password_platform_output(void);
void password_platform_clear_output(void);

/* 自检使用独立的瞬时缓冲区，不覆盖或持久化用户刚生成的密码。 */
void password_platform_begin_diagnostics(void);
const char *password_platform_diagnostic_output(void);
void password_platform_end_diagnostics(void);
