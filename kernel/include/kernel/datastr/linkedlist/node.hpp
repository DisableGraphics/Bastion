#include <kernel/cpp/forward.hpp>

template <typename T>
struct LinkedListNode {
	T data;
	LinkedListNode* next;
	LinkedListNode* prev;
	explicit LinkedListNode(const T& value) : data(value), next(nullptr), prev(nullptr) {}
	template<typename... Args>
	LinkedListNode(Args&& ... args) : data(forward<Args>(args)...), next(nullptr), prev(nullptr) {}
};