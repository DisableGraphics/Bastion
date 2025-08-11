#pragma once
#include <stddef.h>
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/pair.hpp>
#include <kernel/datastr/linkedlist/linkedlist.hpp>
#include "hash.hpp"

template <typename Key, typename Value>
class HashMap {
	public:
		HashMap() : table(DEFAULT_BUCKETS), size_(0) {}
		void insert(const Key& key, const Value& value);
		Value& operator[](const Key& key);
		bool erase(const Key& key);
		Value* find(const Key& key);
		size_t size() const { return size_; }
		bool empty() const { return size_ == 0; }
	private:
		static const size_t DEFAULT_BUCKETS = 16;
		Vector<LinkedList<Pair<Key, Value>>> table;
		size_t size_;
		size_t hash(const Key& key) const;
};

template<typename Key, typename Value>
void HashMap<Key, Value>::insert(const Key& key, const Value& value) {
	size_t idx = hash(key);
	for (auto& pair : table[idx]) {
		if (pair.first == key) {
			pair.second = value;
			return;
		}
	}
	table[idx].emplace_back(Pair(key, value));
	++size_;
}

template <typename Key, typename Value>
Value& HashMap<Key, Value>::operator[](const Key& key) {
	size_t idx = hash(key);
	for (auto& pair : table[idx]) {
		if (pair.first == key) {
			return pair.second;
		}
	}
	table[idx].emplace_back(key, Value());
	++size_;
	return table[idx].back().second;
}

template <typename Key, typename Value>
bool HashMap<Key, Value>::erase(const Key& key) {
	size_t idx = hash(key);
	auto& bucket = table[idx];
	for (auto it = bucket.begin(); it != bucket.end(); ++it) {
		if (it->first == key) {
			bucket.erase(it);
			--size_;
			return true;
		}
	}
	return false;
}

template<typename Key, typename Value>
Value* HashMap<Key, Value>::find(const Key& key) {
	size_t idx = hash(key);
	for (auto& pair : table[idx]) {
		if (pair.first == key) {
			return &pair.second;
		}
	}
	return nullptr;
}

template<typename Key, typename Value>
size_t HashMap<Key, Value>::hash(const Key& key) const {
	return Hash<Key>{}(key) % table.size();
}