#pragma once
#include <stddef.h>
#include <string.h>
/**
	\brief Queue that uses a fixed size array.
 */
template <typename T, size_t N>
class StaticQueue {
	public:
		/**
			\brief Constructor
		 */
		StaticQueue() {
			// Init to 0
			// Just in case it is a stack variable
			// and the stack contains detritus
			memset(arr, 0, N);
		};
		/**
			\brief Add an element to the queue
			\warning It won't push anything into the queue if the array is full
			\param elem element to add to the queue
		 */
		void push(const T &elem);
		/**
			\brief pops an element off the queue and returns it.
			\return The popped element from the queue
		 */
		T pop();
		/**
			\brief Returns the first element in the queue
			\return The first element
		 */
		T peek();
		/**
			\brief Size of the queue
		*/
		inline size_t size() { return int_size; };
	private:
		size_t int_size = 0;
		T arr[N];
};
template <typename T, size_t N>
void StaticQueue<T, N>::push(const T &elem) {
	if(int_size < N) arr[int_size++] = elem;
}

template <typename T, size_t N>
T StaticQueue<T, N>::pop() {
	T elem = arr[0];
	for(size_t i = 0; i < size(); i++) {
		arr[i] = arr[i+1];
	}
	int_size--;
	return elem;
}

template <typename T, size_t N>
T StaticQueue<T, N>::peek() {
	return arr[0];
}