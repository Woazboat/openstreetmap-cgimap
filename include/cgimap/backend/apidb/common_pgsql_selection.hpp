/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#ifndef CGIMAP_BACKEND_APIDB_COMMON_PGSQL_SELECTION_HPP
#define CGIMAP_BACKEND_APIDB_COMMON_PGSQL_SELECTION_HPP

#include <cgimap/output_formatter.hpp>
#include <cgimap/backend/apidb/changeset.hpp>

#include <chrono>
#include <functional>
#include <pqxx/pqxx>
#include <string_view>
#include <string>
#include <vector>
#include <tuple>
#include <optional>

/* these functions take the results of "rolled-up" queries where tags, way
 * nodes and relation members are aggregated per-row.
 */

// extract nodes from the results of the query and write them to the formatter.
// the changeset cache is used to look up user display names.
void extract_nodes(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc);

// extract ways from the results of the query and write them to the formatter.
// the changeset cache is used to look up user display names.
void extract_ways(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc);

// extract relations from the results of the query and write them to the
// formatter. the changeset cache is used to look up user display names.
void extract_relations(
  const pqxx::result &rows, output_formatter &formatter,
  std::function<void(const element_info&)> notify,
  std::map<osm_changeset_id_t, changeset> &cc);

void extract_changesets(
  const pqxx::result &rows, output_formatter &formatter,
  std::map<osm_changeset_id_t, changeset> &cc,
  const std::chrono::system_clock::time_point &now,
  bool include_changeset_discussions);

// parses psql array based on specs given
// https://www.postgresql.org/docs/current/static/arrays.html#ARRAYS-IO
std::vector<std::string> psql_array_to_vector(std::string_view str);

class psql_array_view_no_unescape
{
public:
  class iterator
  {
  public:
    using value_type = std::pair<std::optional<std::string_view>, bool>;
    using reference = value_type&;
    using pointer = void;
    using difference_type = void;
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;


    constexpr iterator() = default;
    constexpr iterator(const iterator&) = default;

    explicit constexpr iterator(const char* _start) : start(_start) {
      auto n = next(start);
      sview = std::get<0>(n);
      sview_escaped = std::get<1>(n);
      start = std::get<2>(n);
    }

    constexpr iterator& operator++() {
      auto n = next(start);
      sview = std::get<0>(n);
      sview_escaped = std::get<1>(n);
      start = std::get<2>(n);

      return *this;
    }

    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr value_type operator*() const {
      return {sview, sview_escaped};
    }

    constexpr bool operator==(const iterator& other) const {
      return sview == other.sview &&
             sview_escaped == other.sview_escaped &&
             start == other.start;
    }

    constexpr bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    static constexpr std::tuple<std::optional<std::string_view>, bool, const char*> next(const char* str) {
      if (!str)
        return {};

      bool escaped = false;
      [[maybe_unused]]bool ever_escaped = false;
      bool quoted = false;
      auto e_start = str;

      const char* i = str;
      for (; *i; ++i) {
        if (escaped) {
          escaped = false;
          continue;
        }

        if (*i == escape_char) {
          escaped = true;
          ever_escaped = true;
          continue;
        }

        if (*i == quote_char) {
          if (quoted)
            return {std::string_view{e_start, static_cast<size_t>(std::distance(e_start, i))},
                    ever_escaped,
                    *(i+1) ? i+2 : i+1}; // skip quote char+separator char for next element (unless end of string reached)

          quoted = true;
          e_start = i + 1;
          continue;
        }

        if (quoted)
          continue;

        switch (*i) {
          case brace_open_char: // skip open brace at start
          {
            e_start = i + 1;
            continue;
          }
          case separator_char:
          case brace_close_char:
          {
            auto len = std::distance(e_start, i);
            if (len == 0) {
              return {{}, ever_escaped, i+1};
            }

            auto sv = std::string_view{e_start, static_cast<size_t>(len)};
            if (sv == "NULL") {
              return {{}, ever_escaped, i+1};
            }

            return {sv, ever_escaped, i+1}; // skip separator char for next element
          }
          default:
            continue;
        }
      }

      if (auto d = std::distance(e_start, i); d > 0)
        return {std::string_view{e_start, static_cast<size_t>(d)},
                ever_escaped,
                i};

      return {};
    }

  private:

    static const constexpr auto escape_char = '\\';
    static const constexpr auto quote_char = '"';
    static const constexpr auto separator_char = ',';
    static const constexpr auto brace_open_char = '{';
    static const constexpr auto brace_close_char = '}';

    std::optional<std::string_view> sview{};
    bool sview_escaped{false};
    const char* start{nullptr};
  };

  explicit constexpr psql_array_view_no_unescape(const char* str) : str(str) {}
  constexpr psql_array_view_no_unescape(const psql_array_view_no_unescape& other) = default;

  constexpr iterator begin() { return iterator(str); }
  constexpr iterator end() { return {}; }

private:
  const char* str;
};

inline std::ostream& operator<<(std::ostream& os,
    const psql_array_view_no_unescape::iterator::value_type & value ) {
  if (value.first)
    os << "{\"" << value.first.value() << "\", " << value.second << "}";
  else
    os << "{NULL, " << value.second << "}";
  return os;
}

inline std::string unescape(std::string_view str) {
  std::string out;

  bool escaped = false;
  for (auto c : str) {
    if (c == '\\' && !escaped) {
      escaped = true;
    } else {
      escaped = false;
      out.push_back(c);
    }
  }

  return out;
}

#endif /* CGIMAP_BACKEND_APIDB_COMMON_PGSQL_SELECTION_HPP */
