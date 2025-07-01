#pragma once
#include <stdint.h>

#include "types.h"

u8 *encrypt(const u8 *iv, u8 *msg, std::size_t s_msg, std::size_t *s_out);
