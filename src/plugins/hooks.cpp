/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#include "cgimap/plugins/hooks.hpp"

extern "C" void Hooks::register_callback_c(HookId hook, void* callback_func)
{
    HookDefinitions::hook_operations_t::register_callback(hook, callback_func);
}
