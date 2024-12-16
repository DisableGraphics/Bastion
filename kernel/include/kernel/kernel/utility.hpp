// Remove reference implementation
template <typename T>
struct remove_reference {
    using type = T;  // If T is not a reference, leave it unchanged
};

template <typename T>
struct remove_reference<T&> {
    using type = T;  // If T is an lvalue reference, strip the reference
};

template <typename T>
struct remove_reference<T&&> {
    using type = T;  // If T is an rvalue reference, strip the reference
};

// Move implementation using the remove_reference trait
template <typename T>
typename remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename remove_reference<T>::type&&>(t);
}