#include <stddef.h>

template <typename T, size_t N>
class StaticQueue {
	public:
		StaticQueue(){};
		void push(const T &elem);
		T pop();
		T peek();
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