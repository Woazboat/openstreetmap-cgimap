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

enum class HookAction
{
    CONTINUE,
    ABORT
};

enum class HookId : int
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

template<HookId h, typename TCallback>
struct HookBase
{
    using CallbackFunc = TCallback;
    using CallbackFuncPtr = std::add_pointer_t<CallbackFunc>;

    static constexpr HookId hook_id = h;

    static inline std::vector<CallbackFuncPtr> registered_callbacks;

    static void register_callback(CallbackFuncPtr callback_func)
    {
        registered_callbacks.emplace_back(callback_func);
    }

    template<typename... Args, std::enable_if_t<std::is_invocable_v<CallbackFuncPtr, Args...>, bool> = true>
    static HookAction call(Args&&... args)
    {
        for (const auto cb : registered_callbacks)
        {
            std::cout << "Calling hook function\n";
            static_assert(std::is_invocable_v<decltype(cb), Args...>, "Non-invokable hook callback function");

            using res_t = std::invoke_result_t<decltype(cb), Args...>;
            
            if constexpr (!std::is_same_v<res_t, void>)
            {
                auto retval = std::invoke(cb, std::forward<Args>(args)...);
                if (retval != HookAction::CONTINUE)
                {
                    fmt::print(FMT_COMPILE("Hook for {} requested action: {}\n"), 
                        static_cast<std::underlying_type_t<HookId>>(hook_id), 
                        static_cast<std::underlying_type_t<HookAction>>(retval));
                    return retval;
                }
            }
            else
            {
                std::invoke(cb, std::forward<Args>(args)...);
            }
        }

        return HookAction::CONTINUE;
    }
};

template<HookId H> 
struct Hook {};

template<> struct Hook<HookId::POST_PLUGIN_LOAD>      : HookBase<HookId::POST_PLUGIN_LOAD, void ()> {};
template<> struct Hook<HookId::POST_SOCKET_INIT>      : HookBase<HookId::POST_SOCKET_INIT, void (int)> {};
template<> struct Hook<HookId::DAEMON_MODE_POST_FORK> : HookBase<HookId::DAEMON_MODE_POST_FORK, void ()> {};
template<> struct Hook<HookId::REQUEST_START>         : HookBase<HookId::REQUEST_START, HookAction (const request&, const std::optional<osm_user_id_t>&, const std::set<osm_user_role_t>&)> {};
template<> struct Hook<HookId::WRITE_REQUEST>         : HookBase<HookId::WRITE_REQUEST, HookAction (const request& req, const std::string& payload, const std::string& ip, const std::optional<osm_user_id_t>& user_id)> {};
template<> struct Hook<HookId::CHANGESET_CREATE>      : HookBase<HookId::CHANGESET_CREATE, HookAction (osm_user_id_t user_id, const api06::TagList& tags)> {};
template<> struct Hook<HookId::CHANGESET_UPLOAD>      : HookBase<HookId::NODE_CREATED, HookAction (osm_user_id_t user_id, osm_changeset_id_t changeset, const api06::OSMChange_Handler& osmchange_handler, const api06::OSMChange_Tracking& change_tracking, const std::vector<api06::diffresult_t> diffresult)> {};
template<> struct Hook<HookId::NODE_CREATED>          : HookBase<HookId::NODE_CREATED, HookAction (const ApiDB_Node_Updater::node_t& new_node)> {};
template<> struct Hook<HookId::NODE_MODIFIED>         : HookBase<HookId::NODE_MODIFIED, HookAction (const ApiDB_Node_Updater::node_t& modified_node)> {};
template<> struct Hook<HookId::NODE_DELETED>          : HookBase<HookId::NODE_DELETED, HookAction (const ApiDB_Node_Updater::node_t& deleted_node)> {};
template<> struct Hook<HookId::WAY_CREATED>           : HookBase<HookId::WAY_CREATED, HookAction (const ApiDB_Way_Updater::way_t& node)> {};
template<> struct Hook<HookId::WAY_MODIFIED>          : HookBase<HookId::WAY_MODIFIED, HookAction (const ApiDB_Way_Updater::way_t& node)> {};
template<> struct Hook<HookId::WAY_DELETED>           : HookBase<HookId::WAY_DELETED, HookAction (const ApiDB_Way_Updater::way_t& node)> {};



// template<HookId hook>
// struct HookInstance
// {
//     static constexpr HookId hookId = hook;

//     using TCallback = typename Hook<hook>::CallbackFunc;

//     HookInstance(const TCallback* callback_func) : callback(callback_func) {}

//     template<typename... Args>
//     void operator()(Args... args) { callback(std::forward<Args>(args)...); }

//     TCallback* callback;
// };


template<HookId... hooks>
struct HookDefinitionsHelper
{
    // using HooksVariant = std::variant<HookInstance<hooks>...>;

    template<HookId h1, HookId... rest>
    struct hook_operations
    {
        static constexpr void register_callback(HookId h, void* callback)
        {
            if (h == h1)
            {
                Hook<h1>::register_callback(reinterpret_cast<typename Hook<h1>::CallbackFuncPtr>(callback));
            }
            else
            {
                if constexpr (sizeof...(rest) > 0)
                {
                    hook_operations<rest...>::register_callback(h, callback);
                }
                else
                {
                    throw std::runtime_error(fmt::format(FMT_COMPILE("Unsupported hook id: {}"), static_cast<std::underlying_type_t<HookId>>(h1)));
                }
            }
        }

        // static constexpr HooksVariant make_hook_variant(HookId h, void* callback)
        // {
        //     if (h == h1)
        //     {
        //         return {HookInstance<h1>{
        //             reinterpret_cast<typename Hook<h1>::CallbackFuncPtr>(callback)
        //         }};
        //     }
        //     else
        //     {
        //         if constexpr (sizeof...(rest) > 0)
        //         {
        //             return hook_operations<rest...>::make_hook_variant(h, callback);
        //         }
        //         else
        //         {
        //             throw std::runtime_error(fmt::format(FMT_COMPILE("Unsupported hook id: {}"), static_cast<std::underlying_type_t<HookId>>(h1)));
        //         }
        //     }
        // }
    };

    using hook_operations_t = hook_operations<hooks...>;
};

using HookDefinitions = HookDefinitionsHelper<
    HookId::POST_PLUGIN_LOAD, 
    HookId::POST_SOCKET_INIT,
    HookId::REQUEST_START,
    HookId::WRITE_REQUEST,
    HookId::CHANGESET_CREATE,
    HookId::CHANGESET_UPLOAD,
    HookId::NODE_CREATED,
    HookId::NODE_MODIFIED,
    HookId::NODE_DELETED,
    HookId::WAY_CREATED,
    HookId::WAY_MODIFIED,
    HookId::WAY_DELETED
>;

#endif
