#pragma once
// Thanks to this tutorial
// https://aozturk.medium.com/simple-hash-map-hash-table-implementation-in-c-931965904250

template <typename K, typename V>
class HashNode {
	public:
		HashNode(const K &key, const V &value);
		K getKey() const;
		V getValue() const;
		void setValue(const V& value);
		HashNode *getNext() const;
		void setNext(HashNode *next);
	private:
		// key-value pair
		K key;
		V value;
		// next bucket with the same key
		HashNode *next;
};

template<typename K, typename V>
HashNode<K, V>::HashNode(const K &key, const V &value) :
	key(key), value(value), next(nullptr) {
}

template<typename K, typename V>
K HashNode<K,V>::getKey() const {
	return key;
}

template<typename K, typename V>
V HashNode<K,V>::getValue() const {
	return value;
}

template<typename K, typename V>
void HashNode<K, V>::setValue(const V& value) {
	this->value = value;
}

template<typename K, typename V>
HashNode<K, V>* HashNode<K, V>::getNext() const {
	return next;
}

template<typename K, typename V>
void HashNode<K, V>::setNext(HashNode<K, V> *next) {
	this->next = next;
}