/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#include "cgimap/backend/apidb/changeset.hpp"
#include <utility>

changeset::changeset(bool data_public, std::string display_name, osm_user_id_t user_id)
    : data_public(data_public), display_name(std::move(display_name)), user_id(user_id) {}

