#pragma once
#include <stdint.h>

uint8_t* encrypt(const uint8_t *iv, uint8_t *msg, size_t s_msg, size_t *s_out);
