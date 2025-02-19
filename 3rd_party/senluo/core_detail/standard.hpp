#ifndef SENLUO_STANDARD_HPP
#define SENLUO_STANDARD_HPP

#include<functional>

#include "../general.hpp"
#include "subtree.hpp"
#include "wrap.hpp"
#include "pretreat.hpp"
#include "make.hpp"

#include "../macro_define.hpp"

namespace senluo
{
    template<class T>
    concept standard = requires(std::remove_cvref_t<T>& t)
    {
        { []<class V>(standard_interface<V>&)->V*{}(t) } -> std::same_as<std::remove_cvref_t<T>*>;
    };
    
    template<class T>
    struct detail::transition_wrapper
    {
        T&& tree;

        // Complex sfinae and noexcept are not currently provided.
        template<typename U, typename Self>
        constexpr operator U(this Self&& self)
        {
            return FWD(self, tree) | make<U>;
        }
    };

    namespace detail
    {
        struct transition_fn : adaptor_closure<transition_fn>
        {
            template<class T>
            constexpr auto operator()(T&& tree)const
            AS_EXPRESSION(
                transition_wrapper<T>{ FWD(tree) }
            )
        };
    }

    inline constexpr detail::transition_fn transition{};
}

namespace senluo
{
    namespace detail
    {
        struct recursive_zip_transform_fn
        {
            template<class F, class T, class...Rest>
            static constexpr auto op_shape()
            {
                if constexpr(requires{ std::declval<F>()(std::declval<T>(), std::declval<Rest>()...); })
                {
                    return tuple{};
                }
                else return []<size_t...I>(std::index_sequence<I...>)
                {
                    constexpr auto subshape = []<size_t J>()
                    {
                        return op_shape<F, subtree_t<T, J>, subtree_t<Rest, J>...>();
                    };

                    return senluo::make_tuple(subshape.template operator()<I>()...);
                }(std::make_index_sequence<size<T>>{});
            }

            template<size_t N, class Shape, auto Layout = detail::default_unfolded_layout<Shape>()>
            static constexpr auto get_layout()
            {
                if constexpr(terminal<Shape>)
                {
                    return []<size_t...I>(std::index_sequence<I...>)
                    {
                        return tuple{ detail::array_cat(array{ I }, Layout)... };
                    }(std::make_index_sequence<N + 1uz>{});
                }
                else return []<size_t...I>(std::index_sequence<I...>)
                {
                    return senluo::make_tuple(get_layout<N, subtree_t<Shape, I>, get<I>(Layout)>()...);
                }(std::make_index_sequence<size<Shape>>{});
            }

            // Complex sfinae and noexcept are not currently provided.
            template<class F, class...T>
            constexpr decltype(auto) operator()(F&& f, T&&...t)const
            {
                constexpr auto shape = op_shape<F, T...>();
                constexpr auto op_layout = detail::make_tree_of_same_value(indexes_of_whole, shape);
                constexpr auto layout = get_layout<sizeof...(T), decltype(shape)>();
                constexpr auto op_tree = detail::make_tree_of_same_value(operation_t::apply_invoke, shape);
                return combine(FWD(f) | relayout<op_layout>, FWD(t)...) | relayout<layout> | operate<op_tree>; 
            }
        };

        struct recursive_transform_fn : adaptor<recursive_transform_fn>
        {
            template<class Shape, auto Layout = detail::default_unfolded_layout<Shape>()>
            static consteval auto get_layout()
            {
                if constexpr(terminal<Shape>)
                {
                    return tuple{ array{ 0uz }, detail::array_cat(array{ 1uz }, Layout) };
                }
                else return []<size_t...I>(std::index_sequence<I...>)
                {
                    return senluo::make_tuple(get_layout<subtree_t<Shape, I>, get<I>(Layout)>()...);
                }(std::make_index_sequence<size<Shape>>{});
            }

            // Complex sfinae and noexcept are not currently provided.
            template<class F, class T>
            constexpr decltype(auto) adapt(F&& f, T&& t)const
            {
                constexpr auto layout = get_layout<shape_t<T>>();
                constexpr auto op_tree = detail::make_tree_of_same_value(operation_t::apply_invoke, shape<T>);
                return combine(FWD(f), FWD(t)) | relayout<layout> | operate<op_tree>; 
            }
        };
    }

    inline constexpr detail::recursive_transform_fn recursive_transform{};
    inline constexpr detail::recursive_zip_transform_fn recursive_zip_transform{};

    namespace detail
    {
        template<class F>
        struct recursive_zip_binary_fn : adaptor<recursive_zip_binary_fn<F>>
        {
            // Complex sfinae and noexcept are not currently provided.
            template<class L, class R>
            constexpr decltype(auto) adapt(L&& l, R&& r)const
            {
                return recursive_zip_transform(F{}, FWD(l), FWD(r));
            }
        };
    }

    template<class F>
    inline constexpr detail::recursive_zip_binary_fn<F> recursive_zip_binary_fn{};

    inline constexpr auto plus = recursive_zip_binary_fn<std::plus<>>;
    inline constexpr auto minus = recursive_zip_binary_fn<std::minus<>>;
    inline constexpr auto multiplies = recursive_zip_binary_fn<std::multiplies<>>;
    inline constexpr auto divides = recursive_zip_binary_fn<std::divides<>>;

    template<standard L, standard R>
    inline constexpr decltype(auto) operator+(L&& l, R&& r)
    {
        return plus(FWD(l), FWD(r))/* | make<std::remove_cvref_t<L>>*/;
    }

    template<standard L, standard R>
    inline constexpr decltype(auto) operator-(L&& l, R&& r)
    {
        return minus(FWD(l), FWD(r));
    }

    template<standard L, terminal R> requires (not standard<R>)
    inline constexpr decltype(auto) operator*(L&& l, R&& r)
    {
        return recursive_transform([r = wrap(FWD(r))](auto&& x){ return FWD(x) * r.base; }, FWD(l));
    }

    template<terminal L, standard R> requires (not standard<L>)
    inline constexpr decltype(auto) operator*(L&& l, R&& r)
    {
        return recursive_transform([l = wrap(FWD(l))](auto&& x){ return l.base * FWD(x); }, FWD(r));
    }
}

#include "../macro_undef.hpp"
#endif