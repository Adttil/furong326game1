#ifndef SENLUO_TAG_HPP
#define SENLUO_TAG_HPP

#include "../general.hpp"
#include "subtree.hpp"

#include "../macro_define.hpp"

namespace senluo
{
    enum class usage_t
    {
        none,
        once,
        repeatedly
    };

    constexpr usage_t operator&(usage_t l, usage_t r)noexcept
    {
        auto result = std::to_underlying(l) + std::to_underlying(r);
        return result < std::to_underlying(usage_t::repeatedly) ? usage_t{ result } : usage_t::repeatedly ;
    }

    enum class stricture_t
    {
        none,
        readonly
    };

    constexpr stricture_t operator&(stricture_t l, stricture_t r)noexcept
    {
        if(l == stricture_t::none && r == stricture_t::none)
        {
            return stricture_t::none;
        }
        else
        {
            return stricture_t::readonly;
        }
    }

    enum class operation_t
    {
        none,
        apply_invoke
    };
}

namespace senluo::detail
{
    //for msvc adl bug: https://gcc.godbolt.org/z/GYvdrbooW
    template<size_t>
    void get();
}

namespace senluo::detail
{
    template<size_t I, class T>
    constexpr auto tag_tree_get(const T& tag_tree)
    {
        if constexpr(terminal<T>)
        {
            return tag_tree;
        }
        else
        {
            return subtree<I>(tag_tree);
        }
    }
    
    template<auto indexes, class T>
    constexpr auto tag_subtree(const T& tag_tree)
    {
        if constexpr(detail::equal(indexes, indexes_of_whole))
        {
            return tag_tree;
        }
        else
        {
            return detail::tag_subtree<detail::array_drop<1uz>(indexes)>(detail::tag_tree_get<indexes[0]>(tag_tree));
        }
    }

    template<size_t I, class L>
    constexpr auto layout_get(const L& layout)
    {
        if constexpr(indexical<L>)
        {
            return detail::array_cat(layout, array{ I });
        }
        else if constexpr(I < size<L>)
        {
            return get<I>(layout);
        }
        // else
        // {
        //     return invalid_index;
        // }
    }
    
