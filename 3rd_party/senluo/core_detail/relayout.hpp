#ifndef SENLUO_RELAYOUT_HPP
#define SENLUO_RELAYOUT_HPP

#include "../general.hpp"
#include "subtree.hpp"
#include "principle.hpp"
#include "wrap.hpp"
//#include "make.hpp"

#include "../macro_define.hpp"

namespace senluo::detail
{
    template<auto Layout, class Shape>
    constexpr auto unfold_layout(Shape shape = {})
    {
        if constexpr(indexical<decltype(Layout)>)
        {
            constexpr auto indexes = detail::normalize_indices<Layout>(Shape{});
            using subshape_t = subtree_t<Shape, indexes>;
            if constexpr(terminal<subshape_t>)
            {
                return indexes;
            }
            else return [&]<size_t...I>(std::index_sequence<I...>)
            {
                constexpr auto indexes = detail::normalize_indices<Layout>(Shape{});
                return make_tuple(detail::unfold_layout<detail::array_cat(indexes, array{ I })>(shape)...);
            }(std::make_index_sequence<size<subshape_t>>{});
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::unfold_layout<get<I>(Layout)>(shape)...);
        }(std::make_index_sequence<size<decltype(Layout)>>{});
    }

    //unfold_layout<layout, shape>() == unfold_layout_by_relayouted_shape(layout, apply_layout<layout>(shape{}))
    template<class Shape, class Layout>
    constexpr auto unfold_layout_by_relayouted_shape(const Layout& layout, Shape shape = {})
    {
        if constexpr(terminal<Shape>)
        {
            static_assert(detail::indexical_array<Layout>, "Invalid layout.");
            return layout;
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            if constexpr(not detail::indexical_array<Layout>)
            {
                static_assert(size<Shape> == size<Layout>, "Invalid layout.");
                return make_tuple(detail::unfold_layout_by_relayouted_shape(get<I>(layout), get<I>(shape))...);
            }
            else
            {
                return make_tuple(
                    detail::unfold_layout_by_relayouted_shape(detail::array_cat(layout, array{ I }) , get<I>(shape))...
                );
            }
        }(std::make_index_sequence<size<Shape>>{});
    }

    template<auto Layout>
    constexpr auto apply_layout(const auto& view)
    {
        using layout_type = decltype(Layout);
        if constexpr(indexical<layout_type>)
        {
            return subtree<Layout>(view);
        }
        else return[&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::apply_layout<get<I>(Layout)>(view)...);
        }(std::make_index_sequence<size<layout_type>>{});
    }

    template<auto UnfoldedLayout, typename U, typename R>
    constexpr void inverse_relayout_usage_tree_at(const U& usage_tree, R& result)
    {
        if constexpr(indexical<decltype(UnfoldedLayout)>)
        {
            auto&& subresult = subtree<UnfoldedLayout>(result);
            if constexpr(terminal<decltype(subresult)>)
            {
                subresult = subresult & usage_tree;
            }
            else return [&]<size_t...I>(std::index_sequence<I...>)
            {
                (..., detail::inverse_relayout_usage_tree_at<indexes_of_whole>(usage_tree, get<I>(subresult)));
            }(std::make_index_sequence<size<decltype(subresult)>>{});
        }
        else return[&]<size_t...I>(std::index_sequence<I...>)
        {
            (..., detail::inverse_relayout_usage_tree_at<get<I>(UnfoldedLayout)>(detail::tag_tree_get<I>(usage_tree), result));
        }(std::make_index_sequence<size<decltype(UnfoldedLayout)>>{});
    }

    template<auto UnfoldedLayout, typename U, typename S>
    constexpr auto inverse_relayout_usage_tree(const U& usage_tree, const S& shape)
    {
        auto result = detail::make_tree_of_same_value(usage_t::none, shape);
        detail::inverse_relayout_usage_tree_at<UnfoldedLayout>(usage_tree, result);
        return result;
    }
    
    //template<auto Layout, class L>
    // constexpr auto relayout_layout(const L layout)
    // {
    //     if constexpr(indexical<decltype(Layout)>)
    //     {
    //         return detail::sublayout<Layout>(layout);
    //     }
    //     else return [&]<size_t...I>(std::index_sequence<I...>)
    //     {
    //         return senluo::make_tuple(senluo::relayout_layout<get<I>(Layout)>(layout)...);
    //     }(std::make_index_sequence<size<decltype(Layout)>>{});
    // }

    //todo... should change to discard repeat.
    template<auto FoldedLayout, class O>
    constexpr bool is_enable_to_relayout_operation_tree(const O& folded_operation_tree)
    {
        if constexpr(indexical<decltype(FoldedLayout)>)
        {
            if constexpr(FoldedLayout.size() == 0uz)
            {
                return true;
            }
            else
            {
                //msvc bug
                constexpr auto i = detail::array_take<FoldedLayout.size() - 1uz>(FoldedLayout);
                return not std::same_as<decltype(detail::tag_subtree<i>(folded_operation_tree)), operation_t>
                || detail::equal(detail::tag_subtree<FoldedLayout>(folded_operation_tree), operation_t::none);
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return (... && detail::is_enable_to_relayout_operation_tree<get<I>(FoldedLayout)>(folded_operation_tree));
        }(std::make_index_sequence<size<decltype(FoldedLayout)>>{});
    };
}

