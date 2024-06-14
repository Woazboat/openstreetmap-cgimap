/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <cassert>
#include <utility>
#include <map>
#include <string>
#include <string_view>

#include <exception>

#include <fmt/core.h>

using osm_user_id_t = uint64_t ;
using osm_changeset_id_t = int64_t;
using osm_changeset_comment_id_t = int32_t;
using osm_nwr_id_t = uint64_t;
using osm_nwr_signed_id_t = int64_t;
using tile_id_t = uint32_t;
using osm_version_t = uint32_t;
using osm_redaction_id_t = uint64_t;
using osm_sequence_id_t = uint64_t;

// a edition of an element is the pair of its ID and version.
using osm_edition_t = std::pair<osm_nwr_id_t, osm_version_t>;

// user roles, used in the OSM permissions system.
enum class osm_user_role_t {
  administrator,
  moderator
};

// OSMChange message operations
enum class operation {
  op_undefined = 0,
  op_create = 1,
  op_modify = 2,
  op_delete = 3
};

template <typename T = std::string_view>
constexpr T osm_operation_name(operation op) noexcept {

  switch (op) {
  case operation::op_undefined:
    return "undefined";
  case operation::op_create:
    return "create";
  case operation::op_modify:
    return "modify";
  case operation::op_delete:
    return "delete";
  }

  return "unknown";
}

// OSMChange message object type
enum class object_type {
  node = 1,
  way = 2,
  relation = 3
};

template <typename T = std::string_view>
constexpr T osm_object_type_name(object_type elt) noexcept {

  switch (elt) {
  case object_type::node:
    return "node";
  case object_type::way:
    return "way";
  case object_type::relation:
    return "relation";
  }

  return "unknown";
}

namespace api06 {

using TagList = std::map<std::string, std::string>;

} // namespace api06

#endif /* TYPES_HPP */
