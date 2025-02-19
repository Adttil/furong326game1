#ifndef SENLUO_MATH_HPP
#define SENLUO_MATH_HPP

#include <cmath>

#include "general.hpp"

#include "macro_define.hpp"

namespace senluo::detail
{
    constexpr auto&& max(auto&& arg0, auto&&...rest)
    {
        if constexpr(sizeof...(rest) == 0)
        {
            return FWD(arg0);
        }
        else
        {
            auto&& rest_max = detail::max(FWD(rest)...);
            if(arg0 > rest_max)
            {
                return FWD(arg0);
            }
            else
            {
                return FWD(rest_max);
            }
        }
    }

    constexpr auto&& min(auto&& arg0, auto&&...rest)
    {
        if constexpr(sizeof...(rest) == 0)
        {
            return FWD(arg0);
        }
        else
        {
            auto&& rest_min = detail::min(FWD(rest)...);
            if(arg0 < rest_min)
            {
                return FWD(arg0);
            }
            else
            {
                return FWD(rest_min);
            }
        }
    }

    template<typename T>
    constexpr auto sqrt(T x)noexcept
	{
	    return std::sqrt(x);
	};
}

#include "macro_undef.hpp"
#endif