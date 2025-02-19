#ifndef SENLUO_PRINCIPLE_HPP
#define SENLUO_PRINCIPLE_HPP

#include "../general.hpp"
#include "subtree.hpp"
#include "wrap.hpp"
#include "tag.hpp"
#include "pretreat.hpp"

#include "../macro_define.hpp"

namespace senluo
{
    namespace detail 
    {
        struct apply_t : adaptor<apply_t>
        {
            // Complex sfinae and noexcept are not currently provided.
            template<class Args, class Fn>
            constexpr decltype(auto) adapt(Args&& args, Fn&& fn)const
            {
                return [&]<size_t...I>(std::index_sequence<I...>) -> decltype(auto)
                {
                    //auto&& seperate_args = FWD(args) | refer | seperate;
                    return FWD(fn)(subtree<I>(FWD(args))...);
                }(std::make_index_sequence<size<Args>>{});
            }
        };
        
        struct apply_invoke_t : adaptor_closure<apply_invoke_t>
        {
            // Complex sfinae and noexcept are not currently provided.
            template<class T>
            constexpr decltype(auto) operator()(T&& tree)const
            {
                return [&]<size_t...I>(std::index_sequence<I...>) -> decltype(auto)
                {
                    //auto&& seperate_tree = FWD(tree) | refer | seperate;
                    return subtree<0uz>(FWD(tree))(subtree<I + 1>(FWD(tree))...);
                }(std::make_index_sequence<size<T> - 1uz>{});
            }
        };
    }

    inline constexpr detail::apply_t apply{};

    inline constexpr detail::apply_invoke_t apply_invoke{};
}

namespace senluo
{
    // namespace detail
    // {
    //     template<auto UsageTree, template<class...> class Tpl>
    //     struct plainize_fn : adaptor_closure<plainize_fn<UsageTree, Tpl>>
    //     {
    //         // Complex sfinae and noexcept are not currently provided.
    //         template<class T>
    //         constexpr decltype(auto) operator()(T&& tree) const
    //         {
    //             return impl(FWD(tree) | sequence_by_usage<detail::unfold_tag_tree(UsageTree, shape<T>)>);
    //         }

    //         template<class TSrc>
    //         constexpr decltype(auto) impl(TSrc&& src)const
    //         {
    //             if constexpr(detail::fold_usage(UsageTree) == usage_t::none)
    //             {
    //                 return tuple{};
    //             }
    //             else if constexpr(terminal<TSrc>)
    //             {
    //                 return (TSrc)FWD(src);
    //             }
    //             else return [&]<size_t...I>(std::index_sequence<I...>)
    //             {
    //                 return tuple<decltype(plainize_fn<detail::tag_tree_get<I>(UsageTree), Tpl>{}.impl(subtree<I>(FWD(src))))...>
    //                 {
    //                     plainize_fn<detail::tag_tree_get<I>(UsageTree), Tpl>{}.impl(subtree<I>(FWD(src)))...
    //                 };
    //             }(std::make_index_sequence<size<TSrc>>{});
    //         }
    //     };
    // }
    
    // template<usage_tree_liked auto UsageTree = usage_t::once, template<class...> class Tpl = tuple>
    // inline constexpr detail::plainize_fn<UsageTree, Tpl> plainize{};
    
    // template<template<class...> class Tpl = tuple>
    // constexpr const auto& to()
    // {
    //     return plainize<usage_t::once, Tpl>;
    // }

    // template<class T>
    // struct plain_principle;

    // namespace detail
    // {
    //     template<auto UsageTree, template<class...> class Tpl>
    //     struct plainize_principle_fn : adaptor_closure<plainize_principle_fn<UsageTree, Tpl>>
    //     {
    //         template<class T>
    //         constexpr auto operator()(T&& tree)const
    //         AS_EXPRESSION(
    //             plain_principle<unwrap_t<decltype(FWD(tree) | plainize<UsageTree, Tpl>)>>{ 
    //                 unwrap_fwd(FWD(tree) | plainize<UsageTree, Tpl>)
    //             }
    //         )
    //     };
    // };

    // template<usage_tree_liked auto UsageTree = usage_t::once, template<class...> class Tpl = tuple>
    // inline constexpr detail::plainize_principle_fn<UsageTree, Tpl> plainize_principle{};
}

namespace senluo
{        
    // template<typename Principle>
    // struct detail::principle_interface : standard_interface<Principle>
    // {
    //     static consteval auto data_shape()
    //     {
    //         return shape<decltype(data(std::declval<Principle&>()))>; 
    //     }

    //     static consteval auto folded_layout()
    //     { 
    //         return detail::fold_layout<Principle::layout()>(data_shape());
    //     }
        
    //     static consteval auto folded_stricture_tree()
    //     { 
    //         return detail::fold_tag_tree<Principle::stricture_tree()>(); 
    //     }
        
    //     // static consteval auto folded_operation_tree()
    //     // { 
    //     //     return detail::fold_operation_tree<Principle::operation_tree()>(); 
    //     // }
        
