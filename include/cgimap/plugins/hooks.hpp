/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is part of openstreetmap-cgimap (https://github.com/zerebubuth/openstreetmap-cgimap/).
 *
 * Copyright (C) 2009-2023 by the CGImap developer community.
 * For a full list of authors see the git log.
 */

#ifndef CGIMAP_HOOKS_HPP
#define CGIMAP_HOOKS_HPP

#include <iostream>
#include <vector>
#include <variant>
#include <type_traits>
#include <functional>

#include <fmt/core.h>
#include <fmt/compile.h>

#include "hook.hpp"

namespace Hooks
{

    inline std::vector<HookDefinitions::HooksVariant> hooks;

    extern "C" void register_callback_c(Hook hook, void* callback_func);
    // {
    //     hooks.emplace_back(HookDefinitions::hook_operations_t::make_hook_variant(hook, callback_func));
    // }

    template<Hook h>
    void register_callback(typename HookTraits<h>::CallbackFuncPtr callback_func)
    {
        register_callback_c(h, reinterpret_cast<void*>(callback_func));
    }
    
    template<Hooks::Hook hook, typename... Args> //, std::enable_if_t<std::is_invocable_v<typename Tag::Func, Args...>, bool> = true>
    Hooks::HookAction call(Args&&... args)
    {
        for (auto& h : hooks)
        {
            if (std::holds_alternative<Hooks::HookInstance<hook>>(h))
            {
                std::cout << "Calling hook function\n";
                auto hook_action = std::visit([&](auto&& arg)
                {
                    
                    using THook = std::decay_t<decltype(arg)>;
                    if constexpr (THook::hookId == hook)
                    {
                        static_assert(std::is_invocable_v<decltype(arg.callback), Args...>, "Non-invokable hook callback function");

                        using res_t = std::invoke_result_t<decltype(arg.callback), Args...>;
                        
                        if constexpr (!std::is_same_v<res_t, void>)
                        {
                            auto retval = std::invoke(arg.callback, std::forward<Args>(args)...);
                            return retval;
                        }
                        else
                        {
                            std::invoke(arg.callback, std::forward<Args>(args)...);
                            return HookAction::CONTINUE;
                        }
                    }

                    return HookAction::CONTINUE;
                }, h);

                if (hook_action != HookAction::CONTINUE)
                {
                    fmt::print(FMT_COMPILE("Hook for {} requested action: {}\n"), static_cast<std::underlying_type_t<Hook>>(hook), static_cast<std::underlying_type_t<HookAction>>(hook_action));
                    return hook_action;
                }
            }
        }

        return HookAction::CONTINUE;
    }
};

class HookRegistry {
public:

// private:
    
};

#endif