    template<typename TLayout, size_t N>
    constexpr auto layout_add_prefix(const TLayout& layout, const array<size_t, N>& prefix)
    {
        if constexpr(detail::indexical_array<TLayout>)
        {
            return detail::array_cat(prefix, layout);
        }
        else return[&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::layout_add_prefix(get<I>(layout), prefix)...);
        }(std::make_index_sequence<size<TLayout>>{});
    }
    
    template<indexical auto Indices, class Shape>
    constexpr auto normalize_indices(Shape shape)
    {
        if constexpr(std::integral<decltype(Indices)>)
        {
            return array{ detail::normalize_index(Indices, size<Shape>) };
        }
        else return []<size_t...I>(std::index_sequence<I...>)
        {
            return array<size_t, Indices.size()>
            {
                detail::normalize_index(Indices[I], size<subtree_t<Shape, detail::array_take<I>(Indices)>>)...
            };
        }(std::make_index_sequence<Indices.size()>{});
    }

    template<typename T>
    constexpr auto default_unfolded_layout()
    {
        if constexpr (terminal<T>)
        {
            return indexes_of_whole;
        }
        else return[]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::layout_add_prefix(detail::default_unfolded_layout<subtree_t<T&, I>>(), array{I})...);
        }(std::make_index_sequence<size<T>>{});    
    };

    template<auto Layout, class Shape>
    constexpr auto fold_layout(Shape shape = {})
    {
        if constexpr(indexical<decltype(Layout)>)
        {
            return detail::normalize_indices<Layout>(shape);
        }
        else return []<size_t...I>(std::index_sequence<I...>)
        {
            constexpr auto child_relayout = make_tuple(detail::fold_layout<subtree<I>(Layout)>(Shape{})...);
            constexpr size_t n = size<decltype(get<0uz>(child_relayout))>;

            if constexpr(n > 0uz
                && (... && detail::indexical_array<decltype(get<I>(child_relayout))>)
                && (... && (n == size<decltype(get<I>(child_relayout))>))
            )
            {
                constexpr auto prefix = detail::array_take<n - 1uz>(get<0uz>(child_relayout));
                if constexpr((... && (prefix == detail::array_take<n - 1uz>(get<I>(child_relayout))))
                    && (... && (get<I>(child_relayout)[n - 1uz] == I))
                )
                {
                    return prefix;
                }
                else
                {
                    return child_relayout;
                }
            }
            else
            {
                return child_relayout;
            }
        }(std::make_index_sequence<size<decltype(Layout)>>{});
    }

    template<auto Layout, class T>
    constexpr auto relayout_tag_tree(const T tag_tree)
    {
        if constexpr(indexical<decltype(Layout)>)
        {
            return detail::tag_subtree<Layout>(tag_tree);
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::relayout_tag_tree<get<I>(Layout)>(tag_tree)...);
        }(std::make_index_sequence<size<decltype(Layout)>>{});
    }

    template<class S1, class S2>
    constexpr auto merge_stricture_tree(const S1& tree1, const S2& tree2)
    {
        if constexpr(std::same_as<S1, stricture_t>)
        {
            if constexpr(std::same_as<S2, stricture_t>)
            {
                return tree1 & tree2;
            }
            else
            {
                return detail::merge_stricture_tree(tree2, tree1);
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        { 
            //static_assert(size<S1> > 0);
            if constexpr(std::same_as<S2, stricture_t>)
            {
                return make_tuple(detail::merge_stricture_tree(get<I>(tree1), tree2)...);
            }
            else
            {
                return make_tuple(detail::merge_stricture_tree(get<I>(tree1), get<I>(tree2))...);
            }
        }(std::make_index_sequence<size<S1>>{});
    }

    template<auto TagTree>
    constexpr auto fold_tag_tree()
    {
        if constexpr(terminal<decltype(TagTree)>)
        {
            return TagTree;
        }
        else return []<size_t...I>(std::index_sequence<I...>)
        {
            constexpr auto subresults = make_tuple(detail::fold_tag_tree<subtree<I>(TagTree)>()...);
            if constexpr((terminal<decltype(get<0uz>(subresults))> && ... && detail::equal(get<I>(subresults), get<0uz>(subresults))))
            {
                return get<0uz>(subresults);
            }
            else
            {
                return subresults;
            }
        }(std::make_index_sequence<size<decltype(TagTree)>>{});
    }
    
    template<typename TagTree, typename S>
    constexpr auto unfold_tag_tree(TagTree tree, S = {})
    {
        if constexpr(terminal<TagTree>)
        {
            return detail::make_tree_of_same_value(tree, S{});
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::unfold_tag_tree(get<I>(tree), get<I>(S{}))...);
        }(std::make_index_sequence<size<TagTree>>{});
    }
    
    template<auto OperationTree>
    constexpr auto fold_operation_tree()
    {
        if constexpr(std::same_as<decltype(OperationTree), operation_t>)
        {
            return OperationTree;
        }
        else return[]<size_t...I>(std::index_sequence<I...>)
        {
            constexpr auto subresults = make_tuple(detail::fold_operation_tree<subtree<I>(OperationTree)>()...);
            if constexpr((... && detail::equal(get<I>(subresults), operation_t::none)))
            {
                return operation_t::none;
            }
            else
            {
                return subresults;
            }
        }(std::make_index_sequence<size<decltype(OperationTree)>>{});
    }

    template<class U>
    constexpr usage_t fold_usage(const U& usage_tree)
    {
        if constexpr(std::same_as<U, usage_t>)
        {
            return usage_tree;
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return (... & detail::fold_usage(get<I>(usage_tree)));
        }(std::make_index_sequence<size<U>>{});
    }

    template<auto FoldedOperationTree, class U>
    constexpr auto fit_operation_usage_impl(const U& usage_tree, bool& need_plain)
    {
        if constexpr(std::same_as<decltype(FoldedOperationTree), operation_t>)
        {
            if constexpr(FoldedOperationTree == operation_t::none)
            {
                return usage_tree;
            }
            else
            {
                usage_t usage = detail::fold_usage(usage_tree);
                need_plain = need_plain || usage == usage_t::repeatedly;
                return usage;
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(
                detail::fit_operation_usage_impl<get<I>(FoldedOperationTree)>(detail::tag_tree_get<I>(usage_tree), need_plain)...
            );
        }(std::make_index_sequence<size<decltype(FoldedOperationTree)>>{});
    }

    template<auto FoldedOperationTree, class U>
    constexpr auto fit_operation_usage(const U& usage_tree)
    {
        bool need_plain = false;

        struct result_t
        {
            decltype(detail::fit_operation_usage_impl<FoldedOperationTree>(usage_tree, need_plain)) usage_tree;
            bool need_plain;
        };

        return result_t{ detail::fit_operation_usage_impl<FoldedOperationTree>(usage_tree, need_plain), need_plain };
    }
}

namespace senluo::detail
{
    template<class DataStrictureTree>
    constexpr auto set_data_stricture_tree(DataStrictureTree& tree)
    {
        if constexpr(std::same_as<DataStrictureTree, stricture_t>)
        {
            tree = stricture_t::readonly;
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            (..., detail::set_data_stricture_tree(get<I>(tree)));
        }(std::make_index_sequence<size<DataStrictureTree>>{});
    }

    template<auto Layout, class DataStrictureTree>
    constexpr auto set_data_stricture_tree_by_layout(DataStrictureTree& tree)
    {
        if constexpr(indexical<decltype(Layout)>)
        {
            detail::set_data_stricture_tree(subtree<Layout>(tree));
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            (..., detail::set_data_stricture_tree_by_layout<get<I>(Layout)>(tree));
        }(std::make_index_sequence<size<decltype(Layout)>>{});
    }

    template<auto RawStrictureTree, auto Layout, auto UsageTree, class CurDataStrictureTree>
    constexpr auto get_inverse_sequence_stricture_tree_impl(CurDataStrictureTree& cur_tree)
    {
        if constexpr(std::same_as<decltype(UsageTree), usage_t>)
        {
            if constexpr(UsageTree == usage_t::none)
            {
                return stricture_t::none;
            }
            else if constexpr(detail::equal(RawStrictureTree, stricture_t::readonly))
            {
                detail::set_data_stricture_tree_by_layout<Layout>(cur_tree);
                return stricture_t::readonly;
            }
            else
            {
                auto result = detail::merge_stricture_tree(RawStrictureTree, detail::relayout_tag_tree<Layout>(cur_tree));
                detail::set_data_stricture_tree_by_layout<Layout>(cur_tree);
                return result;
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return tuple<decltype(detail::get_inverse_sequence_stricture_tree_impl<detail::tag_tree_get<I>(RawStrictureTree)
                                                        , detail::layout_get<I>(Layout)
                                                        , get<I>(UsageTree)>(cur_tree))...>
            {
                detail::get_inverse_sequence_stricture_tree_impl<detail::tag_tree_get<I>(RawStrictureTree)
                                                        , detail::layout_get<I>(Layout)
                                                        , get<I>(UsageTree)>(cur_tree)...
            };
        }(std::make_index_sequence<size<decltype(UsageTree)>>{});
    }

    template<auto RawStrictureTree, auto Layout, auto UsageTree, class CurDataStrictureTree>
    constexpr auto get_sequence_stricture_tree_impl(CurDataStrictureTree& cur_tree)
    {
        if constexpr(std::same_as<decltype(UsageTree), usage_t>)
        {
            if constexpr(UsageTree == usage_t::none)
            {
                return stricture_t::none;
            }
            else if constexpr(detail::equal(RawStrictureTree, stricture_t::readonly))
            {
                detail::set_data_stricture_tree_by_layout<Layout>(cur_tree);
                return stricture_t::readonly;
            }
            else
            {
                auto result = detail::merge_stricture_tree(RawStrictureTree, detail::relayout_tag_tree<Layout>(cur_tree));
                detail::set_data_stricture_tree_by_layout<Layout>(cur_tree);
                return result;
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            static_assert(size<decltype(UsageTree)> > 0);
            constexpr size_t last = size<decltype(UsageTree)> - 1uz;
            auto inverse_result = tuple<decltype(detail::get_sequence_stricture_tree_impl<detail::tag_tree_get<last - I>(RawStrictureTree)
                                                        , detail::layout_get<last - I>(Layout)
                                                        , get<last - I>(UsageTree)>(cur_tree))...>
            {
                detail::get_sequence_stricture_tree_impl<detail::tag_tree_get<last - I>(RawStrictureTree)
                                                        , detail::layout_get<last - I>(Layout)
                                                        , get<last - I>(UsageTree)>(cur_tree)...
            };
            return make_tuple(get<last - I>(inverse_result)...);
        }(std::make_index_sequence<size<decltype(UsageTree)>>{});
    }

    template<auto RawStrictureTree, auto Layout, auto UsageTree, class DataShape>
    constexpr auto get_inverse_sequence_stricture_tree(DataShape data_shape = {})
    {
        auto data_stricture_tree = detail::make_tree_of_same_value(stricture_t::none, data_shape);
        return get_inverse_sequence_stricture_tree_impl<RawStrictureTree, Layout, UsageTree>(data_stricture_tree);
    }

    template<auto RawStrictureTree, auto Layout, auto UsageTree, class DataShape>
    constexpr auto get_sequence_stricture_tree(DataShape data_shape = {})
    {
        auto data_stricture_tree = detail::make_tree_of_same_value(stricture_t::none, data_shape);
        return detail::get_sequence_stricture_tree_impl<RawStrictureTree, Layout, UsageTree>(data_stricture_tree);
    }
}

namespace senluo::detail
{
    template<auto Layout, size_t Depth = 0uz, typename T>
    constexpr bool inverse_layout_index_len_at(T& result_index_len, size_t& setted_count)
    {
        if constexpr(detail::indexical_array<decltype(Layout)>)
        {
            if(subtree<Layout>(result_index_len) == invalid_index)
            {
                subtree<Layout>(result_index_len) = Depth;
                ++setted_count;
                return true;
            }
            else
            {
                return false;
            }
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {   
            return (... && detail::inverse_layout_index_len_at<get<I>(Layout), Depth + 1uz>(result_index_len, setted_count));
        }(std::make_index_sequence<size<decltype(Layout)>>{});
    }

    template<class S, class T>
    constexpr auto make_tree_of_same_value_and_set_leaf_count(const T& value, size_t& count, S = {})
    {
        if constexpr(terminal<S>)
        {
            ++count;
            return value;
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::make_tree_of_same_value_and_set_leaf_count(value, count, get<I>(S{}))...);
        }(std::make_index_sequence<size<S>>{});
    }

    template<auto Layout, typename S>
    constexpr auto inverse_layout_index_len(S = {})
    {
        size_t leaf_count = 0uz;
        auto index_len_tree = detail::make_tree_of_same_value_and_set_leaf_count(invalid_index, leaf_count, S{});
        size_t setted_count = 0uz;
        bool is_injective = detail::inverse_layout_index_len_at<Layout>(index_len_tree, setted_count);
        struct result_t
        {
            bool is_surjection;
            decltype(index_len_tree) index_len_tree;
        };
        return result_t{ is_injective && (leaf_count == setted_count), index_len_tree  };
    }

    template<auto IndexLenTree>
    constexpr auto init_layout()
    {
        if constexpr(std::same_as<decltype(IndexLenTree), size_t>)
        {
            return array<size_t, IndexLenTree>{};
        }
        else return []<size_t...I>(std::index_sequence<I...>)
        {
            return make_tuple(detail::init_layout<get<I>(IndexLenTree)>()...);
        }(std::make_index_sequence<size<decltype(IndexLenTree)>>{});
    }

    template<auto Layout, auto CurIndex = indexes_of_whole, typename R>
    constexpr void inverse_layout_at(R& result)
    {
        if constexpr(detail::indexical_array<decltype(Layout)>)
        {
            subtree<Layout>(result) = CurIndex;
        }
        else return [&]<size_t...I>(std::index_sequence<I...>)
        {   
            return (..., detail::inverse_layout_at<get<I>(Layout), detail::array_cat(CurIndex, array{ I })>(result));
        }(std::make_index_sequence<size<decltype(Layout)>>{});
    }

    template<auto UnFoldedLayout, typename S>
    constexpr auto inverse_layout(S = {})
    {
        constexpr auto inverse_layout_index_len_result = detail::inverse_layout_index_len<UnFoldedLayout, S>();
        
        if constexpr(inverse_layout_index_len_result.is_surjection)
        {
            auto result = detail::init_layout<inverse_layout_index_len_result.index_len_tree>();
            detail::inverse_layout_at<UnFoldedLayout>(result);
            return result;
        }
        else
        {
            return tuple{};
        }
    }
}

#include "../macro_undef.hpp"
#endif