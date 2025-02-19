#ifndef SENLUO_PRETREAT_HPP
#define SENLUO_PRETREAT_HPP

#include "../general.hpp"
#include "subtree.hpp"
#include "wrap.hpp"
#include "tag.hpp"

#include "../macro_define.hpp"

namespace senluo
{
    namespace detail
    {
        template<auto Layout>
        struct relayout_t;

        // template<auto StrictureTree>
        // struct astrict_t;

        template<auto OperationTree>
        struct operate_t;

        template<auto Constexpr>
        consteval auto pass_constexpr()
        {
            return Constexpr;
        }

        template<class T>
        constexpr bool is_layout()
        {
            if constexpr(indexical<T>)
            {
                return true;
            }
            else return []<size_t...I>(std::index_sequence<I...>)
            {
                return (... && detail::is_layout<subtree_t<T&, I>>());
            }(std::make_index_sequence<size<T>>{});
        }

        template<class T, class TTag>
        constexpr bool is_tag_tree()
        {
            if constexpr(std::same_as<std::remove_cvref_t<T>, TTag>)
            {
                return true;
            }
            else return []<size_t...I>(std::index_sequence<I...>)
            {
                return (... && detail::is_tag_tree<subtree_t<T&, I>, TTag>());
            }(std::make_index_sequence<size<T>>{});
        }
    }

    template<class T>
    concept indexes_tree_liked = detail::is_layout<T>();

    template<class T>
    concept stricture_tree_liked = detail::is_tag_tree<T, stricture_t>();

    template<class T>
    concept operation_tree_liked =  detail::is_tag_tree<T, operation_t>();

    template<indexes_tree_liked auto Layout>
    inline constexpr detail::relayout_t<Layout> relayout{};

    // template<stricture_tree_liked auto StrictureTree>
    // inline constexpr detail::astrict_t<detail::fold_tag_tree<StrictureTree>()> astrict{};

    template<operation_tree_liked auto OperationTree>
    inline constexpr detail::operate_t<detail::fold_operation_tree<OperationTree>()> operate{};
}

namespace senluo 
{
//     namespace detail
//     {
//         template<typename PrinciledTree>
//         struct principle_interface;
//     }
//     using detail::principle_interface;

//     template<typename T>
//     concept principled = requires(std::remove_cvref_t<unwrap_t<T>> t)
//     {
//         { []<class U>(principle_interface<U>&)->U*{}(t) } -> std::same_as<decltype(t)*>;
//         data(std::declval<T>());
//         { detail::pass_constexpr<decltype(t)::layout()>() } -> indexes_tree_liked;
//         { detail::pass_constexpr<decltype(t)::stricture_tree()>() } -> stricture_tree_liked;
//         //{ detail::pass_constexpr<decltype(t)::operation_tree()>() } -> operation_tree_liked;
//     };
    
//     namespace detail::principle_t_ns 
//     {
//         template<auto UsageTree>
//         struct principle_fn;

//         template<auto UsageTree>
//         void principle();

//         template<class T, auto UsageTree>
//         constexpr bool is_plain()
//         {
//             constexpr bool no_custom = not bool
//             {
//                 requires{ std::declval<unwrap_t<T>>().template principle<UsageTree>(); }
//                 ||
//                 requires{ principle<UsageTree>(std::declval<unwrap_t<T>>()); }
//             };
//             return [&]<size_t...I>(std::index_sequence<I...>)
//             {
//                 return (no_custom && ... && is_plain<subtree_t<T, I>, detail::tag_tree_get<I>(UsageTree)>());
//             }(std::make_index_sequence<size<T>>{});
//         }
//     }
    
