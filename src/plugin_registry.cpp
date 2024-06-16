/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap
 * (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2024 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#include "cgimap/plugins/plugin_registry.hpp"

#include <algorithm>
#include <dlfcn.h>
#include <fmt/core.h>
#include <vector>
#include <cassert>

#include "cgimap/plugins/cgimap_plugin.hpp"

PluginRegistry plugin_registry;

Plugin::Plugin(const std::filesystem::path &load_path)
    : name_(load_path.filename().string()), load_path_(load_path) {}

bool Plugin::is_loaded() const { return dlopen_handle_ != nullptr; }

std::filesystem::path Plugin::load_path() const { return load_path_; }

int Plugin::load() {
  if (is_loaded()) {
    fmt::print("Cannot load plugin {}: already loaded", load_path_.string());
    return -1;
  }

  int plugin_init_status = 0;
  plugin_info_func_t plugin_info_func = nullptr;
  plugin_init_func_t plugin_init_func = nullptr;

  dlopen_handle_ = dlopen(load_path_.c_str(), RTLD_NOW);
  if (!dlopen_handle_) {
    auto dlopen_error = dlerror();
    fmt::print("Unable to load plugin {}: {}\n", load_path_.string(), dlopen_error);
    return -1;
  }



  plugin_info_func = reinterpret_cast<plugin_info_func_t>(dlsym(dlopen_handle_, "plugin_info"));
  if (!plugin_info_func) {
    fmt::print("Plugin {} info function not found: {}\n", load_path_.string(), dlerror());
    goto error_unload;
  }

  info = plugin_info_func();
  if (!info->id || !info->name || !info->description || !info->version) {
    fmt::print("Plugin {} invalid plugin info\n", load_path_.string());
    goto error_unload;
  }

  plugin_init_func = reinterpret_cast<plugin_init_func_t>(dlsym(dlopen_handle_, "init_plugin"));
  if (!plugin_init_func) {
    fmt::print("Plugin {} init function not found: {}\n", info->id, dlerror());
    goto error_unload;
  }

  plugin_init_status = plugin_init_func();
  if (plugin_init_status)
  {
    fmt::print("Plugin {} init failed\n", info->id);
    goto error_unload;
  }

  fmt::print("Loaded plugin: {}\n", info->id);

  return 0;

error_unload:
  fmt::print("Failed to load plugin {}\n", load_path_.string());

  int close_status = dlclose(dlopen_handle_);
  if (close_status) {
    fmt::print("Failed to unload plugin {}\n", load_path_.string());
    return -1;
  }

  dlopen_handle_ = nullptr;

  return -1;
}

int Plugin::unload() {
  fmt::print("Unloading plugin: {}\n", load_path_.string());
  int close_status = dlclose(dlopen_handle_);
  if (close_status) {
    fmt::print("Failed to unload plugin {}\n", name_);
    return -1;
  }

  dlopen_handle_ = nullptr;

  return 0;
}

PluginRegistry::~PluginRegistry()
{
  unload_plugins();
}

int PluginRegistry::register_plugin(const std::filesystem::path &load_path) {
  bool already_exists = std::find_if(plugins.begin(), plugins.end(),
                                     [&load_path](const Plugin &p) {
                                       return p.load_path() == load_path;
                                     }) != plugins.end();
  if (already_exists)
    return -1;

  plugins.emplace_back(load_path);

  fmt::print("Registered plugin {}\n", load_path.string());

  return 0;
}

void PluginRegistry::load_plugins() {
  auto num_unloaded_plugins = std::count_if(plugins.begin(), plugins.end(), [](const Plugin& p){ return !p.is_loaded(); });
  fmt::print("Loading {}/{} plugins\n", num_unloaded_plugins, plugins.size());

  for (auto &plugin : plugins) {
    if (!plugin.is_loaded())
      plugin.load();
  }
}

void PluginRegistry::unload_plugins() {
  auto num_loaded_plugins = std::count_if(plugins.begin(), plugins.end(), [](const Plugin& p){ return p.is_loaded(); });
  if (num_loaded_plugins > 0)
  {
    fmt::print("Unloading {}/{} plugins\n", num_loaded_plugins, plugins.size());
  }
  
  for (auto it = plugins.rbegin(); it != plugins.rend(); ++it) {
    if (it->is_loaded())
        it->unload();
  }
}
