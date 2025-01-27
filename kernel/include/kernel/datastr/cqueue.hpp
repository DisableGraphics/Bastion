#pragma once
#include "vector.hpp"

/**
	\brief Circular queue
 */
template <typename T>
class CircularQueue {
	public:
		CircularQueue();
		/**
			\brief Add a new element
		 */
		void push_back(const T& element);
		/**
			\brief Add a new element
		 */
		void emplace_back(T&& element);
		/**
			\brief Advance to the next position
			\return The previous element in the queue (e.g If we call next() the first time it will 
			return the first element in the queue instead of the second)
		 */
		T& next();
		/**
			\brief Get current position
		 */
		size_t current_position();
	private:
		// Vector of all the elements
		Vector<T> vec;
		// Current element
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