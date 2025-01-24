#pragma once
#include "vector.hpp"

template <typename T>
class CircularQueue {
	public:
		CircularQueue();
		void push_back(const T& element);
		void emplace_back(T&& element);
		T& next();
		size_t current_position();
	private:
		Vector<T> vec;
		size_t current = 0;
};

template <typename T>
CircularQueue<T>::CircularQueue() {

}

template <typename T>
void CircularQueue<T>::push_back(const T& element) {
	vec.push_back(element);
}

template <typename T>
void CircularQueue<T>::emplace_back(T&& element) {
	vec.emplace_back(move(element));
}

template <typename T>
T& CircularQueue<T>::next() {
	size_t now = current;
	current = (current + 1) % vec.size();
	return vec[now];
}

template <typename T>
size_t CircularQueue<T>::current_position() {
	return current;
}