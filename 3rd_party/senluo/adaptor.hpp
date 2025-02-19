#ifndef SENLUO_ADAPTOR_CLOSURE_HPP
#define SENLUO_ADAPTOR_CLOSURE_HPP

#include "general.hpp"
#include "tuple.hpp"

#include "macro_define.hpp"

namespace senluo
{
    template<class F>
    struct adaptor_closure{};

    template<class T>
    concept adaptor_closuroid = requires(std::remove_cvref_t<T>& t) 
    {
        { []<class F>(adaptor_closure<F>&)->F*{}(t) } -> std::same_as<std::remove_cvref_t<T>*>;
    };

    namespace detail
    {
        template<class Adaptor, class...Args>
        struct closure : tuple<Args...>, adaptor_closure<closure<Adaptor, Args...>>
        {
            // clang crashed with [[no_unique_address]]
            // https://github.com/llvm/llvm-project/issues/104227
            // SENLUO(no_unique_address) tuple<Args...> captures;

        private:
            template<typename T, size_t...I, typename Self>
            constexpr auto impl(this Self&& self, T&& t, std::index_sequence<I...>)
            AS_EXPRESSION(
                Adaptor{}(FWD(t), get<I>(FWD(self))...)
            )

        public:
            template<typename T, typename Self>
            constexpr auto operator()(this Self&& self, T&& t)
            AS_EXPRESSION(
                FWD(self).impl(FWD(t), std::index_sequence_for<Args...>{})
            )
        };
    }

    template<class F> 
    struct adaptor
    {
        template<typename...Args>
        constexpr auto operator()(Args&&...args) const
        AS_EXPRESSION(
            F{}.adapt(FWD(args)...)
        )

        template<typename...Args> 
            requires (not requires{ F{}.adapt(std::declval<Args>()...); })
        constexpr auto operator()(Args&&...args) const
        AS_EXPRESSION(
            detail::closure<F, Args...>{ FWD(args)... }
        )
    };
}

namespace senluo::detail
{        
    template<class ClosureLeft, class ClosureRight>
    struct pipeline : adaptor_closure<pipeline<ClosureLeft, ClosureRight>>
    {
        SENLUO(no_unique_address) ClosureLeft  left;
        SENLUO(no_unique_address) ClosureRight right;

        template<class T, class Self>
        constexpr auto operator()(this Self&& self, T&& val)
        AS_EXPRESSION(
            FWD(self, right)(FWD(self, left)(FWD(val)))
        )

        friend constexpr bool operator==(const pipeline&, const pipeline&) = default; 
    };
}

namespace senluo
{
    template<adaptor_closuroid L, adaptor_closuroid R>
    constexpr auto operator|(L&& l, R&& r)
    AS_EXPRESSION(
        detail::pipeline<L, R>{ {}, FWD(l), FWD(r) }
    )

    template<class L, adaptor_closuroid R>
    requires (not adaptor_closuroid<L>)
    constexpr auto operator|(L&& l, R&& r)
    AS_EXPRESSION(
        FWD(r)(FWD(l))
    )
}

#include "macro_undef.hpp"
#endif