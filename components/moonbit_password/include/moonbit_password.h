#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void moonbit_init(void);
uint64_t passport_moonbit_initial_state(void);
uint64_t passport_moonbit_handle_input(uint64_t state, int32_t input);
int32_t passport_moonbit_state_get(uint64_t state, int32_t requested);
int32_t passport_moonbit_state_action(uint64_t state);
int32_t passport_moonbit_generate(uint64_t state);
int32_t passport_moonbit_entropy_x10(uint64_t state);

#ifdef __cplusplus
}
#endif