namespace senluo
{
    namespace detail::make_t_ns
    {
        template<typename T, auto indexes>
        struct make_t;
    }

    template<typename T, indexical auto...indexes>
    inline constexpr auto make = detail::make_t_ns::make_t<T, detail::to_indexes(indexes...)>{};
}

namespace senluo::detail::relayout_ns
{
    // template<typename TBasePrinciple, auto FoldedLayout>
    // struct principle_t : detail::based_on<TBasePrinciple>, principle_interface<principle_t<TBasePrinciple, FoldedLayout>>
    // {
    //     friend constexpr auto data(unwarp_derived_from<principle_t> auto&& self)
    //     AS_EXPRESSION( 
    //         data(FWD(self) | base)
    //     )
        
    //     static consteval auto layout()
    //     {
    //         constexpr auto data_shape = shape<decltype(data(std::declval<TBasePrinciple>()))>;
    //         constexpr auto base_unfolded_layout = detail::unfold_layout<TBasePrinciple::layout()>(data_shape);
    //         return detail::apply_layout<FoldedLayout>(base_unfolded_layout); 
    //     }
        
    //     static consteval auto stricture_tree()
    //     { 
    //         return detail::relayout_tag_tree<FoldedLayout>(TBasePrinciple::stricture_tree());
    //     }

    //     // static consteval auto operation_tree()
    //     // {
    //     //     return detail::relayout_tag_tree<FoldedLayout>(TBasePrinciple::operation_tree());
    //     // }
    // };

    template<typename T, auto FoldedLayout>
    struct tree_t : detail::based_on<T>, standard_interface<tree_t<T, FoldedLayout>>
    {
        template<size_t I, unwarp_derived_from<tree_t> Self>
            requires indexical<decltype(detail::layout_get<I>(FoldedLayout))>
        friend constexpr auto subtree(Self&& self)
        AS_EXPRESSION(
            FWD(self) | const_base | senluo::subtree<detail::layout_get<I>(FoldedLayout)>
        )

        template<size_t I, unwarp_derived_from<tree_t> Self>
            requires (not indexical<decltype(detail::layout_get<I>(FoldedLayout))>)
        friend constexpr auto subtree(Self&& self)
        AS_EXPRESSION(
            tree_t<unwrap_t<decltype(FWD(self) | const_base)>, detail::layout_get<I>(FoldedLayout)>{ unwrap_fwd(FWD(self) | const_base) }
        )

        // Complex sfinae and noexcept are not currently provided.
        // template<auto UsageTree, unwarp_derived_from<tree_t> Self>
        // friend constexpr decltype(auto) principle(Self&& self)
        // {
        //     constexpr auto unfolded_layout = detail::unfold_layout<FoldedLayout>(shape<T>);
        //     constexpr auto base_usage = detail::inverse_relayout_usage_tree<unfolded_layout>(UsageTree, shape<T>);

        //     using base_principle_t = decltype(FWD(self) | base | senluo::principle<base_usage>);
        //     return principle_t<base_principle_t, FoldedLayout>{ FWD(self) | base | senluo::principle<base_usage> };

        //     // if constexpr(detail::is_enable_to_relayout_operation_tree<FoldedLayout>(base_principle_t::folded_operation_tree()))
        //     // {
        //     //     return principle_t<base_principle_t, FoldedLayout>{ FWD(self) | base | senluo::principle<base_usage> };
        //     // }
        //     // else
        //     // {
        //     //     using base_plain_principle_t = decltype(FWD(self) | base | plainize_principle<UsageTree>);

        //     //     return principle_t<base_plain_principle_t, FoldedLayout>{ 
        //     //         FWD(self) | base | plainize_principle<UsageTree>
        //     //     };
        //     // }           
        // }

        friend constexpr auto get_maker(type_tag<tree_t>) noexcept
        requires (not std::same_as<decltype(detail::inverse_layout<detail::unfold_layout<FoldedLayout>(shape<T>)>(shape<T>)), tuple<>>)
        {
            return []<class U>(U&& tree)
            {
                return tree_t{ 
                    FWD(tree) 
                    | relayout<detail::inverse_layout<detail::unfold_layout<FoldedLayout>(shape<T>)>(shape<T>)> 
                    | senluo::make<T> 
                };
            };
        }
    };
}

namespace senluo
{
    template<auto Layout>
    struct detail::relayout_t : adaptor_closure<relayout_t<Layout>>
    {
        template<typename T> 
#ifdef _MSC_VER
        requires indexical<decltype(detail::fold_layout<Layout>(shape<T>))> && (not std::is_object_v<subtree_t<T, detail::fold_layout<Layout>(shape<T>)>>)
#else
        requires (not std::is_object_v<subtree_t<T, detail::fold_layout<Layout>(shape<T>)>>)
#endif
        constexpr auto operator()(T&& t)const
        AS_EXPRESSION(
            refer(subtree<detail::fold_layout<Layout>(shape<T>)>(FWD(t)))
        )

