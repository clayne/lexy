// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_ACTION_BASE_HPP_INCLUDED
#define LEXY_ACTION_BASE_HPP_INCLUDED

#include <lexy/_detail/config.hpp>
#include <lexy/_detail/lazy_init.hpp>
#include <lexy/_detail/type_name.hpp>
#include <lexy/callback/noop.hpp>
#include <lexy/dsl/base.hpp>
#include <lexy/grammar.hpp>

//=== parse_context ===//
namespace lexy
{
namespace _detail
{
    struct parse_context_var_base
    {
        const void*             id;
        parse_context_var_base* next;

        constexpr parse_context_var_base(const void* id) : id(id), next(nullptr) {}

        template <typename Context>
        constexpr void link(Context& context)
        {
            auto cb  = context.control_block;
            next     = cb->vars;
            cb->vars = this;
        }

        template <typename Context>
        constexpr void unlink(Context& context)
        {
            auto cb  = context.control_block;
            cb->vars = next;
        }
    };

    template <typename Id, typename T>
    struct parse_context_var : parse_context_var_base
    {
        static constexpr auto type_id = lexy::_detail::type_id<Id>();

        T value;

        explicit constexpr parse_context_var(T&& value)
        : parse_context_var_base(&type_id), value(LEXY_MOV(value))
        {}

        template <typename Context>
        static constexpr T& get(Context& context)
        {
            auto cb = context.control_block;

            for (auto cur = cb->vars; cur; cur = cur->next)
                if (cur->id == &type_id)
                    return static_cast<parse_context_var*>(cur)->value;

            LEXY_ASSERT(false, "context variable hasn't been created");
            return LEXY_DECLVAL(T&);
        }
    };

    template <typename Handler>
    struct parse_context_control_block
    {
        Handler                 parse_handler;
        parse_context_var_base* vars;
        int                     cur_depth, max_depth;
        bool                    enable_whitespace_skipping;

        constexpr parse_context_control_block(Handler&& handler, std::size_t max_depth)
        : parse_handler(LEXY_MOV(handler)), vars(nullptr), cur_depth(0),
          max_depth(static_cast<int>(max_depth)), enable_whitespace_skipping(true)
        {}
    };
} // namespace _detail

template <typename Handler, typename Production, typename RootProduction = Production>
struct _pc
{
    using production      = Production;
    using root_production = RootProduction;
    using value_type      = typename Handler::template value_callback<Production>::return_type;

    typename Handler::template event_handler<Production> handler;
    _detail::parse_context_control_block<Handler>*       control_block;
    _detail::lazy_init<value_type>                       value;

    constexpr explicit _pc(_detail::parse_context_control_block<Handler>* cb) : control_block(cb) {}

    template <typename ChildProduction>
    constexpr auto sub_context(ChildProduction)
    {
        // Update the root production if necessary.
        using new_root = std::conditional_t<lexy::is_token_production<ChildProduction>,
                                            ChildProduction, RootProduction>;
        return _pc<Handler, ChildProduction, new_root>(control_block);
    }

    constexpr auto value_callback()
    {
        using callback = typename Handler::template value_callback<Production>;
        return callback(control_block->parse_handler);
    }

    template <typename Event, typename... Args>
    constexpr void on(Event ev, Args&&... args)
    {
        handler.on(control_block->parse_handler, ev, LEXY_FWD(args)...);
    }
};
} // namespace lexy

//=== do_action ===//
namespace lexy::_detail
{
struct final_parser
{
    template <typename Context, typename Reader, typename... Args>
    LEXY_PARSER_FUNC static bool parse(Context& context, Reader&, Args&&... args)
    {
        context.value.emplace_result(context.value_callback(), LEXY_FWD(args)...);
        return true;
    }
};

template <typename NextParser>
struct context_finish_parser
{
    template <typename Context, typename Reader, typename SubContext, typename... Args>
    LEXY_PARSER_FUNC static bool parse(Context& context, Reader& reader, SubContext& sub_context,
                                       Args&&... args)
    {
        // Might need to skip whitespace, according to the original context.
        using continuation
            = std::conditional_t<lexy::is_token_production<typename SubContext::production>,
                                 lexy::whitespace_parser<Context, NextParser>, NextParser>;

        // Pass the produced value to the next parser.
        if constexpr (std::is_void_v<typename SubContext::value_type>)
            return continuation::parse(context, reader, LEXY_FWD(args)...);
        else
            return continuation::parse(context, reader, LEXY_FWD(args)...,
                                       LEXY_MOV(*sub_context.value));
    }
};
} // namespace lexy::_detail

namespace lexy
{
template <typename Production, typename Handler, typename Reader>
constexpr auto do_action(Handler&& handler, Reader& reader)
{
    static_assert(!std::is_reference_v<Handler>, "need to move handler in");

    _detail::parse_context_control_block control_block(LEXY_MOV(handler),
                                                       max_recursion_depth<Production>());
    _pc<Handler, Production>             context(&control_block);

    context.on(parse_events::production_start{}, reader.position());

    using parser     = lexy::parser_for<lexy::production_rule<Production>, _detail::final_parser>;
    auto rule_result = parser::parse(context, reader);

    if (rule_result)
        context.on(parse_events::production_finish{}, reader.position());
    else
        context.on(parse_events::production_cancel{}, reader.position());

    using value_type = typename decltype(context)::value_type;
    if constexpr (std::is_void_v<value_type>)
        return LEXY_MOV(control_block.parse_handler).get_result_void(rule_result);
    else if (context.value)
        return LEXY_MOV(control_block.parse_handler)
            .template get_result<value_type>(rule_result, LEXY_MOV(*context.value));
    else
        return LEXY_MOV(control_block.parse_handler).template get_result<value_type>(rule_result);
}
} // namespace lexy

//=== value callback ===//
namespace lexy::_detail
{
struct _void_value_callback
{
    constexpr _void_value_callback() = default;
    template <typename Handler>
    constexpr explicit _void_value_callback(Handler&)
    {}

    using return_type = void;

    constexpr auto sink() const
    {
        return lexy::noop.sink();
    }

    template <typename... Args>
    constexpr void operator()(Args&&...) const
    {}
};
template <typename Production>
using void_value_callback = _void_value_callback; // to prevent unnecessary instantiations
} // namespace lexy::_detail

#endif // LEXY_ACTION_BASE_HPP_INCLUDED

