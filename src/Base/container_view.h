#pragma once
#include <ranges>
#include <type_traits>
#include <memory>


template <typename T>
const T& deref_impl(const T& x) {
    return x;
}

template <typename T>
const T& deref_impl(const std::unique_ptr<T>& p) {
    return *p;
}

template <typename T>
const T& deref_impl(const std::shared_ptr<T>& p) {
    return *p;
}

template <typename T>
const T& deref_impl(const T* p) {
    return *p;
}

template <typename K, typename V>
decltype(auto) extract_for_deref(const std::pair<K, V>& p) {
    return deref_impl(p.second);
}

template <typename T>
decltype(auto) extract_for_deref(const T& x) {
    return deref_impl(x);
}

template <typename Elem>
using deref_result_t = decltype(extract_for_deref(std::declval<Elem>()));

template <typename Elem>
concept dereferenceable =
    std::is_lvalue_reference_v<deref_result_t<Elem>> &&
    !std::is_rvalue_reference_v<deref_result_t<Elem>>;

template <std::ranges::input_range Container>
    requires dereferenceable<std::ranges::range_reference_t<Container>>
auto make_deref_view(const Container& container) {
    using ElemRef = std::ranges::range_reference_t<Container>;
    using ResultRef = deref_result_t<ElemRef>;

    static_assert(std::is_lvalue_reference_v<ResultRef>,
        "extract_for_deref must return an lvalue reference to avoid dangling");

    return container
        | std::views::transform([](const auto& item) -> ResultRef {
            return extract_for_deref(item);
        });
}
