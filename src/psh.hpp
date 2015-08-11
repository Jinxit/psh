#include <string>
#include <functional>
#include <cmath>
#include <random>
#include <experimental/optional>
#include <iostream>
#include <set>
#include <vector>
#include <Eigen/Dense>

#define VALUE(x) std::cout << #x "=" << x << std::endl

namespace psh
{
	using uint = long unsigned int;

	template<uint d, class data_t>
	class set
	{
		static_assert(d > 0, "d must be larger than 0.");
	public:
		using point = Eigen::Matrix<uint, d, 1>;
		using opt_point = std::experimental::optional<point>;

		uint M0;
		uint M1;
		uint n;
		uint m_bar;
		uint m;
		uint r_bar;
		uint r;
		std::vector<point> phi;
		std::vector<opt_point> H;

		set(const std::vector<data_t>& data)
			: n(data.size()), m_bar(std::ceil(std::pow(n, 1.0f / d))), m(std::pow(m_bar, d)),
			  r_bar(std::ceil(std::pow(n / (2 * d), 1.0f / d)) - 1)
		{
			std::default_random_engine gen(time(0));

			std::function<uint(void)> prime;
			{
				// http://planetmath.org/goodhashtableprimes
				const std::vector<uint> primes = { 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289,
					24593, 49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469 };
				std::uniform_int_distribution<uint> prime_dist(0, primes.size() - 1);
				prime = [prime_dist, primes, &gen]() mutable {
					return primes[prime_dist(gen)];
				};
			}

			M0 = prime();
			M1 = prime();

			VALUE(m);
			VALUE(m_bar);

		restart:
			{
				r_bar += d;
				r = std::pow(r_bar, d);
				VALUE(r);
				VALUE(r_bar);

				std::vector<point> phi_hat;
				phi_hat.reserve(r);
				std::vector<opt_point> H_hat(m);
				std::cout << "creating " << r << " buckets" << std::endl;

				{
					auto m_mod_r = m_bar % r_bar;
					if (m_mod_r == 1 || m_mod_r == r - 1)
					{
						std::cout << "prime restart" << std::endl;
						goto restart;
					}
				}

				std::uniform_int_distribution<uint> dist(0, m - 1);

				std::vector<std::vector<point>> buckets(r);

				for (auto& element : data)
				{
					auto h1 = M1 * element;
					buckets[point_to_index(h1, r_bar, r)].push_back(element);
				}

				std::cout << "buckets created, jiggling offsets" << std::endl;

				bool easy = false;
				for (uint i = 0; i < buckets.size(); i++)
				{
					if (buckets.size() / 10 > 0 && i % (buckets.size() / 10) == 0)
						std::cout << (100 * i) / buckets.size() << "% done" << std::endl;
					uint max_index = 0;

					// in hard mode we find the biggest bucket
					uint max_count = 0;
					for (uint j = 0; j < buckets.size(); j++)
					{
						if (buckets[j].size() > max_count
							|| (easy && buckets[j].size() > 0))
						{
							max_count = buckets[j].size();
							max_index = j;

							// in easy mode we grab the first filled bucket
							if (easy)
								break;
						}
					}
					// if the biggest bucket was size 1, switch to easy mode next time
					if (max_count == 1)
						easy = true;
					// if the biggest bucket was zero, we done
					if (max_count == 0)
						break;

					uint possible_offset = dist(gen);
					bool success = false;
					for (uint k = 0; k < m; k++)
					{
						possible_offset = (possible_offset + 1) % m;

						phi_hat[max_index] = index_to_point(possible_offset, m_bar, m);

						bool already_used = false;
						for (uint l = 0; l < buckets[max_index].size(); l++)
						{
							auto hashed = h(buckets[max_index][l], phi_hat);
							if (H_hat[point_to_index(hashed, m_bar, m)])
							{
								already_used = true;
								break;
							}
						}

						if (!already_used)
						{
							success = true;
							for (uint l = 0; l < buckets[max_index].size(); l++)
							{
								auto hashed = h(buckets[max_index][l], phi_hat);
								H_hat[point_to_index(hashed, m_bar, m)] = buckets[max_index][l];
							}
							break;
						}
					}
					if (!success)
					{
						goto restart;
					}
					buckets[max_index].clear();
				}

				std::cout << "done!" << std::endl;
				phi = std::move(phi_hat);
				H = std::move(H_hat);
			}
		}

		constexpr uint point_to_index(const point& p, uint width, uint max) const
		{
			uint index = p[0];
			for (uint i = 1; i < d; i++)
			{
				index += uint(std::pow(width, i)) * p[i];
			}
			return index % max;
		}

		constexpr point index_to_point(uint index, uint width, uint max) const
		{
			point output;
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

		point h(const data_t& p, const std::vector<point>& temp_phi) const
		{
			auto h0 = M0 * p;
			auto h1 = M1 * p;
			auto offset = temp_phi[point_to_index(h1, r_bar, r)];
			return h0 + offset;
		}

		point h(const data_t& p) const
		{
			return h(p, phi);
		}

		point get(const data_t& p) const
		{
			auto maybe_element = H[point_to_index(h(p), m_bar, m)];
			if (maybe_element)
				return maybe_element.value();
			else
				throw std::out_of_range("Element not found in map");
		}
	};
}