        template<typename T> 
#ifdef _MSC_VER
        requires indexical<decltype(detail::fold_layout<Layout>(shape<T>))> && std::is_object_v<subtree_t<T, detail::fold_layout<Layout>(shape<T>)>>
#else
        requires std::is_object_v<subtree_t<T, detail::fold_layout<Layout>(shape<T>)>>
#endif
        constexpr auto operator()(T&& t)const
        AS_EXPRESSION(
            decltype(wrap(subtree<detail::fold_layout<Layout>(shape<T>)>(FWD(t)))){
                subtree<detail::fold_layout<Layout>(shape<T>)>(FWD(t))
            }
        )

        template<typename T> requires (not indexical<decltype(detail::fold_layout<Layout>(shape<T>))>)
        constexpr auto operator()(T&& t)const
        AS_EXPRESSION(
            relayout_ns::tree_t<senluo::unwrap_t<T>, detail::fold_layout<Layout>(shape<T>)>{ unwrap_fwd(FWD(t)) }
        )
    };

    template<class Relayouter>
    struct relayouter_interface;

    template<class Relayouter>
    struct relayouter_interface : adaptor_closure<Relayouter>
    {
        template<typename T, derived_from<Relayouter> Self>
        constexpr auto operator()(this Self&& self, T&& tree)
        {
            constexpr auto layout = Relayouter::relayout(detail::default_unfolded_layout<T>());
            return FWD(tree) | relayout<layout>;
        }
    };

    namespace detail
    {
        template<size_t N>
        struct repeat_t : relayouter_interface<repeat_t<N>>
        {
            static constexpr auto relayout(const auto& tree)
            {
                return [&]<size_t...I>(std::index_sequence<I...>)
                {
                    return tuple{ (I, tree)... };
                }(std::make_index_sequence<N>{});
            }
        };
    }

    template<size_t N>
    inline constexpr detail::repeat_t<N> repeat{};

    namespace detail
    {
        template<size_t I, size_t Axis>
        struct component_t : relayouter_interface<component_t<I, Axis>>
        {
            template<typename T>
            static constexpr auto relayout(const T& tree)
            {
                if constexpr (Axis == 0uz)
                {
                    static_assert(I < size<T>, "Component index out of range.");
                    return subtree<I>(tree);
                }
                else
                {
                    static_assert(branched<T>, "Axis index out of range.");
                    return[&]<size_t...J>(std::index_sequence<J...>)
                    {
                        return make_tuple(component_t<I, Axis - 1uz>::relayout(subtree<J>(tree))...);
                    }(std::make_index_sequence<size<T>>{});
                }
            }
        };
    }

    template<size_t I, size_t Axis>
    inline constexpr detail::component_t<I, Axis> component{}; 

    namespace detail
    {
        template<size_t Axis1, size_t Axis2>
        struct transpose_t : relayouter_interface<transpose_t<Axis1, Axis2>>
        {
            template<typename T>
            static constexpr auto relayout(const T& tree)
            {
                if constexpr (Axis1 == 0uz)
                {
                    constexpr size_t N = tensor_shape<T>[Axis2];
                    return[&]<size_t...I>(std::index_sequence<I...>)
                    {
                        return senluo::make_tuple(component_t<I, Axis2>::relayout(tree)...);
                    }(std::make_index_sequence<N>{});
                }
                else return[&]<size_t...I>(std::index_sequence<I...>)
                {
                    return senluo::make_tuple(transpose_t<Axis1 - 1uz, Axis2 - 1uz>::relayout(subtree<I>(tree))...);
                }(std::make_index_sequence<size<T>>{});
            }
        };
    }

    template<size_t Axis1 = 0uz, size_t Axis2 = Axis1 + 1uz>
    inline constexpr detail::transpose_t<Axis1, Axis2> transpose{}; 

    namespace detail
    {
        struct inverse_t : relayouter_interface<inverse_t>
        {
            template<typename T>
            static constexpr auto relayout(const T& tree)
            {
                return[&]<size_t...I>(std::index_sequence<I...>)
                {
                    constexpr auto last_index = size<T> - 1uz;
                    return make_tuple(subtree<last_index - I>(tree)...);
                }(std::make_index_sequence<size<T>>{});
            }
        };
    }

    inline constexpr detail::inverse_t inverse{}; 

    namespace detail
    {
        struct combine_t
        {
            template<typename...T>
            constexpr auto operator()(T&&...t) const
            {
                return tuple<unwrap_t<T>...>{ unwrap_fwd(FWD(t))... };
            }
        };
    }

    inline constexpr detail::combine_t combine{};  

    namespace detail
    {
        struct zip_t
        {
            template<typename...T>
            constexpr auto operator()(T&&...t) const
            {
                return tuple<T...>{ FWD(t)... } | transpose<>;
            }
        };
    }

    inline constexpr detail::zip_t zip{};    
}

#include "../macro_undef.hpp"
#endif