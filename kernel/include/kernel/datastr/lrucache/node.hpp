#include <kernel/datastr/uptr.hpp>
#include <kernel/cpp/forward.hpp>

// Node for doubly linked list
template <typename K, typename V>
struct Node {
	K key;
	UniquePtr<V> value;
	Node* prev;
	Node* next;

	template <typename... Args>
	Node(K k, Args&&... args) :
		key(move(k)), 
		value(forward<Args>(args)...), 
		prev(nullptr), 
		next(nullptr) 
		{}
};