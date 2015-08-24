#include "psh.hpp"
#include <experimental/optional>
#include <iostream>
#include <chrono>

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

int main( int argc, const char* argv[] )
{
	const uint d = 3;
	using map = psh::map<d, dumb>;

	uint width = 400;
	std::vector<map::data_t> data;
	std::vector<bool> data_b(width * width * width);
	for (uint x = 0; x < width; x++)
	{
		for (uint y = 0; y < width; y++)
		{
			for (uint z = 0; z < width; z++)
			{
				if (rand() % 10 == 0)
				{
					//std::cout << psh::point<d>{x, y, z} << std::endl;
					if (x == 10 && y == 10 && z == 10)
						continue;
					data.push_back(
						map::data_t{
							psh::point<d>{x, y, z},
							dumb{float(x) / width, float(y) / width, float(z) / width}
						});
					data_b[psh::point_to_index<d>(psh::point<d>{x, y, z}, width, uint(-1))] = true;
				}
			}
		}
	}
	std::cout << "data size: " << data.size() << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	map s(data, psh::point<d>{width, width, width});
	auto stop_time = std::chrono::high_resolution_clock::now();

	bool failed = false;
	std::cout << "verifying" << std::endl;
	for (auto& element : data)
	{
		dumb found;
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

	auto original_data_size = width * width * width * (sizeof(dumb) + sizeof(psh::point<d>));
	std::cout << "original data: " << (original_data_size / (1024 * 1024.0f)) << " mb" << std::endl;
	std::cout << "m_bar: " << s.m_bar << " ^ 3 = " << uint(std::pow(s.m_bar, 3)) << std::endl;
	std::cout << "r_bar: " << s.r_bar << " ^ 3 = " << uint(std::pow(s.r_bar, 3)) << std::endl;

	std::cout << "class size: " << std::endl;
	std::cout << s.memory_size() << " bytes" << std::endl;
	std::cout << s.memory_size() / 1024 << " kb" << std::endl;
	std::cout << s.memory_size() / (1024 * 1024.0f) << " mb" << std::endl;

	std::cout << "compression factor vs dense: " << (float(s.memory_size())
		/ (uint(std::pow(width, 3)) * sizeof(dumb))) << std::endl;
	std::cout << "compression factor vs sparse: " << (float(s.memory_size())
		/ (sizeof(data) + sizeof(decltype(data)::value_type) * data.size())) << std::endl;
	std::cout << "compression factor vs optimal: " << (float(s.memory_size())
		/ (sizeof(data) + (sizeof(decltype(data)::value_type) - sizeof(psh::point<d>)) * data.size())) << std::endl;

	try
	{
		auto found = s.get(psh::point<d>{10, 10, 10});
		std::cout << "found non-existing element!" << std::endl;
		VALUE(found);
		auto hash_found = s.h(psh::point<d>{uint(found.r), uint(found.g), uint(found.b)});
		hash_found = psh::index_to_point<d>(psh::point_to_index<3>(hash_found, s.m_bar, s.m), s.m_bar, s.m);
		VALUE(hash_found);
		auto hash_nonexisting = s.h(psh::point<d>{10, 10, 10});
		hash_nonexisting = psh::index_to_point<d>(psh::point_to_index<3>(hash_nonexisting, s.m_bar, s.m), s.m_bar, s.m);
		VALUE(hash_nonexisting);
	}
	catch (const std::out_of_range& e)
	{
		std::cout << "element not found, as expected" << std::endl;
	}

	std::cout << "map creation time: " << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
		(stop_time - start_time).count() / 1000.0f << " seconds" << std::endl;

	std::cout << "exhaustive test" << std::endl;
	for (uint x = 0; x < width; x++)
	{
		for (uint y = 0; y < width; y++)
		{
			for (uint z = 0; z < width; z++)
			{
				auto exists = data_b[psh::point_to_index(psh::point<d>{x, y, z}, width, uint(-1))];
				try
				{
					s.get(psh::point<d>{x, y, z});
					if (!exists)
					{
						std::cout << "found non-existing element!" << std::endl;
						std::cout << psh::point<d>{x, y, z} << std::endl;
						std::cout << map::entry(map::data_t{psh::point<d>{x, y, z},
							dumb{float(x) / width, float(y) / width, float(z) / width}},
							s.M2).hk << std::endl;
					}
				}
				catch (const std::out_of_range& e)
				{
					if (exists)
						std::cout << "didn't find existing element!" << std::endl;
				}
			}
		}
	}
}
