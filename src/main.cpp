#include "psh.hpp"
#include <experimental/optional>
#include <iostream>

int main( int argc, const char* argv[] )
{
	using data_t = Eigen::Matrix<long unsigned int, 3, 1>;
	using set = psh::set<3, data_t>;

	std::vector<data_t> data;
	int width = 128;
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < width; y++)
		{
			for (int z = 0; z < width; z++)
			{
				if (rand() % 20 == 0)
				{
					if (x == 10 && y == 10 && z == 10)
						continue;
					data.emplace_back(x, y, z);
				}
			}
		}
	}
	std::cout << "data size: " << data.size() << std::endl;

	set s(data);

	bool failed = false;
	std::cout << "verifying" << std::endl;
	for (auto& element : data)
	{
		data_t found;
		try
		{
			found = s.get(element);
		}
		catch (const std::out_of_range& e)
		{
			std::cout << "element not found in hash!" << std::endl;
			std::cout << element << std::endl;
			failed = true;
		}
		if (element != found)
		{
			std::cout << "element different in hash!" << std::endl;
			std::cout << element << std::endl;
			std::cout << found << std::endl;
			failed = true;
		}
	}
	if (failed)
		std::cout << "failed!" << std::endl;
	else
		std::cout << "success!" << std::endl;

	std::cout << "original data: " << width << " * " << width << " = " << width * width << std::endl;
	std::cout << "m_bar: " << s.m_bar << " * " << s.m_bar << " = " << s.m_bar * s.m_bar << std::endl;
	std::cout << "r_bar: " << s.r_bar << " * " << s.r_bar << " = " << s.r_bar * s.r_bar << std::endl;
	std::cout << "compression factor: " << float(s.r_bar * s.r_bar + s.m_bar * s.m_bar)
		/ (width * width) << " (less is better)" << std::endl;

	try
	{
		auto found = s.get(data_t(10, 10, 10));
		std::cout << "found non-existing element!" << std::endl;
		VALUE(found);
	}
	catch (const std::out_of_range& e)
	{
		std::cout << "element not found, as expected" << std::endl;
	}

	std::cout << "class size: " << std::endl;
	std::cout << s.memory_size() << " bytes" << std::endl;
	std::cout << s.memory_size() / 1024 << " kb" << std::endl;
	std::cout << s.memory_size() / (1024 * 1024) << " mb" << std::endl;
}
