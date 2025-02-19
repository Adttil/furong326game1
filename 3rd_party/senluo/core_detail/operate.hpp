#ifndef SENLUO_OPERATE_HPP
#define SENLUO_OPERATE_HPP

#include "../general.hpp"
#include "principle.hpp"
#include "wrap.hpp"
#include "subtree.hpp"
#include "relayout.hpp"

#include "../macro_define.hpp"

namespace senluo::detail
{
    template<auto OperationTree, class T>
    constexpr decltype(auto) operate_impl(T&& data)
    {
        if constexpr(std::same_as<decltype(OperationTree), operation_t>)
        {
            if constexpr(OperationTree == operation_t::none)
            {
                return (T)FWD(data);
            }
            else// if constexpr(OperationTree == operation_t::apply_invoke)
            {
                return apply_invoke(FWD(data));
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return tuple<decltype(detail::operate_impl<get<I>(OperationTree)>(subtree<I>(FWD(data))))...>{
                detail::operate_impl<get<I>(OperationTree)>(subtree<I>(FWD(data)))...
            };
        }(std::make_index_sequence<size<decltype(OperationTree)>>{});
    }
}

namespace senluo 
{
    template<auto OperationTree>
    struct detail::operate_t : adaptor_closure<operate_t<OperationTree>>
    {
        // Complex sfinae and noexcept are not currently provided.
        template<typename T>
        constexpr decltype(auto) operator()(T&& t)const
        {
            //constexpr auto usage_tree = make_tree_of_same_value(usage_t::once, shape<decltype(OperationTree)>);
            //decltype(auto) data = FWD(t) | sequence_by_usage<usage_tree>;

            return decltype(detail::operate_impl<OperationTree>(FWD(t)) | wrap)
            {
                detail::operate_impl<OperationTree>(FWD(t))
            };
            // if constexpr(not std::same_as<decltype(OperationTree), operation_t>)
            // {
            //     return operate_ns::tree_t<senluo::unwrap_t<T>, OperationTree>{ unwrap_fwd(FWD(t)) };
            // }
            // else if constexpr(OperationTree == operation_t::none)
            // {
            //     return decltype(wrap(FWD(t))){ unwrap_fwd(FWD(t)) };
            // }
            // else
            // {
            //     return decltype(wrap(apply_invoke(FWD(t)))){ 
            //         unwrap_fwd(apply_invoke(FWD(t))) 
            //     };
            // }
        }
    };
}

namespace senluo
{
    namespace detail
    {
        struct transform_t : adaptor<transform_t>
        {
            // Complex sfinae and noexcept are not currently provided.
            template<class ArgTable, class Fn>
            constexpr decltype(auto) adapt(ArgTable&& arg_table, Fn&& fn)const
            {
                constexpr size_t n = size<ArgTable>;
                return zip(FWD(fn) | repeat<n>, FWD(arg_table)) 
                    | operate<detail::make_tree_of_same_value(operation_t::apply_invoke, shape<array<size_t, n>>)>;
            }
        };
    }
    
    inline constexpr detail::transform_t transform{};
    
    namespace detail
    {
        struct zip_transform_t
        {
            // Complex sfinae and noexcept are not currently provided.
            template<class Fn, class Args, class...Rest>
            constexpr decltype(auto) operator()(Fn&& fn, Args&& args, Rest&&...rest)const
            {
                constexpr size_t n = size<Args>;
                return zip(FWD(fn) | repeat<n>, FWD(args), FWD(rest)...) 
                    | operate<detail::make_tree_of_same_value(operation_t::apply_invoke, shape<array<size_t, n>>)>;
            }
        };
    }

    inline constexpr detail::zip_transform_t zip_transform{};
}

#include "../macro_undef.hpp"
#endif