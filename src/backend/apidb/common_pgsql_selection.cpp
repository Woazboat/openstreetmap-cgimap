/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#include "cgimap/backend/apidb/common_pgsql_selection.hpp"
#include "cgimap/backend/apidb/apidb.hpp"
#include "cgimap/backend/apidb/utils.hpp"

#include <chrono>
#include <iterator>
#include <charconv>

namespace {

using pqxx_tuple = pqxx::result::reference;
using pqxx_field = pqxx::field;

void extract_elem(const pqxx_tuple &row, element_info &elem,
                  std::map<osm_changeset_id_t, changeset> &changeset_cache) {

  elem.id        = row["id"].as<osm_nwr_id_t>();
  elem.version   = row["version"].as<int>();
  elem.timestamp = row["timestamp"].c_str();
  elem.changeset = row["changeset_id"].as<osm_changeset_id_t>();
  elem.visible   = row["visible"].as<bool>();

  changeset & cs = changeset_cache[elem.changeset];

  if (cs.data_public) {
    elem.uid = cs.user_id;
    elem.display_name = cs.display_name;
  } else {
    elem.uid = {};
    elem.display_name = {};
  }
}

template <typename T>
std::optional<T> extract_optional(const pqxx_field &f) {
  if (f.is_null()) {
    return {};
  } else {
    return f.as<T>();
  }
}

void extract_changeset(const pqxx_tuple &row,
                       changeset_info &elem,
                       std::map<osm_changeset_id_t, changeset> &changeset_cache) {
  elem.id = row["id"].as<osm_changeset_id_t>();
  elem.created_at = row["created_at"].c_str();
  elem.closed_at = row["closed_at"].c_str();

  const auto & cs = changeset_cache[elem.id];

  if (cs.data_public) {
    elem.uid = cs.user_id;
    elem.display_name = cs.display_name;
  } else {
    elem.uid = {};
    elem.display_name = {};
  }

  auto min_lat = extract_optional<int64_t>(row["min_lat"]);
  auto max_lat = extract_optional<int64_t>(row["max_lat"]);
  auto min_lon = extract_optional<int64_t>(row["min_lon"]);
  auto max_lon = extract_optional<int64_t>(row["max_lon"]);

  if (bool(min_lat) && bool(min_lon) && bool(max_lat) && bool(max_lon)) {
    elem.bounding_box = bbox(double(*min_lat) / global_settings::get_scale(),
                             double(*min_lon) / global_settings::get_scale(),
                             double(*max_lat) / global_settings::get_scale(),
                             double(*max_lon) / global_settings::get_scale());
  } else {
    elem.bounding_box = {};
  }

  elem.num_changes = row["num_changes"].as<size_t>();
}

tags_t extract_tags(const pqxx_tuple &row) {
  tags_t tags;

  auto keys = psql_array_view_no_unescape<false, true, true>(row["tag_k"].c_str());
  auto values = psql_array_view_no_unescape<false, true, true>(row["tag_v"].c_str());

  auto k_it = keys.begin();
  auto v_it = values.begin();

  for (; k_it != keys.end() && v_it != values.end(); ++k_it, ++v_it) {
    auto [k_sv, k_esc] = *k_it;
    auto [v_sv, v_esc] = *v_it;
    tags.emplace_back(k_esc ? unescape(k_sv) : k_sv,
                      v_esc ? unescape(v_sv) : v_sv);
  }

  if (k_it != keys.end() || v_it != values.end()) {
    throw std::runtime_error("Mismatch in tags key and value size");
  }

  return tags;
}

nodes_t extract_nodes(const pqxx_tuple &row) {
  nodes_t nodes;

  auto ids = psql_array_view_no_unescape<false, false>(row["node_ids"].c_str());
  for (const auto id_str : ids) {
    osm_nwr_id_t id = 0;
    auto [ptr, ec] = std::from_chars(id_str.begin(), id_str.begin() + id_str.size(), id);
    if (ec != std::errc())
      throw std::runtime_error("Failed to convert node id to integer.");
    nodes.push_back(id);
  }

  return nodes;
}

element_type type_from_name(const char *name) {
  element_type type;

  switch (name[0]) {
  case 'N':
  case 'n':
    type = element_type_node;
    break;

  case 'W':
  case 'w':
    type = element_type_way;
    break;

  case 'R':
  case 'r':
    type = element_type_relation;
    break;

  default:
    // in case the name match isn't exhaustive...
    throw std::runtime_error(
        "Unexpected name not matched to type in type_from_name().");
  }

  return type;
}

element_type type_from_name(std::string_view name) {
  if (name.size() == 0)
    throw std::runtime_error("Type name is empty");
  return type_from_name(name.data());
}

[[nodiscard]] members_t extract_members(const pqxx_tuple &row) {
  members_t members;

  auto types = psql_array_view_no_unescape<false, false>(row["member_types"].c_str());
  auto ids = psql_array_view_no_unescape<false, false>(row["member_ids"].c_str());
  auto roles = psql_array_view_no_unescape<false>(row["member_roles"].c_str());

  auto t_it = types.begin();
  auto i_it = ids.begin();
  auto r_it = roles.begin();
  for (; t_it != types.end() && i_it != ids.end() && r_it != roles.end(); ++t_it, ++i_it, ++r_it) {
    auto id_str = *i_it;
    osm_nwr_id_t id = 0;
    auto [ptr, ec] = std::from_chars(id_str.begin(), id_str.begin() + id_str.size(), id);
    if (ec != std::errc())
      throw std::runtime_error("Failed to convert member ref id to integer.");

    auto [role_sv, escaped] = *r_it;
    members.emplace_back(type_from_name(*t_it), id, escaped ? unescape(role_sv) : std::string(role_sv));
  }

  if (t_it != types.end() || i_it != ids.end() || r_it != roles.end()) {
    throw std::runtime_error("Mismatch in members types, ids and roles size");
  }

  return members;
}

comments_t extract_comments(const pqxx_tuple &row) {
  comments_t comments;

  auto id_array           = psql_array_view_no_unescape<false, false, true>(row["comment_id"].c_str());
  auto author_id_array    = psql_array_view_no_unescape<false, false, true>(row["comment_author_id"].c_str());
  auto display_name_array = psql_array_view_no_unescape<false, true, true>(row["comment_display_name"].c_str());
  auto body_array         = psql_array_view_no_unescape<false, true, true>(row["comment_body"].c_str());
  auto created_at_array   = psql_array_view_no_unescape<false, false, true>(row["comment_created_at"].c_str());

  auto id_it           = id_array.begin();
  auto author_id_it    = author_id_array.begin();
  auto display_name_it = display_name_array.begin();
  auto body_it         = body_array.begin();
  auto created_at_it   = created_at_array.begin();

  for (; id_it != id_array.end() &&
         author_id_it != author_id_array.end() &&
         display_name_it != display_name_array.end() &&
         body_it != body_array.end() &&
         created_at_it != created_at_array.end();
         ++id_it, ++author_id_it, ++display_name_it, ++body_it, ++created_at_it) {

    auto id_sv = *id_it;
    osm_changeset_comment_id_t id = 0;
    auto [ptr1, ec1] = std::from_chars(id_sv.begin(), id_sv.begin() + id_sv.size(), id);
    if (ec1 != std::errc())
      throw std::runtime_error("Failed to convert comment id to integer.");

    auto author_id_sv = *author_id_it;
    osm_user_id_t author_id = 0;
    auto [ptr2, ec2] = std::from_chars(author_id_sv.begin(), author_id_sv.begin() + author_id_sv.size(), author_id);
    if (ec2 != std::errc())
      throw std::runtime_error("Failed to convert comment author id to integer.");

    auto [author_name_sv, author_name_esc] = *display_name_it;
    auto [body_sv, body_esc] = *body_it;

    comments.emplace_back(id, 
                          author_id, 
                          body_esc ? unescape(body_sv) : std::string(body_sv),
                          std::string(*created_at_it),
                          author_name_esc ? unescape(author_name_sv) : std::string(author_name_sv)
                          );
  }

    if (id_it != id_array.end() ||
        author_id_it != author_id_array.end() ||
        display_name_it != display_name_array.end() ||
        body_it != body_array.end() ||
        created_at_it != created_at_array.end()) {
    throw std::runtime_error("Mismatch in comments author_id, display_name, body and created_at size");
  }

  return comments;
}

struct node {
  struct extra_info {
    double lon, lat;
    inline void extract(const pqxx_tuple &row) {
      lon = double(row["longitude"].as<int64_t>()) / (global_settings::get_scale());
      lat = double(row["latitude"].as<int64_t>()) / (global_settings::get_scale());
    }
  };
  static inline void write(
    output_formatter &formatter, const element_info &elem,
    const extra_info &extra, const tags_t &tags) {
    formatter.write_node(elem, extra.lon, extra.lat, tags);
  }
};

struct way {
  struct extra_info {
    nodes_t nodes;
    inline void extract(const pqxx_tuple &row) {
      nodes = extract_nodes(row);
    }
  };
  static inline void write(
    output_formatter &formatter, const element_info &elem,
    const extra_info &extra, const tags_t &tags) {
    formatter.write_way(elem, extra.nodes, tags);
  }
};

struct relation {
  struct extra_info {
    members_t members;
    inline void extract(const pqxx_tuple &row) {
      members = extract_members(row);
    }
  };
  static inline void write(
    output_formatter &formatter, const element_info &elem,
    const extra_info &extra, const tags_t &tags) {
    formatter.write_relation(elem, extra.members, tags);
  }
};

template <typename T>
void extract(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc) {

  element_info elem;
  typename T::extra_info extra;

  for (const auto &row : rows) {
    extract_elem(row, elem, cc);
    extra.extract(row);
    tags_t tags = extract_tags(row);
    if (notify)
      notify(elem);     // let callback function know about a new element we're processing
    T::write(formatter, elem, extra, tags);
  }
}

} // anonymous namespace

