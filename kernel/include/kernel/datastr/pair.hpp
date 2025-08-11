#pragma once
/**
	\brief Generic pair class.
 */
template <typename T, typename K>
class Pair {
	public:
		Pair(){};
		Pair(T&& first, K&& second) : first(move(first)), second(move(second)){}
		Pair(const T& first, const K& second) : first(first), second(second){}
		/// First element in the pair
		T first;
		/// Second element in the pair
		K second;
	private:
};