     template<class T>
     concept usage_tree_liked =  detail::is_tag_tree<T, usage_t>();

//     template<usage_tree_liked auto UsageTree>
//     inline constexpr detail::principle_t_ns::principle_fn<UsageTree> principle;
    
//     template<typename T, usage_tree_liked auto UsageTree>
//     using principle_t = decltype(std::declval<T>() | principle<UsageTree>);

//     template<class T, auto UsageTree>
//     concept plain = usage_tree_liked<decltype(UsageTree)> && (detail::principle_t_ns::is_plain<T, UsageTree>());

//     template<class Pretreater>
//     struct pretreater_interface : adaptor_closure<Pretreater>
//     {
//         constexpr decltype(auto) operator()(auto&& tree) const
//         noexcept(noexcept(detail::pass(data(FWD(tree) | senluo::principle<Pretreater::usage_tree()>))))
//         requires requires{detail::pass(data(FWD(tree) | senluo::principle<Pretreater::usage_tree()>));}
//         {
//             using principle_result_t = decltype(FWD(tree) | senluo::principle<Pretreater::usage_tree()>);
//             using principle_t = std::remove_cvref_t<unwrap_t<principle_result_t>>;
//             using data_t = decltype(data(std::declval<principle_result_t>()));
            
//             constexpr auto layout = principle_t::layout();
//             constexpr auto raw_stricture_tree = principle_t::stricture_tree();
//             constexpr auto stricture_tree = Pretreater::template pretreat_stricture<raw_stricture_tree, layout>(shape<data_t>);
//             //constexpr auto operation_tree = principle_t::operation_tree();

//             if constexpr(std::is_rvalue_reference_v<data_t>)
//             {
//                 return decltype(std::declval<data_t>() | refer | relayout<layout> | astrict<stricture_tree>)
//                 {
//                     data(FWD(tree) | senluo::principle<Pretreater::usage_tree()>)
//                 };
//             }
//             else
//             {
//                 return decltype(std::declval<data_t>() | relayout<layout> | astrict<stricture_tree>)
//                 {
//                     data(FWD(tree) | senluo::principle<Pretreater::usage_tree()>)
//                 };
//             }
//         }
//     };

//     namespace detail
//     {
//         template<auto UsageTree>
//         struct sequence_by_usage_t : pretreater_interface<sequence_by_usage_t<UsageTree>>
//         {
//             static consteval auto usage_tree()
//             {
//                 return UsageTree;
//             }

//             template<auto RawStrictureTree, auto Layout, class S>
//             static consteval auto pretreat_stricture(S data_shape)
//             {
//                 return detail::get_sequence_stricture_tree<RawStrictureTree, Layout, UsageTree>(data_shape);
//             }
//         };

//         template<auto UsageTree>
//         struct inverse_sequence_by_usage_t : pretreater_interface<inverse_sequence_by_usage_t<UsageTree>>
//         {
//             static consteval auto usage_tree()
//             {
//                 return UsageTree;
//             }

//             template<auto RawStrictureTree, auto Layout, class S>
//             static consteval auto pretreat_stricture(S data_shape)
//             {
//                 return detail::get_inverse_sequence_stricture_tree<RawStrictureTree, Layout, UsageTree>(data_shape);
//             }
//         };

//         template<auto UsageTree>
//         struct seperate_by_usage_t : pretreater_interface<seperate_by_usage_t<UsageTree>>
//         {
//             static consteval auto usage_tree()
//             {
//                 return UsageTree;
//             }

//             template<auto RawStrictureTree, auto Layout, class S>
//             static consteval auto pretreat_stricture(S data_shape)
//             {
//                 constexpr auto seq_stricture_tree = detail::get_sequence_stricture_tree<RawStrictureTree, Layout, UsageTree>(S{});
//                 constexpr auto inv_stricture_tree = detail::get_inverse_sequence_stricture_tree<RawStrictureTree, Layout, UsageTree>(S{});

//                 return detail::merge_stricture_tree(seq_stricture_tree, inv_stricture_tree);
//             }
//         };
//     }

//     template<usage_tree_liked auto UsageTree>
//     inline constexpr detail::sequence_by_usage_t<UsageTree> sequence_by_usage{};

//     template<usage_tree_liked auto UsageTree>
//     inline constexpr detail::inverse_sequence_by_usage_t<UsageTree> inverse_sequence_by_usage{};

//     template<usage_tree_liked auto UsageTree>
//     inline constexpr detail::seperate_by_usage_t<UsageTree> seperate_by_usage{};

//     template<class Tree>
//     inline constexpr detail::sequence_by_usage_t<detail::make_tree_of_same_value(usage_t::once, shape<Tree>)> sequence_by_shape{};

//     template<class Tree>
//     inline constexpr detail::inverse_sequence_by_usage_t<detail::make_tree_of_same_value(usage_t::once, shape<Tree>)> inverse_sequence_by_shape{};

//     template<class Tree>
//     inline constexpr detail::seperate_by_usage_t<detail::make_tree_of_same_value(usage_t::once, shape<Tree>)> seperate_by_shape{};

//     namespace detail
//     {
//         struct sequence_t : adaptor_closure<sequence_t>
//         {
//             template<branched T>
//             constexpr auto operator()(T&& tree)const
//             AS_EXPRESSION(
//                 FWD(tree) | sequence_by_usage<detail::make_tree_of_same_value(usage_t::once, shape<array<int, size<T>>>)>
//             )

//             template<terminal T>
//             constexpr tuple<> operator()(T&& t)const noexcept
//             {
//                 return {};
//             }
//         };

//         struct inverse_sequence_t : adaptor_closure<inverse_sequence_t>
//         {
//             template<branched T>
//             constexpr auto operator()(T&& tree)const
//             AS_EXPRESSION(
//                 FWD(tree) | inverse_sequence_by_usage<detail::make_tree_of_same_value(usage_t::once, shape<array<int, size<T>>>)>
//             )

//             template<terminal T>
//             constexpr tuple<> operator()(T&& t)const noexcept
//             {
//                 return {};
//             }
//         };

//         struct seperate_t : adaptor_closure<seperate_t>
//         {
//             template<branched T>
//             constexpr auto operator()(T&& tree)const
//             AS_EXPRESSION(
//                 FWD(tree) | seperate_by_usage<detail::make_tree_of_same_value(usage_t::once, shape<array<int, size<T>>>)>
//             )

//             template<terminal T>
//             constexpr tuple<> operator()(T&& t)const noexcept
//             {
//                 return {};
//             }
//         };
//     }

//     inline constexpr detail::sequence_t sequence{};
    
//     inline constexpr detail::inverse_sequence_t inverse_sequence{};

//     inline constexpr detail::seperate_t seperate{};
}

#include "../macro_undef.hpp"
#endif