#ifndef BACKEND_APIDB_PQXX_STRING_TRAITS_HPP
#define BACKEND_APIDB_PQXX_STRING_TRAITS_HPP

#include "cgimap/types.hpp"
#include <set>
#include <pqxx/pqxx>

// See https://github.com/jtv/libpqxx/blob/master/include/pqxx/doc/datatypes.md
namespace pqxx {

/*
 * PQXX_ARRAY_STRING_TRAITS provides an instantiation of the string_traits
 * template from PQXX which is used to stringify arguments when sending them
 * to Postgres. Cgimap uses several different containers across different
 * integer types, all of which stringify to arrays in the same way.
 *
 * Note that it would be nicer to hide this in a .cpp, but it seems that the
 * implementation has to be available when used in the prepared statement
 * code.
 */
#define PQXX_ARRAY_STRING_TRAITS(type)                                  \
  template <>                                                           \
  struct nullness<type> : no_null<type> {};                             \
                                                                        \
  template<>                                                            \
  struct string_traits<type> : internal::array_string_traits<type> {};


PQXX_ARRAY_STRING_TRAITS(std::set<osm_nwr_id_t>);
PQXX_ARRAY_STRING_TRAITS(std::set<osm_changeset_id_t>);

} // namespace pqxx

#endif /* BACKEND_APIDB_PQXX_STRING_TRAITS_HPP */
