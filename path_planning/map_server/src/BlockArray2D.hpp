#pragma once

#include <cstddef>	 //std::size_t
#include <stdexcept> // std::out_of_range
#include <cstring>	 //memcpy

template <typename T>
class ArrayView
{
public:
	ArrayView(T *arr, size_t len) : arr(arr), length(len) {}

	T &operator[](size_t index)
	{
#ifdef _DEBUG
		if (index >= length)
		{
			throw std::out_of_range("Array View Accessed Out Of Bounds");
		}
#endif
		return arr[index];
	}

	const T &operator[](size_t index) const
	{
#ifdef _DEBUG
		if (index >= length)
		{
			throw std::out_of_range("Array View Accessed Out Of Bounds");
		}
#endif
		return arr[index];
	}

private:
	T *arr;
	const size_t length;
};

template <typename T>
class BlockArray2DRT
{
public:
	BlockArray2DRT(size_t dim_1_in, size_t dim_2_in) : arr((T*)malloc(dim_1_in * dim_2_in * sizeof(T))), dim_1(dim_1_in), dim_2(dim_2_in){};

	BlockArray2DRT(BlockArray2DRT &other) : arr((T*)malloc(other.dim_1_in * other.dim_2_in * sizeof(T))), dim_1(other.dim_1), dim_2(other.dim_2)
	{
		std::memcpy((void *)arr, (void *)other.arr, other.dim_1 * other.dim_2);
	};

	ArrayView<T> operator[](size_t index)
	{
#ifdef _DEBUG
		if (index >= dim_1)
		{
			throw std::out_of_range("Array View Accessed Out Of Bounds");
		}
#endif
		return ArrayView<T>(&arr[index * dim_2], dim_2);
	}

	const ArrayView<T> operator[](size_t index) const
	{
#ifdef _DEBUG
		if (index >= dim_1)
		{
			throw std::out_of_range("Array View Accessed Out Of Bounds");
		}
#endif
		return ArrayView<T>(&arr[index * dim_2], dim_2);
	}

	~BlockArray2DRT()
	{
		free(arr);
	}

private:
	T *const arr;
	const size_t dim_1, dim_2;
};

