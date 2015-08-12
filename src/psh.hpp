#include <string>
#include <functional>
#include <cmath>
#include <random>
#include <experimental/optional>
#include <iostream>
#include <set>
#include <vector>
#include <utility>
#include <Eigen/Dense>
#include "tbb/parallel_sort.h"

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

		struct bucket : public std::vector<point>
		{
			uint phi_index;

			bucket(uint phi_index) : phi_index(phi_index) { }
			friend bool operator<(const bucket& lhs, const bucket& rhs) {
				return lhs.size() > rhs.size();
			}
		};

		uint M0;
		uint M1;
		uint n;
		uint m_bar;
		uint m;
		uint r_bar;
		uint r;
		std::vector<point> phi;
		std::vector<opt_point> H;
		std::default_random_engine generator;

		set(const std::vector<data_t>& data)
			: n(data.size()), m_bar(std::ceil(std::pow(n, 1.0f / d))), m(std::pow(m_bar, d)),
			  r_bar(std::ceil(std::pow(n / d, 1.0f / d)) - 1), generator(time(0))
		{
			M0 = prime();
			while ((M1 = prime()) == M0);

			VALUE(m);
			VALUE(m_bar);

			bool create_succeeded = false;

			std::uniform_int_distribution<uint> m_dist(0, m - 1);

			do
			{
				r_bar += d;
				r = std::pow(r_bar, d);
				VALUE(r);
				VALUE(r_bar);

				create_succeeded = create(data, m_dist);

			} while (!create_succeeded);
		}

		uint prime()
		{
			static const std::vector<uint> primes{ 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289,
				24593, 49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469 };
			static std::uniform_int_distribution<uint> prime_dist(0, primes.size() - 1);

			return primes[prime_dist(generator)];
		}

		bool bad_m_r()
		{
			auto m_mod_r = m_bar % r_bar;
			return m_mod_r == 1 || m_mod_r == r - 1;
		}

		void insert(const bucket& b, decltype(H)& H_hat, const decltype(phi)& phi_hat)
		{
			for (auto& element : b)
			{
				auto hashed = h(element, phi_hat);
				H_hat[point_to_index(hashed, m_bar, m)] = element;
			}
		}

		bool jiggle_offsets(decltype(H)& H_hat, decltype(phi)& phi_hat,
			const bucket& b, std::uniform_int_distribution<uint>& m_dist)
		{
			uint possible_offset = m_dist(generator);
			for (uint k = 0; k < m; k++)
			{
				possible_offset = (possible_offset + 1) % m;

				phi_hat[b.phi_index] = index_to_point(possible_offset, m_bar, m);

				bool collision = false;
				for (auto& element : b)
				{
					if (contains(element, H_hat, phi_hat))
					{
						collision = true;
						break;
					}
				}

				if (!collision)
				{
					insert(b, H_hat, phi_hat);
					return true;
				}
			}
			return false;
		}

		std::vector<bucket> create_buckets(const std::vector<data_t>& data)
		{
			std::vector<bucket> buckets;
			buckets.reserve(r);
			{
				uint i = 0;
				std::generate_n(std::back_inserter(buckets), r, [&] {
						return bucket(i++);
					});
			}

			for (auto& element : data)
			{
				auto h1 = M1 * element;
				buckets[point_to_index(h1, r_bar, r)].push_back(element);
			}

			std::cout << "buckets created" << std::endl;

			tbb::parallel_sort(buckets.begin(), buckets.end());
			std::cout << "buckets sorted" << std::endl;

			return buckets;
		}

		bool create(const std::vector<data_t>& data, std::uniform_int_distribution<uint>& m_dist)
		{
			decltype(phi) phi_hat;
			phi_hat.reserve(r);
			decltype(H) H_hat(m);
			std::cout << "creating " << r << " buckets" << std::endl;

			if (bad_m_r())
				return false;

			auto buckets = create_buckets(data);
			std::cout << "jiggling offsets" << std::endl;

			for (uint i = 0; i < buckets.size(); i++)
			{
				if (buckets.size() == 0)
					break;
				if (i % (buckets.size() / 10) == 0)
					std::cout << (100 * i) / buckets.size() << "% done" << std::endl;

				if (!jiggle_offsets(H_hat, phi_hat, buckets[i], m_dist))
				{
					return false;
				}
			}

			std::cout << "done!" << std::endl;
			phi = std::move(phi_hat);
			H = std::move(H_hat);
			return true;
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

		point h(const data_t& p, const decltype(phi)& phi_hat) const
		{
			auto h0 = M0 * p;
			auto h1 = M1 * p;
			auto offset = phi_hat[point_to_index(h1, r_bar, r)];
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

		bool contains(const data_t& p, const decltype(H)& H_hat, const decltype(phi)& phi_hat) const
		{
			return bool(H_hat[point_to_index(h(p, phi_hat), m_bar, m)]);
		}

		bool contains(const data_t& p) const
		{
			return contains(p, H, phi);
		}

		bool contains(uint index, const decltype(H)& H_hat) const
		{
			return bool(H_hat[index]);
		}

		bool contains(uint index) const
		{
			return contains(index, H);
		}

		uint memory_size()
		{
			return sizeof(*this) + sizeof(typename decltype(phi)::value_type) * phi.size()
				+ sizeof(typename decltype(H)::value_type) * H.size();
		}
	};
}