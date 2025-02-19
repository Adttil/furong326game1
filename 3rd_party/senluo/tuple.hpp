#ifndef SENLUO_TUPLE_HPP
#define SENLUO_TUPLE_HPP
//this tuple is aggregate

#include "general.hpp"

#include "macro_define.hpp"

namespace senluo
{
    template<class...T>
    struct tuple;

#include "code_generate/tuple_specialization.code"

	template<class T, class...Rest>
    struct tuple<T, Rest...>
	{
	    SENLUO(no_unique_address) T              first;
	    SENLUO(no_unique_address) tuple<Rest...> rest;

		template<size_t I, derived_from<tuple> Self> requires (I <= sizeof...(Rest))
	    friend constexpr auto&& get(Self&& self) noexcept
		{
		    if constexpr (I)
			{
			    return get<I - 1>(FWD(self, rest));
			}
		    else
			{
			    return FWD(self, first);
			}
		}

	    friend constexpr bool operator==(const tuple&, const tuple&) = default;
	};

	template<class...T>
    tuple(T...) -> tuple<std::decay_t<T>...>;
}

template<class...T>
struct std::tuple_size<senluo::tuple<T...>> : std::integral_constant<std::size_t, sizeof...(T)> {};

template<size_t I, class...T>
struct std::tuple_element<I, senluo::tuple<T...>> : std::tuple_element<I, std::tuple<T...>> {};

namespace senluo
{
	namespace detail 
	{
		struct make_tuple_fn
		{
			template<class...Args>
    		constexpr auto operator()(Args&&...args) const
			AS_EXPRESSION(tuple<std::decay_t<Args>...>{ FWD(args)... });
		};

		struct fwd_as_tuple_fn
		{
			template<class...Args>
    		constexpr tuple<Args&&...> operator()(Args&&...args) const noexcept
			{
			    return { FWD(args)... };
			};
		};
	}

	inline constexpr detail::make_tuple_fn make_tuple{};

	inline constexpr detail::fwd_as_tuple_fn fwd_as_tuple{};
}

#include "macro_undef.hpp"
#endif