#pragma once

#include <stddef.h>
#include "node.hpp"

template <typename T>
class LinkedList {
	public:
		class iterator {
			public:
				iterator(LinkedListNode<T>* n) : node(n) {}
				T& operator*() { return node->data; }
				T* operator->() { return &node->data; }
				iterator& operator++() { node = node->next; return *this; }
				iterator& operator--() { node = node->prev; return *this; }
				bool operator==(const iterator& other) const { return node == other.node; }
				bool operator!=(const iterator& other) const { return node != other.node; }
				LinkedListNode<T>* getNode() {return node;}
			private:
				LinkedListNode<T>* node;
		};

		LinkedList() : head(nullptr), tail(nullptr), size_(0) {}
		~LinkedList() { clear(); }

		void push_back(const T& value);
		void push_front(const T& value);
		template <typename... Args>
		void emplace_back(Args&&... args);
		template <typename... Args>
		void emplace_front(Args&&... args);
		void pop_back();
		void pop_front();
		void clear();

		T& front() { return head->data; }
		T& back() { return tail->data; }

		size_t size() const { return size_; }
		bool empty() const { return size_ == 0; }
		iterator begin() { return iterator(head); }
		iterator end() { return iterator(nullptr); }

		void erase(iterator it);
	private:
		LinkedListNode<T>* head;
		LinkedListNode<T>* tail;
		size_t size_;
};

template <typename T>
void LinkedList<T>::push_back(const T& value) {
	emplace_back(value);
}

template<typename T>
void LinkedList<T>::push_front(const T& value) {
	emplace_front(value);
}

template <typename T>
void LinkedList<T>::pop_back() {
	if (!tail) return;
	LinkedListNode<T>* temp = tail;
	tail = tail->prev;
	if (tail) tail->next = nullptr;
	else head = nullptr;
	delete temp;
	--size_;
}

template <typename T>
void LinkedList<T>::pop_front() {
	if (!head) return;
	LinkedListNode<T>* temp = head;
	head = head->next;
	if (head) head->prev = nullptr;
	else tail = nullptr;
	delete temp;
	--size_;
}

template <typename T>
void LinkedList<T>::clear() {
	while (!empty()) pop_back();
}

template<typename T>
template<typename... Args>
void LinkedList<T>::emplace_back(Args&&... args) {
	LinkedListNode<T>* newNode = new LinkedListNode(forward<Args>(move(args))...);
	if (!tail) {
		head = tail = newNode;
	} else {
		tail->next = newNode;
		newNode->prev = tail;
		tail = newNode;
	}
	++size_;
}

template<typename T>
template<typename... Args>
void LinkedList<T>::emplace_front(Args&&... args) {
	LinkedListNode<T>* newNode = new LinkedListNode(forward<Args>(move(args))...);
	if (!head) {
		head = tail = newNode;
	} else {
		newNode->next = head;
		head->prev = newNode;
		head = newNode;
	}
	++size_;
}

template<typename T>
void LinkedList<T>::erase(iterator it) {
	if (it == end()) return;
	LinkedListNode<T>* node = it.getNode();
	if (node == head) {
		pop_front();
	} else if (node == tail) {
		pop_back();
	} else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
		delete node;
		--size_;
	}
}