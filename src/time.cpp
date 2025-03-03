/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2024 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#include "cgimap/time.hpp"

#include <stdexcept>
#include <sstream>
#include <cctype>
#include <ctime>
#include <format>



std::chrono::system_clock::time_point parse_time(const std::string &s) {
  // parse only YYYY-MM-DDTHH:MM:SSZ
  if ((s.size() == 20) && (s[19] == 'Z')) {
    std::tm tm{};
    strptime(s.c_str(), "%Y-%m-%dT%H:%M:%S%z", &tm);
    auto tp = std::chrono::system_clock::from_time_t(timegm(&tm));

    return tp;
  }

  throw std::runtime_error(std::format("Unable to parse string '{}' as an ISO 8601 format date time.", s));
}
