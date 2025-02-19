#ifndef SENLUO_GEO_HPP
#define SENLUO_GEO_HPP

#include <cmath>

#include "tensor.hpp"

#include "macro_define.hpp"

namespace senluo::geo
{
    template<size_t N, class T = float>
    struct vec
    {
        array<T, N> storage;

        template<size_t I, class Self> requires (I < N)
        constexpr auto&& get(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[I]);
        }

        template<class Self> requires (N >= 1)
        constexpr auto&& x(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[0]);
        }

        template<class Self> requires (N >= 2)
        constexpr auto&& y(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[1]);
        }

        template<class Self> requires (N >= 3)
        constexpr auto&& z(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[2]);
        }

        template<class Self> requires (N >= 4)
        constexpr auto&& w(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[3]);
        }

        template<class Self> requires (N >= 1)
        constexpr auto&& r(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[0]);
        }

        template<class Self> requires (N >= 2)
        constexpr auto&& g(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[1]);
        }

        template<class Self> requires (N >= 3)
        constexpr auto&& b(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[2]);
        }

        template<class Self> requires (N >= 4)
        constexpr auto&& a(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.storage[3]);
        }
    };
}

template<size_t N, class T>
struct std::tuple_size<::senluo::geo::vec<N, T>>
 : std::integral_constant<size_t, N>{};

 template<size_t I, size_t N, class T>
struct std::tuple_element<I, ::senluo::geo::vec<N, T>>
 : std::type_identity<T>{};

 namespace senluo::geo
{
    template<size_t N, class T = float>
    constexpr vec<N, T> operator+(const vec<N, T>& l, const vec<N, T>& r)
    {
        return +plus(l, r);
    }

    template<size_t N, class T = float>
    constexpr vec<N, T> operator-(const vec<N, T>& l, const vec<N, T>& r)
    {
        return +minus(l, r);
    }
}

#include "macro_undef.hpp"
#endif