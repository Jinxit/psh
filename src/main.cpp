#include "psh.hpp"
#include <experimental/optional>
#include <iostream>
#include <chrono>
#include <stdint.h>

struct dumb
{
	union
	{
		struct { float r, g, b; };
		float colors[3];
	};
	dumb() : colors{0} { };
	dumb(float r, float g, float b) : r(r), g(g), b(b) { };

	friend bool operator==(const dumb& lhs, const dumb& rhs)
	{
		return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
	}
	friend bool operator!=(const dumb& lhs, const dumb& rhs) { return !(lhs == rhs); }
	friend std::ostream& operator<< (std::ostream& stream, const dumb& d)
	{
		stream << "(" << d.r << ", " << d.g << ", " << d.b << ")";
		return stream;
	}
};

struct voxelgroup
{
	uint16_t voxels[8];

	friend bool operator==(const voxelgroup& lhs, const voxelgroup& rhs)
	{
		return std::equal(lhs.voxels, lhs.voxels + 8, rhs.voxels);
	}
	friend bool operator!=(const voxelgroup& lhs, const voxelgroup& rhs) { return !(lhs == rhs); }
	friend std::ostream& operator<< (std::ostream& stream, const voxelgroup& vg)
	{
		stream << "(";
		for (uint i = 0; i < 8; i++)
			stream << ", " << vg.voxels[i];
		stream << ")";
		return stream;
	}
};

int main( int argc, const char* argv[] )
{
	using voxel = voxelgroup;
	const uint d = 3;
	using PosInt = uint8_t;
	using HashInt = uint8_t;
	using map = psh::map<d, voxel, PosInt, HashInt>;
	using point = psh::point<d, PosInt>;

	PosInt width = 128;
	std::vector<map::data_t> data;
	std::vector<bool> data_b(width * width * width);
	for (PosInt x = 0; x < width; x++)
	{
		for (PosInt y = 0; y < width; y++)
		{
			for (PosInt z = 0; z < width; z++)
			{
				if (rand() % 10 == 0)
				{
					//std::cout << point{x, y, z} << std::endl;
					data.push_back(
						map::data_t{
							point{x, y, z},
							//voxel{uint16_t(x + y + z)}
							voxel{
								uint16_t(x), uint16_t(y), uint16_t(z), uint16_t(x+1),
								uint16_t(y+1), uint16_t(z+1), uint16_t(x+2), uint16_t(y+2)}
						});
					data_b[psh::point_to_index<d>(point{x, y, z}, width, uint(-1))] = true;
				}
			}
		}
	}
	std::cout << "data size: " << data.size() << std::endl;
	std::cout << "data %: " << float(data.size()) / std::pow(width, d) << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	map s([&](size_t i) { return data[i]; }, data.size(), width);
	auto stop_time = std::chrono::high_resolution_clock::now();

	bool failed = false;
	std::cout << "verifying" << std::endl;
	for (auto& element : data)
	{
		voxel found;
		try
		{
			found = s.get(element.location);
		}
		catch (const std::out_of_range& e)
		{
			std::cout << "element not found in hash!" << std::endl;
			std::cout << element.contents << std::endl;
			failed = true;
		}
		if (element.contents != found)
		{
			std::cout << "element different in hash!" << std::endl;
			std::cout << element.contents << std::endl;
			std::cout << found << std::endl;
			failed = true;
		}
	}
	if (failed)
		std::cout << "failed!" << std::endl;
	else
		std::cout << "success!" << std::endl;

	auto original_data_size = width * width * width * (sizeof(voxel) + sizeof(point));
	std::cout << "original data: " << (original_data_size / (1024 * 1024.0f)) << " mb" << std::endl;

	std::cout << "class size: " << std::endl;
	std::cout << s.memory_size() << " bytes" << std::endl;
	std::cout << s.memory_size() / 1024 << " kb" << std::endl;
	std::cout << s.memory_size() / (1024 * 1024.0f) << " mb" << std::endl;

	std::cout << "compression factor vs dense: " << (float(s.memory_size())
		/ (uint(std::pow(width, 3)) * sizeof(voxel))) << std::endl;
	std::cout << "compression factor vs sparse: " << (float(s.memory_size())
		/ (sizeof(data) + sizeof(decltype(data)::value_type) * data.size())) << std::endl;
	std::cout << "compression factor vs optimal: " << (float(s.memory_size())
		/ (sizeof(data) + (sizeof(decltype(data)::value_type) - sizeof(point)) * data.size())) << std::endl;

	std::cout << "map creation time: " << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
		(stop_time - start_time).count() / 1000.0f << " seconds" << std::endl;

	std::cout << "exhaustive test" << std::endl;
	tbb::parallel_for(uint(0), uint(width * width * width), [&](uint i)
		{
			point p = psh::index_to_point<d>(i, width, uint(-1));
			auto exists = data_b[i];
			try
			{
				s.get(p);
				if (!exists)
				{
					std::cout << "found non-existing element!" << std::endl;
					std::cout << p << std::endl;
				}
			}
			catch (const std::out_of_range& e)
			{
				if (exists)
				{
					std::cout << "didn't find existing element!" << std::endl;
					std::cout << p << std::endl;
				}
			}
		});
	std::cout << "finished!" << std::endl;
}
