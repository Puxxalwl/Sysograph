// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// (Full text in LICENSE; LGPL components listed in THIRD_PARTY_LICENSES.md)
#include "core/types.h"

namespace sysograph {

const char* process_state_key(char state) {
  switch (state) {
    case 'R': return "running";
    case 'S': return "sleeping";
    case 'D': return "disk-wait";
    case 'Z': return "zombie";
    case 'T': return "stopped";
    case 't': return "tracing-stop";
    case 'X': return "dead";
    case 'x': return "dead";
    case 'K': return "wakekill";
    case 'W': return "waking";
    case 'P': return "parked";
    case 'I': return "idle";
    default:  return "unknown";
  }
}

}  // namespace sysograph