    //     // Complex sfinae and noexcept are not currently provided.
    //     template<size_t I, unwarp_derived_from<Principle> Self>
    //     friend constexpr decltype(auto) subtree(Self&& self)
    //     {
    //         decltype(auto) tree = wrapper<decltype(data(FWD(self)))>{ data(FWD(self)) }
    //             | relayout<Principle::layout()>
    //             | astrict<Principle::stricture_tree()>
    //             //| operate<Principle::operation_tree()>
    //         ;
    //         if constexpr(I >= size<decltype(tree)>)
    //         {
    //             return end();
    //         }
    //         else
    //         {
    //            return FWD(tree) | subtree<I>; 
    //         }
            
    //     }

    //     template<auto UsageTree, unwarp_derived_from<Principle> Self>
    //     friend constexpr decltype(auto) principle(Self&& self) noexcept
    //     {
    //         return FWD(self);
    //         // constexpr auto fitted_usage_result = detail::fit_operation_usage<folded_operation_tree()>(UsageTree);
    //         // constexpr auto fittedd_usage = fitted_usage_result.usage_tree;
    //         // constexpr bool need_plain = fitted_usage_result.need_plain;

    //         // if constexpr(need_plain)
    //         // {
    //         //     return FWD(self) | plainize_principle<UsageTree>;
    //         // }
    //         // else
    //         // {
    //         //     return FWD(self);
    //         // }
    //     }
    // };
    
    // struct null_principle : principle_interface<null_principle>
    // {
    //     friend constexpr auto data(unwarp_derived_from<null_principle> auto&& self) noexcept
    //     {
    //         return std::in_place_t{};
    //     }
        
    //     static consteval auto layout(){ return indexes_of_whole; }
        
    //     static consteval auto stricture_tree(){ return stricture_t::none; }
        
    //     //static consteval auto operation_tree(){ return operation_t::none; }
    // };

    // template<class T>
    // struct plain_principle : detail::based_on<T>, principle_interface<plain_principle<T>>
    // {
    //     friend constexpr auto data(unwarp_derived_from<plain_principle> auto&& self)
    //     AS_EXPRESSION(
    //         unwrap(FWD(self) | detail::base)
    //     )
        
    //     static consteval auto layout(){ return indexes_of_whole; }
        
    //     static consteval auto stricture_tree(){ return stricture_t::none; }
        
    //     //static consteval auto operation_tree(){ return operation_t::none; }
    // };

    // template<class T, auto UsageTree>
    // struct trivial_principle : detail::based_on<T>, principle_interface<trivial_principle<T, UsageTree>>
    // {        
    //     // Complex sfinae and noexcept are not currently provided.
    //     friend constexpr auto data(unwarp_derived_from<trivial_principle> auto&& self)
    //     {
    //         return [&]<size_t...I>(std::index_sequence<I...>)
    //         {
    //             return tuple<decltype(data(FWD(self) | detail::base | subtree<I> | principle<detail::tag_tree_get<I>(UsageTree)>))...>
    //             {
    //                 data(FWD(self) | detail::base | subtree<I> | principle<detail::tag_tree_get<I>(UsageTree)>)...
    //             };
    //         }(std::make_index_sequence<size<T>>{});
    //     }

    //     static consteval auto layout()
    //     {
    //         return []<size_t...I>(std::index_sequence<I...>)
    //         {
    //             return make_tuple(
    //                 detail::layout_add_prefix(principle_t<subtree_t<T, I>, detail::tag_tree_get<I>(UsageTree)>::layout(), array{ I })...
    //             );
    //         }(std::make_index_sequence<size<T>>{});
    //     }

    //     static consteval auto stricture_tree()
    //     {
    //         return []<size_t...I>(std::index_sequence<I...>)
    //         {
    //             return make_tuple(
    //                 principle_t<subtree_t<T, I>, detail::tag_tree_get<I>(UsageTree)>::stricture_tree()...
    //             );
    //         }(std::make_index_sequence<size<T>>{});
    //     }

    //     // static consteval auto operation_tree()
    //     // {
    //     //     return []<size_t...I>(std::index_sequence<I...>)
    //     //     {
    //     //         return make_tuple(
    //     //             principle_t<subtree_t<T, I>, detail::tag_tree_get<I>(UsageTree)>::operation_tree()...
    //     //         );
    //     //     }(std::make_index_sequence<size<T>>{});
    //     // }
    // };

    // template<auto UsageTree>
    // struct detail::principle_t_ns::principle_fn : adaptor_closure<principle_fn<UsageTree>>
    // {
    //     // Complex sfinae and noexcept are not currently provided.
    //     template<typename T>
    //     constexpr auto operator()(T&& tree)const
    //     {
    //         if constexpr(detail::equal(UsageTree, usage_t::none))
    //         {
    //             return null_principle{};
    //         }
    //         else if constexpr(requires{ { FWD(tree).template principle<UsageTree>() } -> principled; })
    //         {
    //             return FWD(tree).template principle<UsageTree>();
    //         }
    //         else if constexpr(requires{ { principle<UsageTree>(FWD(tree)) } -> principled; })
    //         {
    //             return principle<UsageTree>(FWD(tree));
    //         }
    //         else if constexpr(plain<T, UsageTree>)
    //         {                
    //             return plain_principle<unwrap_t<T>>{ unwrap_fwd(FWD(tree)) };
    //         }
    //         else
    //         { 
    //             return trivial_principle<unwrap_t<T>, UsageTree>{ unwrap_fwd(FWD(tree)) };
    //         }
    //     }
    // };
}

#include "../macro_undef.hpp"
#endif