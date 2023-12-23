#pragma once

// Helper for use to be able to pattern match on std::variant types
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Merge a variant with additional options
template <typename T, typename... Args> struct variant_union;
template <typename... Args0, typename... Args1>
struct variant_union<std::variant<Args0...>, Args1...> {
    using type = std::variant<Args0..., Args1...>;
};
