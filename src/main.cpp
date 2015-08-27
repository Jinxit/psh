#include "psh.hpp"
#include <experimental/optional>
#include <iostream>
#include <chrono>
#include <stdint.h>
#include <thread>

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

void voxel_test()
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
	std::cout << "data density: " << float(data.size()) / std::pow(width, d) << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	map s([&](size_t i) { return data[i]; }, data.size(), width);
	auto stop_time = std::chrono::high_resolution_clock::now();

	auto original_data_size = width * width * width * (sizeof(voxel) + sizeof(point));
	std::cout << "original data: " << (original_data_size / (1024 * 1024.0f)) << " mb" << std::endl;

	std::cout << "class size: " << s.memory_size() / (1024 * 1024.0f) << " mb" << std::endl;

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

void game_of_life_test()
{
	using namespace std::literals;
	using pixel = bool;
	const uint d = 2;
	using PosInt = uint8_t;
	using HashInt = uint8_t;
	using map = psh::map<d, pixel, PosInt, HashInt>;
	using point = psh::point<d, PosInt>;

	PosInt width = 48;
	std::vector<map::data_t> data;
	std::vector<bool> data_b(width * width);
	for (PosInt x = 0; x < width; x++)
	{
		for (PosInt y = 0; y < width; y++)
		{
			if (rand() % 20 == 0)
			{
				data.push_back(
					map::data_t{
						point{x, y},
						pixel{true}
					});
				data_b[psh::point_to_index<d>(point{x, y}, width, uint(-1))] = true;
			}
		}
	}
	std::cout << "data size: " << data.size() << std::endl;
	std::cout << "data density: " << float(data.size()) / std::pow(width, d) << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	map s([&](size_t i) { return data[i]; }, data.size(), width);
	auto stop_time = std::chrono::high_resolution_clock::now();

	auto original_data_size = width * width * width * (sizeof(pixel) + sizeof(point));
	std::cout << "original data: " << (original_data_size / (1024 * 1024.0f)) << " mb" << std::endl;

	std::cout << "class size: " << s.memory_size() / (1024 * 1024.0f) << " mb" << std::endl;

	std::cout << "compression factor vs dense: " << (float(s.memory_size())
		/ (uint(std::pow(width, 3)) * sizeof(pixel))) << std::endl;
	std::cout << "compression factor vs sparse: " << (float(s.memory_size())
		/ (sizeof(data) + sizeof(decltype(data)::value_type) * data.size())) << std::endl;
	std::cout << "compression factor vs optimal: " << (float(s.memory_size())
		/ (sizeof(data) + (sizeof(decltype(data)::value_type) - sizeof(point)) * data.size())) << std::endl;

	std::cout << "map creation time: " << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
		(stop_time - start_time).count() / 1000.0f << " seconds" << std::endl;

	std::cout << "exhaustive test" << std::endl;
	tbb::parallel_for(uint(0), uint(width * width), [&](uint i)
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

	std::unordered_map<psh::point<d, PosInt>, pixel> queue;
	bool add_failed = false;
	auto insert = [&](const psh::point<d, PosInt>& p, bool value)
		{
			if (!add_failed)
			{
				try
				{
					auto& found = s.get(p);
					found = value;
				}
				catch (const std::out_of_range& e)
				{
					if (!s.add(p, value))
					{
						add_failed = true;
						std::cout << "failed.." << std::endl;
					}
				}
			}

			if (add_failed)
			{
				queue[p] = value;
			}
		};
	uint num_rebuilds = 0;
	auto finalize = [&]()
		{
			if (add_failed)
			{
				std::vector<map::data_t> new_data;
				new_data.reserve(queue.size());
				for (auto& kvp : queue)
				{
					new_data.push_back(map::data_t{kvp.first, kvp.second});
				}

				s = s.rebuild([&](size_t i)
					{
						return new_data[i];
					}, queue.size(), data_b);
				queue.clear();
				add_failed = false;
				num_rebuilds++;
			}
			std::cout << num_rebuilds << " rebuilds so far, memory use: "
				<< s.memory_size() / (1024 * 1024.0f) << "mb" << std::endl << std::endl;
		};

	for (uint i = 0; i < 100000000; i++)
	{
		if (i != 0)
		{
			for (PosInt y = 0; y < width; y++)
			{
				for (PosInt x = 0; x < width; x++)
				{
					int num_neighbours = 0;
					for (PosInt x_off = x - 1; x_off <= x + 1; x_off++)
					{
						for (PosInt y_off = y - 1; y_off <= y + 1; y_off++)
						{
							if (x_off == x && y_off == y)
								continue;
							try
							{
								if (s.get(psh::point<d, PosInt>{x_off, y_off}))
									num_neighbours++;
							}
							catch (const std::out_of_range& e)
							{
							}
						}
					}

					auto p = psh::point<d, PosInt>{x, y};
					bool alive = false;
					try
					{
						if (s.get(p))
							alive = true;
					}
					catch (const std::out_of_range& e)
					{
					}

					if (alive)
					{
						if (num_neighbours < 2)
							alive = false;
						else if (num_neighbours > 3)
							alive = false;
					}
					else if (num_neighbours == 3)
						alive = true;
					insert(p, alive);
				}
			}
		}

		for (uint j = 0; j < 20; j++)
			std::cout << std::endl;

		finalize();
		//s = s.rebuild([](size_t i) { return map::data_t(); }, 0, data_b);
		for (PosInt y = 0; y < width; y++)
		{
			std::cout << std::endl;
			for (PosInt x = 0; x < width; x++)
			{
				auto p = psh::point<d, PosInt>{x, y};
				try
				{
					if (s.get(p))
					{
						std::cout << "▮";
					}
					else
						throw std::out_of_range("");
				}
				catch (const std::out_of_range& e)
				{
					std::cout << " ";
				}
			}
		}
		std::cout << std::endl;
		std::this_thread::sleep_for(200ms);
	}
}

int main( int argc, const char* argv[] )
{
	game_of_life_test();
}
