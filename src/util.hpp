#pragma once

#include <cmath>
#include "point.hpp"

namespace psh
{
	template<uint d>
	constexpr uint point_to_index(const point<d>& p, uint width, uint max)
	{
		uint index = p[0];
		for (uint i = 1; i < d; i++)
		{
			index += uint(std::pow(width, i)) * p[i];
		}
		return index % max;
	}

	template<>
	constexpr uint point_to_index<2>(const point<2>& p, uint width, uint max)
	{
		return (width * p[0] + p[1]) % max;
	}

	template<>
	constexpr uint point_to_index<3>(const point<3>& p, uint width, uint max)
	{
		return (p[2] + width * p[1] + width * width * p[0]) % max;
	}

	template<uint d>
	constexpr point<d> index_to_point(uint index, uint width, uint max)
	{
		point<d> output;
		max /= width;
		for (uint i = 0; i < d; i++)
		{
			output[i] = index / max;
			index = index % max;

			if (i + 1 < d)
			{
				max /= width;
			}
		}
		return output;
	}

	template<>
	constexpr point<2> index_to_point<2>(uint index, uint width, uint max)
	{
		return point<2>{index / width, index % width};
	}

	template<>
	constexpr point<3> index_to_point<3>(uint index, uint width, uint max)
	{
		return point<3>{
			index / (width * width),
			(index % (width * width)) / width,
			(index % (width * width)) % width};
	}
}
