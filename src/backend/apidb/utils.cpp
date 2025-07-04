/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2025 by the openstreetmap-cgimap developer community.
 * For a full list of authors see the git log.
 */

#include <charconv>
#include <string_view>
#include <string>
#include <vector>

#include <fmt/core.h>
#include <pqxx/pqxx>

#include "cgimap/backend/apidb/utils.hpp"

void check_postgres_version(const pqxx::connection_base &conn) {
  auto version = conn.server_version();
  if (version < 110000) {
    throw std::runtime_error(fmt::format("Expected Postgres version 11+, currently installed version {}", version));
  }
}

void extract_bbox_from_row(const pqxx::row &row, bbox_t &result) {

  if (row["minlat"].is_null())
    return;

  result.minlat = row["minlat"].as<int64_t>();
  result.minlon = row["minlon"].as<int64_t>();
  result.maxlat = row["maxlat"].as<int64_t>();
  result.maxlon = row["maxlon"].as<int64_t>();
}

/**
 * From: https://www.postgresql.org/docs/current/libpq-connect.html
 * Keyword/Value Connection Strings
 *
 * To write an empty value, or a value containing spaces, surround it
 * with single quotes, for example keyword = 'a value'. Single quotes and
 * backslashes within a value must be escaped with a backslash, i.e., \' and \\.
 */

std::string escape_pg_value(const std::string &value) {
  std::string escaped;
  bool needs_quotes = value.empty();
  for (char c : value) {
    if (c == '\'' || c == '\\') {
      escaped += '\\';
      needs_quotes = true;
    }
    if (c == ' ') {
      needs_quotes = true;
    }
    escaped += c;
  }
  if (needs_quotes) {
    return "'" + escaped + "'";
  }
  return escaped;
}

std::vector<std::string> psql_array_to_vector(const pqxx::field& field, int size_hint) {
  return psql_array_to_vector(std::string_view(field.c_str(), field.size()), size_hint);
}

std::vector<std::string> psql_array_to_vector(std::string_view str, int size_hint) {
  std::vector<std::string> strs;
  std::string value;
  bool quotedValue = false;
  bool escaped = false;
  bool write = false;

  if (size_hint > 0)
    strs.reserve(size_hint);

  if (str == "{NULL}" || str.empty())
    return strs;

  const auto str_size = str.size();
  for (unsigned int i = 1; i < str_size; i++) {
    if (str[i] == ',') {
      if (quotedValue) {
        value += ',';
      } else {
        write = true;
      }
    } else if (str[i] == '"') {
      if (escaped) {
        value += '"';
        escaped = false;
      } else if (quotedValue) {
        quotedValue = false;
      } else {
        quotedValue = true;
      }
    } else if (str[i] == '\\') {
      if (escaped) {
        value += '\\';
        escaped = false;
      } else {
        escaped = true;
      }
    } else if (str[i] == '}') {
      if (quotedValue) {
        value += '}';
      } else {
        write = true;
      }
    } else {
      value += str[i];
    }

    if (write) {
      strs.emplace_back(std::move(value));
      value.clear();
      write = false;
    }
  }
  return strs;
}

template <typename T>
std::vector<T> psql_array_ids_to_vector(const pqxx::field& field) {
  return psql_array_ids_to_vector<T>(std::string_view(field.c_str(), field.size()));
}

template <typename T>
std::vector<T> psql_array_ids_to_vector(std::string_view str) {
  std::vector<T> ids;
  int start_offset = 1;

  if (str == "{NULL}" || str.empty())
    return ids;

  const auto str_size = str.size();

  for (unsigned int i = 1; i < str_size; i++) {
    if (str[i] == ',' || str[i] == '}') {
      T id;

      auto [_, ec] = std::from_chars(str.data() + start_offset, str.data() + i, id);

      if (ec != std::errc()) {
       throw std::runtime_error("Conversion to integer failed");
      }
      ids.emplace_back(id);
      start_offset = i + 1;
    }
  }
  return ids;
}


template std::vector<int32_t> psql_array_ids_to_vector(const pqxx::field& field);
template std::vector<int64_t> psql_array_ids_to_vector(const pqxx::field& field);

template std::vector<uint32_t> psql_array_ids_to_vector(const pqxx::field& field);
template std::vector<uint64_t> psql_array_ids_to_vector(const pqxx::field& field);

template std::vector<int32_t> psql_array_ids_to_vector(std::string_view str);
template std::vector<int64_t> psql_array_ids_to_vector(std::string_view str);

template std::vector<uint32_t> psql_array_ids_to_vector(std::string_view str);
template std::vector<uint64_t> psql_array_ids_to_vector(std::string_view str);


