#pragma once

#include <vector>

//#define Array std::vector

template <typename T>
class Array : public std::vector<T>
{
public:
	Array<T>() : std::vector<T>() {}
	Array<T>(size_t n) : std::vector<T>(n) {}

	// remove item by swapping it with the last item in the array
	// and then removing the last item in the array (avoids array resizing)
	void swap_remove(size_t index)
	{
		std::swap((*this)[index], (*this)[this->size() - 1]);
		this->pop_back();
	}
};