#pragma once
#include <stdint.h>

#include "types.h"

u8 *encrypt(const u8 *iv, u8 *msg, size_t s_msg, size_t *s_out);
