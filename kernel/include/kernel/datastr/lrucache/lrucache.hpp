#include <kernel/datastr/hashmap/hashmap.hpp>
#include "node.hpp"

// LRU Cache
template <typename K, typename V>
class LRUCache {
	public:
		class iterator {
			public:
				iterator(Node<K, V>* node) : node(node) {}
				void operator++() {
					node = node->next;
				}
				bool operator==(const iterator& other) {
					return node == other.node;
				}
				bool operator!=(const iterator& other) {
					return node != other.node;
				}
				V& operator*() {
					return node->value;
				}
			private:
				Node<K, V>* node;
		};
		explicit LRUCache(size_t cap) : capacity(cap), head(nullptr), tail(nullptr) {}
		~LRUCache();
		V* get(const K& key);
		size_t size() const;
		template <typename... Args>
		void emplace(K key, Args&&... args);

		iterator begin() { return iterator{head}; };
		iterator end() { return iterator{nullptr}; };
	private:
		size_t capacity;
		HashMap<K, Node<K, V>*> cache;
		Node<K, V>* head;
		Node<K, V>* tail;

		void moveToFront(Node<K, V>* node);
		void removeNode(Node<K, V>* node);
		void insertAtFront(Node<K, V>* node);
		void removeLRUNode();
};

template<typename K, typename V>
LRUCache<K, V>::~LRUCache() {
	while (head) {
		Node<K, V>* temp = head;
		head = head->next;
		delete temp;
	}
}

template<typename K, typename V>
template <typename... Args>
void LRUCache<K, V>::emplace(K key, Args&&... args) {
	Node<K, V>** found = cache.find(key);
	if (found) {
		*((*found)->value) = V(forward<Args>(args)...);
		moveToFront(*found);
	} else {
		if (cache.size() == capacity) {
			removeLRUNode();
		}
		Node<K, V>* newNode = new Node<K, V>(move(key), forward<Args>(args)...);
		insertAtFront(newNode);
		cache[newNode->key] = newNode;
	}
}

template<typename K, typename V>
V* LRUCache<K, V>::get(const K& key) {
	auto it = cache.find(key);
	if (!it) return nullptr;
	moveToFront(*it);
	return (*it)->value.get();
}

template<typename K, typename V>
size_t LRUCache<K, V>::size() const {
	return cache.size();
}


template <typename K, typename V>
void LRUCache<K, V>::moveToFront(Node<K, V>* node) {
	if (node == head) return;
	removeNode(node);
	insertAtFront(node);
}

template <typename K, typename V>
void LRUCache<K, V>::removeNode(Node<K, V>* node) {
	if (!node) return;

	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;

	if (node == head) head = node->next;
	if (node == tail) tail = node->prev;
}

template <typename K, typename V>
void LRUCache<K, V>::insertAtFront(Node<K, V>* node) {
	node->next = head;
	node->prev = nullptr;
	if (head) head->prev = node;
	head = node;
	if (!tail) tail = node;
}

template <typename K, typename V>
void LRUCache<K, V>::removeLRUNode() {
	if (!tail) return;
	cache.erase(tail->key);
	Node<K, V>* prevTail = tail;
	tail = tail->prev;
	if (tail) tail->next = nullptr;
	if (prevTail == head) head = nullptr;  // Last node case
	delete prevTail;  // Free memory
}