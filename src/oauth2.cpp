/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#include <cryptopp/config.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <sys/types.h>

#include <string_view>

#include "cgimap/oauth2.hpp"


inline std::string sha256_hash(const std::string& s) {

  using namespace CryptoPP;

  SHA256 hash;
  std::string digest;
  StringSource ss(
      s, true,
      new HashFilter(hash, new HexEncoder(new StringSink(digest), false)));
  return digest;
}


namespace oauth2 {

  [[nodiscard]] constexpr bool is_valid_bearer_token_char(char c) {
      // according to RFC 6750, section 2.1

      switch (c) {
          case 'a' ... 'z':
          case 'A' ... 'Z':
          case '0' ... '9':
          case '-':
          case '.':
          case '_':
          case '~':
          case '+':
          case '/':
          case '=':
              return true;  // we ignore that this char should only occur at end
      }

      return false;
  }

  [[nodiscard]] bool has_forbidden_char(std::string_view str) {
      return !std::all_of(str.begin(), str.end(), is_valid_bearer_token_char);
  }

  [[nodiscard]] std::optional<osm_user_id_t> validate_bearer_token(const request &req, data_selection& selection, bool& allow_api_write)
  {
    const char * auth_hdr = req.get_param("HTTP_AUTHORIZATION");
    if (auth_hdr == nullptr)
      return std::nullopt;

    const std::string_view auth_header{auth_hdr};

    // Auth header starts with Bearer?
    if (auth_header.rfind("Bearer ", 0) == std::string_view::npos)
      return std::nullopt;

    const auto bearer_token = auth_header.substr(7);

    if (bearer_token.empty())
      return std::nullopt;

    if (has_forbidden_char(bearer_token))
      return std::nullopt;

    bool expired = true;
    bool revoked = true;

    // Check token as plain text first
    auto user_id = selection.get_user_id_for_oauth2_token(bearer_token, expired, revoked, allow_api_write);

    // Fallback to sha256-hashed token
    if (!(user_id)) {
      const auto bearer_token_hashed = sha256_hash(std::string(bearer_token));
      user_id = selection.get_user_id_for_oauth2_token(bearer_token_hashed, expired, revoked, allow_api_write);
    }

    if (!(user_id)) {
      throw http::unauthorized("invalid_token");
    }

    if (expired) {
      throw http::unauthorized("token_expired");
    }

    if (revoked) {
      throw http::unauthorized("token_revoked");
    }

    return user_id;
  }
}
