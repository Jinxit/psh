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
		stream << "(" << d.r << ", " << d.g << ", " << d.b << ")" << std::endl;
		return stream;
	}
};

int main( int argc, const char* argv[] )
{
	using map = psh::map<3, dumb>;

	std::vector<map::data_t> data;
	uint width = 256;
	for (uint x = 0; x < width; x++)
	{
		for (uint y = 0; y < width; y++)
		{
			for (uint z = 0; z < width; z++)
			{
				if (rand() % 10 == 0)
				{
					if (x == 10 && y == 10 && z == 10)
						continue;
					data.push_back(
						map::data_t{
							map::point(x, y, z),
							dumb{float(x) / width, float(y) / width, float(1) / width}
						});
				}
			}
		}
	}
	std::cout << "data size: " << data.size() << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	map s(data);
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

	auto original_data_size = width * width * width * (sizeof(dumb) + sizeof(map::point));
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

	try
	{
		auto found = s.get(map::point{10, 10, 10});
		std::cout << "found non-existing element!" << std::endl;
		VALUE(found);
	}
	catch (const std::out_of_range& e)
	{
		std::cout << "element not found, as expected" << std::endl;
	}

	std::cout << "map creation time: " << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>
		(stop_time - start_time).count() / 1000.0f << " seconds" << std::endl;
}
