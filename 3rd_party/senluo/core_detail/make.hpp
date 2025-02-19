#ifndef SENLUO_MAKE_HPP
#define SENLUO_MAKE_HPP

#include "../general.hpp"
#include "subtree.hpp"
#include "wrap.hpp"
#include "pretreat.hpp"
#include "relayout.hpp"

#include "../macro_define.hpp"

namespace senluo 
{
    
    
    template<typename T>
    struct sequence_maker
    {
        template<typename Arg>
        constexpr T operator()(Arg&& arg)const
        {
            return [&]<size_t...I>(std::index_sequence<I...>)
            {
                //auto&& seq = FWD(arg) | refer | sequence;
                return T{ FWD(arg) | make<std::tuple_element_t<I, T>, I>... };
            }(std::make_index_sequence<std::tuple_size_v<T>>{});
        }
    };

    template<typename T>
    struct inverse_sequence_maker
    {
        template<typename Arg>
        constexpr T operator()(Arg&& arg)const
        {
            return [&]<size_t...I>(std::index_sequence<I...>)
            {
                //auto&& seq = FWD(arg) | refer | inverse_sequence;
                //constexpr size_t last_index = std::tuple_size_v<T> - 1uz;
                return T{ FWD(arg) | make<std::tuple_element_t<I, T>, I>... };
            }(std::make_index_sequence<std::tuple_size_v<T>>{});
        }
    };

    template<typename T>
    struct children_maker
    {
        template<typename Arg>
        constexpr T operator()(Arg&& arg)const
        {
            return [&]<size_t...I>(std::index_sequence<I...>)
            {
                //auto&& seq = FWD(arg) | refer | seperate;
                return T{ FWD(arg) | make<std::tuple_element_t<I, T>, I>... };
            }(std::make_index_sequence<std::tuple_size_v<T>>{});
        }
    };

    template<typename T>
    struct aggregate_maker
    {
        template<typename Arg>
        constexpr T operator()(Arg&& arg)const
        {
            return [&]<size_t...I>(std::index_sequence<I...>)
            {
               // auto&& seq = FWD(arg) | refer | sequence;
                return T{ { FWD(arg) | make<detail::aggregate_member_t<I, T>, I> }... };
            }(std::make_index_sequence<size<T>>{});
        }
    };

    namespace detail
    {
        template<class T>
        constexpr bool is_aggregate_tree()
        {
            return []<size_t...I>(std::index_sequence<I...>) -> bool
            {
                return (... && (detail::get_ns::get_t<I>::template choose<T&>().strategy == detail::get_ns::strategy_t::aggregate));
            }(std::make_index_sequence<size<T>>{});
        }
    }

    template<class T>
    concept aggregate_tree = std::is_aggregate_v<std::remove_cvref_t<T>> && detail::is_aggregate_tree<T>();

    namespace detail::make_t_ns
    {        
        // template<typename T>
        // constexpr auto get_maker(type_tag<T>)noexcept
        // {
        //     if constexpr(aggregate_tree<T>)
        //     {
        //         return aggregate_maker<T>{};
        //     }
        //     else if constexpr(requires{ std::tuple_size<T>::value; })
        //     {
        //         return children_maker<T>{};
        //     }
        //     else
        //     {

        //     }
        // }
    }

    template<typename T, auto indexes>
    struct detail::make_t_ns::make_t : adaptor_closure<make_t<T, indexes>>
    {
        template<typename Arg>
        constexpr T operator()(Arg&& arg)const
        {
            if constexpr(wrapped<T>)
            {
                return T{ make<unwrap_t<T>>(FWD(arg)) };
            }
            else if constexpr(terminal<subtree_t<Arg, indexes>> || terminal<T>)
            {
                return (T)unwrap_fwd(subtree<indexes>(FWD(arg)));
            }
            else if constexpr(std::same_as<std::remove_cvref_t<subtree_t<Arg, indexes>>, T> && requires{ T{ subtree<indexes>(FWD(arg)) }; })
            {
                return (T)subtree<indexes>(FWD(arg));
            }
            else if constexpr(requires{ get_maker(type_tag<T>{}); })
            {
                return get_maker(type_tag<T>{})(subtree<indexes>(FWD(arg)));
            }
            else if constexpr(aggregate_tree<T>)
            {
                return aggregate_maker<T>{}(subtree<indexes>(FWD(arg)));
            }
            else if constexpr(requires{ std::tuple_size<T>::value; })
            {
                return children_maker<T>{}(subtree<indexes>(FWD(arg)));
            }
            else
            {
                static_assert(false, "maker for T not found.");
            }
        }
    };
}

namespace senluo 
{
    template<class To>
    constexpr const auto& to()
    {
        return make<To>;
    }
}

namespace senluo 
{
    template<class...T>
    constexpr auto get_maker(type_tag<tuple<T...>>)
    {
        return sequence_maker<tuple<T...>>{};
    }

    template<class...T>
    constexpr auto get_maker(type_tag<std::tuple<T...>>)
    {
        return inverse_sequence_maker<std::tuple<T...>>{};
    }

    template<class T, size_t N>
    constexpr auto get_maker(type_tag<array<T, N>>)
    {
        return sequence_maker<array<T, N>>{};
    }
}

#include "../macro_undef.hpp"
#endif