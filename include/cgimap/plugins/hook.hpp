/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#ifndef CGIMAP_HOOK_HPP
#define CGIMAP_HOOK_HPP

#include <optional>
#include <set>

#include <fmt/core.h>
#include <fmt/compile.h>

#include "cgimap/types.hpp"
#include "cgimap/request.hpp"

#include "cgimap/api06/changeset_upload/osmchange_handler.hpp"
#include "cgimap/api06/changeset_upload/osmchange_tracking.hpp"
#include "cgimap/backend/apidb/changeset_upload/node_updater.hpp"
#include "cgimap/backend/apidb/changeset_upload/way_updater.hpp"

namespace Hooks
{
    enum class HookAction
    {
        CONTINUE,
        ABORT
    };

    enum class Hook : int
    {
        POST_PLUGIN_LOAD,
        POST_SOCKET_INIT,
        DAEMON_MODE_POST_FORK,
        REQUEST_START,
        WRITE_REQUEST,
        CHANGESET_CREATE,
        CHANGESET_UPLOAD,
        NODE_CREATED,
        NODE_MODIFIED,
        NODE_DELETED,
        WAY_CREATED,
        WAY_MODIFIED,
        WAY_DELETED
    };

    template<Hook H>
    struct HookTraits
    {
        // using CallbackType = void;
    };

    template<typename TCallback>
    struct HookTraitsBase
    {
        using CallbackFunc = TCallback;
        using CallbackFuncPtr = std::add_pointer_t<CallbackFunc>;
    };

    template<> struct HookTraits<Hook::POST_PLUGIN_LOAD>      : HookTraitsBase<void ()> {};
    template<> struct HookTraits<Hook::POST_SOCKET_INIT>      : HookTraitsBase<void (int)> {};
    template<> struct HookTraits<Hook::DAEMON_MODE_POST_FORK> : HookTraitsBase<void ()> {};
    template<> struct HookTraits<Hook::REQUEST_START>         : HookTraitsBase<HookAction (const request&, const std::optional<osm_user_id_t>&, const std::set<osm_user_role_t>&)> {};
    template<> struct HookTraits<Hook::WRITE_REQUEST>         : HookTraitsBase<HookAction (const request& req, const std::string& payload, const std::string& ip, const std::optional<osm_user_id_t>& user_id)> {};
    template<> struct HookTraits<Hook::CHANGESET_CREATE>      : HookTraitsBase<HookAction (osm_user_id_t user_id, const api06::TagList& tags)> {};
    template<> struct HookTraits<Hook::CHANGESET_UPLOAD>      : HookTraitsBase<HookAction (osm_user_id_t user_id, osm_changeset_id_t changeset, const api06::OSMChange_Handler& osmchange_handler, const api06::OSMChange_Tracking& change_tracking, const std::vector<api06::diffresult_t> diffresult)> {};
    template<> struct HookTraits<Hook::NODE_CREATED>          : HookTraitsBase<HookAction (const ApiDB_Node_Updater::node_t& new_node)> {};
    template<> struct HookTraits<Hook::NODE_MODIFIED>         : HookTraitsBase<HookAction (const ApiDB_Node_Updater::node_t& modified_node)> {};
    template<> struct HookTraits<Hook::NODE_DELETED>          : HookTraitsBase<HookAction (const ApiDB_Node_Updater::node_t& deleted_node)> {};
    template<> struct HookTraits<Hook::WAY_CREATED>           : HookTraitsBase<HookAction (const ApiDB_Way_Updater::way_t& node)> {};
    template<> struct HookTraits<Hook::WAY_MODIFIED>          : HookTraitsBase<HookAction (const ApiDB_Way_Updater::way_t& node)> {};
    template<> struct HookTraits<Hook::WAY_DELETED>           : HookTraitsBase<HookAction (const ApiDB_Way_Updater::way_t& node)> {};



    template<Hook hook>
    struct HookInstance
    {
        static constexpr Hook hookId = hook;

        using TCallback = typename HookTraits<hook>::CallbackFunc;

        HookInstance(const TCallback* callback_func) : callback(callback_func) {}

        template<typename... Args>
        void operator()(Args... args) { callback(std::forward<Args>(args)...); }

        TCallback* callback;
    };


    template<Hook... hooks>
    struct HookDefinitionsHelper
    {
        using HooksVariant = std::variant<HookInstance<hooks>...>;

        template<Hook h1, Hook... rest>
        struct hook_operations
        {
            static constexpr HooksVariant make_hook_variant(Hook h, void* callback)
            {
                if (h == h1)
                {
                    return {HookInstance<h1>{
                        reinterpret_cast<typename HookTraits<h1>::CallbackFuncPtr>(callback)
                    }};
                }
                else
                {
                    if constexpr (sizeof...(rest) > 0)
                    {
                        return hook_operations<rest...>::make_hook_variant(h, callback);
                    }
                    else
                    {
                        throw std::runtime_error(fmt::format(FMT_COMPILE("Unsupported hook id: {}"), static_cast<std::underlying_type_t<Hook>>(h1)));
                    }
                }
            }
        };

        using hook_operations_t = hook_operations<hooks...>;
    };

    using HookDefinitions = HookDefinitionsHelper<
        Hook::POST_PLUGIN_LOAD, 
        Hook::POST_SOCKET_INIT,
        Hook::REQUEST_START,
        Hook::WRITE_REQUEST,
        Hook::CHANGESET_CREATE,
        Hook::CHANGESET_UPLOAD,
        Hook::NODE_CREATED,
        Hook::NODE_MODIFIED,
        Hook::NODE_DELETED,
        Hook::WAY_CREATED,
        Hook::WAY_MODIFIED,
        Hook::WAY_DELETED
    >;
};

#endif
