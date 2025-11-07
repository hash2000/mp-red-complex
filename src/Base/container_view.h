#pragma once
#include <ranges>
#include <type_traits>

template <typename T>
const auto& extract_ptr(const T& x) {
  return x; // для vector: x — unique_ptr
}

template <typename K, typename V>
const V& extract_ptr(const std::pair<K, V>& p) {
  return p.second; // для map: p — pair, нужен second
}

template <typename Container>
auto make_unique_ptr_view(const Container& container) {
  using ItemRef = decltype(*container.begin());
  using PtrType = std::decay_t<
      decltype(extract_ptr(std::declval<ItemRef>()))
  >;
  using ElementType = typename PtrType::element_type;

  return container
    | std::views::transform([=](const auto& item) -> const ElementType& {
      return *extract_ptr(item);
    });
}