void extract_nodes(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc) {
  extract<node>(rows, formatter, std::move(notify), cc);
}

void extract_ways(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc) {
  extract<way>(rows, formatter, std::move(notify), cc);
}

// extract relations from the results of the query and write them to the
// formatter. the changeset cache is used to look up user display names.
void extract_relations(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc) {
  extract<relation>(rows, formatter, std::move(notify), cc);
}

void extract_changesets(
  const pqxx::result &rows, output_formatter &formatter,
  std::map<osm_changeset_id_t, changeset> &cc, const std::chrono::system_clock::time_point &now,
  bool include_changeset_discussions) {

  changeset_info elem;

  for (const auto &row : rows) {
    extract_changeset(row, elem, cc);
    tags_t tags = extract_tags(row);
    comments_t comments = extract_comments(row);
    elem.comments_count = comments.size();
    formatter.write_changeset(
      elem, tags, include_changeset_discussions, comments, now);
  }
}

std::vector<std::string> psql_array_to_vector(std::string_view str) {
  std::vector<std::string> strs;
  std::string value;
  bool quotedValue = false;
  bool escaped = false;
  bool write = false;

  if (str == "{NULL}" || str.empty())
    return strs;

  const auto str_size = str.size();
  for (unsigned int i = 1; i < str_size; i++) {
    if (str[i] == ',') {
      if (quotedValue) {
        value += ",";
      } else {
        write = true;
      }
    } else if (str[i] == '\"') {
      if (escaped) {
        value += "\"";
        escaped = false;
      } else if (quotedValue) {
        quotedValue = false;
      } else {
        quotedValue = true;
      }
    } else if (str[i] == '\\') {
      if (escaped) {
        value += "\\";
        escaped = false;
      } else {
        escaped = true;
      }
    } else if (str[i] == '}') {
      if (quotedValue) {
        value += "}";
      } else {
        write = true;
      }
    } else {
      value += str[i];
    }

    if (write) {
      strs.push_back(value);
      value.clear();
      write = false;
    }
  }
  return strs;
}
