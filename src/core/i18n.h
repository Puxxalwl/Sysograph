// Sysograph — a free, original Linux system task manager.
// Copyright (C) 2025 The Sysograph Project Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Tiny built-in translation layer (English / Russian). Keeping this in
// our own code (instead of pulling in a gettext toolchain at build time)
// makes the project dependency-free and trivially auditable. Language
// is picked from the environment (LC_ALL / LC_MESSAGES / LANG) and can
// be forced with SYSSOGRAPH_LANG=en|ru.
#pragma once

namespace sysograph {
namespace i18n {

// Reads the environment once and selects the language.
void init();

// Returns the translation for an English source string; falls back to
// the key itself when no translation exists.
const char* tr(const char* key);

// Currently active language ("en" or "ru").
const char* activeLanguage();

}  // namespace i18n
}  // namespace sysograph

// Translation accessor used throughout the UI sources.
#define _(KEY) (::sysograph::i18n::tr(KEY))
