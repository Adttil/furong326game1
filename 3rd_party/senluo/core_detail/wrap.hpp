#ifndef SENLUO_WRAP_HPP
#define SENLUO_WRAP_HPP

#include "../general.hpp"
#include "../adaptor.hpp"
#include "subtree.hpp"

#include "../macro_define.hpp"

namespace senluo::detail
{
    template<class T>
    struct transition_wrapper;
    
    template<class T>
    struct based_on
    {
        SENLUO(no_unique_address) T base;
    };
}

namespace senluo 
{
    template<class T>
    struct standard_interface
    {
        template<class Self>
        constexpr detail::transition_wrapper<Self&&> operator+(this Self&& self) noexcept
        {
            return { FWD(self) };
        }
    };

    template<class T>
    struct wrapper : detail::based_on<T>, standard_interface<wrapper<T>>
    {
        template<size_t I, class Self>
        constexpr auto subtree(this Self&& self)
        AS_EXPRESSION(
            detail::raw_get<I>(FWD(self, base))
        )
    };

    template<class T>
    wrapper(T) -> wrapper<T>;

    template<class T>
    concept wrapped = requires(std::remove_cvref_t<T>& t)
    {
        { []<class V>(wrapper<V>&)->wrapper<V>*{ return nullptr; }(t) } -> std::same_as<std::remove_cvref_t<T>*>;
    };

    namespace detail 
    {
        template<class T>
        constexpr auto unwrap_t_tag()
        {
            if constexpr(not wrapped<T>)
            {
                return type_tag<std::remove_const_t<T>>{};
            }
            else if constexpr(std::is_object_v<T> && requires{ requires std::is_object_v<decltype(std::declval<T>().base)>; })
            {
                return type_tag<std::remove_const_t<decltype(std::declval<T>().base)>>{};
            }
            else
            {
                return type_tag<decltype(FWD(std::declval<T>(), base))>{};
            }
        }
    }
    
    template<class T>
    using unwrap_t = decltype(detail::unwrap_t_tag<T>())::type;
    
    namespace detail 
    {
        struct unwrap_fn : adaptor_closure<unwrap_fn>
        {
            template<class T> requires (not wrapped<T>)
            constexpr T& operator()(T& tree)const noexcept
            {
                return tree;
            }

            template<class T> requires (not wrapped<T>) && std::constructible_from<std::remove_const_t<T>, T>
            constexpr std::remove_const_t<T> operator()(T&& tree) const 
            noexcept(std::is_nothrow_constructible_v<std::remove_const_t<T>, T>)
            {
                return FWD(tree);
            }

            template<wrapped T>
            constexpr auto& operator()(T& tree)const noexcept
            {
                return FWD(tree, base);
            }

            template<wrapped T>
            constexpr auto operator()(T&& tree)const
            AS_EXPRESSION(
                (unwrap_t<T>)FWD(tree, base)
            )
        };

        struct unwrap_fwd_fn : adaptor_closure<unwrap_fwd_fn>
        {
            template<class T>
            constexpr T&& operator()(T&& tree)const noexcept
            {
                return FWD(tree);
            }

            template<wrapped T>
            constexpr auto&& operator()(T&& tree)const noexcept
            {
                return FWD(tree, base);
            }
        };
    }

    inline constexpr detail::unwrap_fn unwrap{};

    inline constexpr detail::unwrap_fwd_fn unwrap_fwd{};

    namespace detail 
    {
        struct wrap_t : adaptor_closure<wrap_t>
        {
            template<class T>
            constexpr auto operator()(T&& t) const
            AS_EXPRESSION(
                wrapper<senluo::unwrap_t<T>>{ unwrap_fwd(FWD(t)) }
            )
        };

        struct refer_t : adaptor_closure<refer_t>
        {
            template<class T>
            constexpr wrapper<senluo::unwrap_t<T&&>&&> operator()(T&& t) const noexcept
            {
                return { unwrap_fwd(FWD(t)) };
            }
        };
    }

    inline constexpr detail::wrap_t wrap{};

    inline constexpr detail::refer_t refer{};

    namespace detail 
    {
        struct base_fn : adaptor_closure<base_fn>
        {
            template<class T>
            constexpr decltype(auto) operator()(T&& t) const noexcept
            {
                if constexpr(std::is_object_v<unwrap_t<T>> && std::is_object_v<decltype(unwrap_fwd(FWD(t)).base)>)
                {
                    return FWD(unwrap_fwd(FWD(t)), base);
                }
                else
                {
                    return FWD(unwrap_fwd(FWD(t)), base) | refer;
                }
            }
        };

        struct const_base_fn : adaptor_closure<const_base_fn>
        {
            template<class T>
            constexpr decltype(auto) operator()(T&& t) const noexcept
            {
                if constexpr(std::is_object_v<unwrap_t<T>> && std::is_object_v<decltype(unwrap_fwd(FWD(t)).base)>)
                {
                    return detail::to_readonly(FWD(unwrap_fwd(FWD(t)), base));
                }
                else
                {
                    return detail::to_readonly(FWD(unwrap_fwd(FWD(t)), base)) | refer;
                }
            }
        };

        inline constexpr base_fn base{};
        inline constexpr const_base_fn const_base{};
    }

    template<class T, class U>
    concept unwarp_derived_from = derived_from<unwrap_t<T>, U>;

    template<auto Indexes>
    struct detail::subtree_fn : adaptor_closure<subtree_fn<Indexes>>
    {
        template<class T> requires (Indexes.size() == 0uz)
        constexpr auto operator()(T&& t) const
        AS_EXPRESSION(
            pass(unwrap_fwd(FWD(t)))
        )

        template<class T> requires (Indexes.size() == 1uz && size<T> > 0)
        constexpr auto operator()(T&& t) const
        AS_EXPRESSION(
            pass(unwrap_fwd(raw_get<detail::normalize_index(Indexes[0], size<T>)>(FWD(t))))
        )

        template<class T> requires (Indexes.size() > 1uz && size<T> > 0)
        constexpr auto operator()(T&& t) const
        AS_EXPRESSION(
            pass(raw_get<detail::normalize_index(Indexes[0], size<T>)>(FWD(t)) | subtree<detail::array_drop<1uz>(Indexes)>)
        )
    };
}

#include "../macro_undef.hpp"
#endif