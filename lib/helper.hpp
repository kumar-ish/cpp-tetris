// Helper for use to be able to pattern match on std::variant types
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Support for std::ranges::to (see:
// https://en.cppreference.com/w/cpp/ranges/to) was only added in C++23, which
// is still in progress to be implemented and at the time of writing