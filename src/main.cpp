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
		auto hashed = s.get(element);
		if (!hashed)
		{
			std::cout << "element not found in hash!" << std::endl;
			std::cout << element << std::endl;
			failed = true;
		}
		else if (element != hashed.value())
		{
			std::cout << "element different in hash!" << std::endl;
			std::cout << element << std::endl;
			std::cout << hashed.value() << std::endl;
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
}