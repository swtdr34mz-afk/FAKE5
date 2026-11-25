/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* LOAD SYX: a Take 5's program dumps (a .syx file: one program, a bank, a whole set) into FAKE5 programs. FAKE5 ships
 * none of the Take 5's own sounds; this reads the ones its owner has. */
#pragma once
#include "Params.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace f5
{
/* every Take 5 program dump (F0 01 35 02 / 03 ...) in the bytes, in order; none: empty */
std::vector<Program> importTake5Syx(const uint8_t *data, size_t size);
} // namespace f5
