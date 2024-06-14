/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#ifndef CGIMAP_PLUGIN_HPP
#define CGIMAP_PLUGIN_HPP

extern "C" int init_plugin();
extern "C" int deinit_plugin();

extern "C" const char* plugin_version();

#endif
