#ifndef SENLUO_TENSOR_HPP
#define SENLUO_TENSOR_HPP

#include "general.hpp"
#include "core.hpp"

#include "macro_define.hpp"

namespace senluo
{
    using value_t = float;

    template<size_t N, class T = value_t>
    using vec = array<T, N>;

    template<size_t M, size_t N, class T = value_t>
    using rmat = array<array<T, N>, M>;

    template<size_t M, size_t N, class T = value_t>
    using cmat = decltype(std::declval<rmat<N, M>>() | inverse);

    template<size_t M, size_t N, class T = value_t>
    using mat = rmat<M, N, T>;
}

namespace senluo
{
    namespace detail
    {
        struct dot_t : adaptor<dot_t>
        {
            template<class L, class R>
            constexpr decltype(auto) adapt(L&& left, R&& right)const
            {
                return [&]<size_t...I>(std::index_sequence<I...>) -> decltype(auto)
                {
                    //auto&& left_seq = FWD(left) | refer | seperate;
                    //auto&& right_seq = FWD(right) | refer | seperate;
                    return (... + (subtree<I>(FWD(left)) * subtree<I>(FWD(right))));
                }(std::make_index_sequence<detail::min(size<L>, size<R>)>{});
            }
        };
    }

    inline constexpr detail::dot_t dot{};

    namespace detail
    {
        struct mat_mul_vec_t : adaptor<mat_mul_vec_t>
        {
            template<class M, class V>
            constexpr decltype(auto) adapt(M&& mat, V&& vec)const
            {
                constexpr size_t row_count = size<M>;
                return zip_transform(dot, FWD(mat), FWD(vec) | repeat<row_count>);
            }
        };
    }

    inline constexpr detail::mat_mul_vec_t mat_mul_vec{};

    namespace detail
    {
        struct vec_mul_mat_t : adaptor<vec_mul_mat_t>
        {
            constexpr auto adapt(auto&& _vec, auto&& _mat)const
	        {
	            return mat_mul_vec(FWD(_mat) | transpose<>, FWD(_vec));
	        };
        };
    }

    inline constexpr detail::vec_mul_mat_t vec_mul_mat{};

    namespace detail
    {
        struct mat_mul_t : adaptor<mat_mul_t>
        {
            template<size_t I, size_t S, size_t J>
            static constexpr auto get_element_layout()
            {
                return []<size_t...K>(std::index_sequence<K...>)
                {
                    return tuple
                    {
                        array{ 0uz },
                        array{ 1uz, I },
                        tuple{ array{ 2uz, K, J }... }
                    };
                }(std::make_index_sequence<S>{});
            }

            template<size_t I, size_t S, size_t N>
            static constexpr auto get_row_layout()
            {
                return []<size_t...J>(std::index_sequence<J...>)
                {
                    return make_tuple(get_element_layout<I, S, J>()...);
                }(std::make_index_sequence<N>{});
            }

            template<size_t M, size_t S, size_t N>
            static constexpr auto get_layout()
            {
                return []<size_t...I>(std::index_sequence<I...>)
                {
                    return make_tuple(get_row_layout<I, S, N>()...);
                }(std::make_index_sequence<M>{});
            }

            template<class L, class R>
            constexpr decltype(auto) adapt(L&& l, R&& r) const
            {
                constexpr size_t m = size<L>;
                constexpr size_t s = size<R>;
                constexpr size_t n = tensor_shape<R>[1uz];
                
                constexpr auto layout = get_layout<m, s, n>();
                constexpr auto op_tree = detail::make_tree_of_same_value(operation_t::apply_invoke, shape<mat<m, n>>);

                return decltype(combine(decltype(dot){}, FWD(l), FWD(r)) | relayout<layout>)
                {
                    dot, unwrap_fwd(FWD(l)), unwrap_fwd(FWD(r))
                } | operate<op_tree>;
            }
        };
    }

    inline constexpr detail::mat_mul_t mat_mul{};
}

#include "macro_undef.hpp"
#endif