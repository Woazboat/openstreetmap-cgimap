/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap
 * (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2024 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <variant>
#include <iostream>
#include <type_traits>
#include <functional>

class Plugin {
public:
  Plugin(const std::filesystem::path &load_path);

  int load();
  int unload();

  [[nodiscard]] bool is_loaded() const;

  std::filesystem::path load_path() const;

private:
  using init_func_t = int (*)();

  std::string name_;
  std::filesystem::path load_path_;
  void *dlopen_handle_ = nullptr;
};

class PluginRegistry {
public:
  PluginRegistry() = default;
  ~PluginRegistry();

  PluginRegistry(const PluginRegistry &) = delete;
  PluginRegistry &operator=(const PluginRegistry &) = delete;

  int register_plugin(const std::filesystem::path &load_path);

  void load_plugins();
  void unload_plugins();

private:
  std::vector<Plugin> plugins;
};

extern PluginRegistry plugin_registry;



