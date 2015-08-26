#pragma once

#include <string>
#include <functional>
#include <cmath>
#include <random>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <utility>
#include <thread>
#include "tbb/parallel_sort.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_for_each.h"
#include "tbb/mutex.h"
#include "tbb/pipeline.h"
#include "util.hpp"
#include "point.hpp"

#define VALUE(x) std::cout << #x "=" << x << std::endl

namespace psh
{
	// creates a perfect hash for a predefined data set
	// d is the dimensionality, T is the data type
	// PosInt is the integer type used for positions
	// HashInt is the integer type used for the position hash
	template<uint d, class T, class PosInt, class HashInt>
	class map
	{
		static_assert(d > 0, "d must be larger than 0.");
		using IndexInt = size_t;
		class bucket;
		class entry;
		class entry_large;

		// three primes for use in hashing
		IndexInt M0;
		IndexInt M1;
		IndexInt M2;
		// number of data points
		IndexInt n;
		// width of the hash table
		PosInt m_bar;
		// size of the hash table
		IndexInt m;
		// width of the offset table
		PosInt r_bar;
		// size of the offset table
		IndexInt r;
		// offset table
		std::vector<point<d, PosInt>> phi;
		std::vector<entry> H;
		std::default_random_engine generator;

	public:
		struct data_t
		{
			point<d, PosInt> location;
			T contents;
		};
		using data_function = std::function<data_t(IndexInt)>;

		// data_function maps an index to a data point, n is the total number of data points,
		// domain_width is the limit of the domain in each dimension
		map(const data_function& data, IndexInt n, PosInt domain_width)
			: n(n), m_bar(std::ceil(std::pow(n, 1.0f / d))), m(std::pow(m_bar, d)),
			  r_bar(std::ceil(std::pow(n / d, 1.0f / d)) - 1), generator(time(0))
		{
			// generate primes, M0 must be different from M1
			M0 = prime();
			while ((M1 = prime()) == M0);
			M2 = prime();

			VALUE(m);
			VALUE(uint(m_bar));

			VALUE(M0);
			VALUE(M1);
			VALUE(M2);

			std::uniform_int_distribution<IndexInt> m_dist(0, m - 1);

			bool create_succeeded = false;
			do
			{
				// if we fail, we try again with a larger offset table
				r_bar += d;
				r = std::pow(r_bar, d);
				VALUE(r);
				VALUE(uint(r_bar));

				create_succeeded = create(data, domain_width, m_dist);

			} while (!create_succeeded);
		}

		T get(const point<d, PosInt>& p) const
		{
			// find where the element would be located
			auto i = point_to_index(h(p), m_bar, m);
			// but also check that they are equal (have the same positional hash)
			if (H[i].equals(p, M2))
				return H[i].contents;
			else
				throw std::out_of_range("Element not found in map");
		}

		size_t memory_size() const
		{
			return sizeof(*this)
				+ sizeof(typename decltype(phi)::value_type) * phi.capacity()
				+ sizeof(typename decltype(H)::value_type) * H.capacity();
		}

	private:
		// internal data structures

		// a bucket is simply a vector<data_t> which is then sorted
		// by the original phi_index (descending order)
		struct bucket : public std::vector<data_t>
		{
			IndexInt phi_index;

			bucket(IndexInt phi_index) : phi_index(phi_index) { }
			friend bool operator<(const bucket& lhs, const bucket& rhs) {
				return lhs.size() > rhs.size();
			}
		};

		// data type for each entry in the hash table
		struct entry
		{
			// stored data
			T contents;
			// parameter for the hash function
			HashInt k;
			// result of the hash function
			HashInt hk;

			entry() : contents(T()), k(1), hk(1) { };
			entry(const data_t& data, IndexInt M2) : contents(data.contents), k(1) 
			{
				rehash(data, M2);
			}

			static constexpr HashInt h(const point<d, PosInt>& p, IndexInt M2, HashInt k)
			{
				// p dot {k, k * k, k * k * k...} * M2
				// creds to David
				return p * point<d, PosInt>::increasing_pow(k) * M2;
			}

			void rehash(const point<d, PosInt>& location, IndexInt M2, HashInt new_k = 1)
			{
				k = new_k;
				hk = h(location, M2, k);
			}
			void rehash(const data_t& data, IndexInt M2, HashInt new_k = 1)
			{
				rehash(data.location, M2, new_k);
			}

			constexpr bool equals(const point<d, PosInt>& p, IndexInt M2) const
			{
				return hk == h(p, M2, k);
			}
		};

		// a larger version of entry to also keep the original location
		// later discarded (using slicing!) for memory efficiency
		struct entry_large : public entry
		{
			point<d, PosInt> location;
			entry_large() : entry(), location(point<d, PosInt>()) { };
			entry_large(const data_t& data, IndexInt M2)
				: entry(data, M2), location(data.location) { };
			void rehash(IndexInt M2) { entry::rehash(location, M2, entry::k + 1); };
		};

		// internal functions

		// provides the index in the hash table for a given position in the domain,
		// optionally with a temporary offset table
		point<d, PosInt> h(const point<d, PosInt>& p, const decltype(phi)& phi_hat) const
		{
			auto h0 = p * M0;
			auto h1 = p * M1;
			auto i = point_to_index(h1, r_bar, r);
			auto offset = phi_hat[i];
			return h0 + offset;
		}
		point<d, PosInt> h(const point<d, PosInt>& p) const
		{
			return h(p, phi);
		}

		// returns a random prime from a small predefined list
		IndexInt prime()
		{
			static const std::vector<IndexInt> primes{ 53, 97, 193, 389, 769, 1543, 3079,
				6151, 12289, 24593, 49157, 98317, 196613, 393241, 786433, 1572869,
				3145739, 6291469 };
			static std::uniform_int_distribution<IndexInt> prime_dist(0, primes.size() - 1);

			return primes[prime_dist(generator)];
		}

		// tried to create the hash table given a certain offset table size
		bool create(const data_function& data, PosInt domain_width,
			std::uniform_int_distribution<IndexInt>& m_dist)
		{
			// _hats are temporary variables, later moved into the real vectors
			decltype(phi) phi_hat(r);
			std::vector<entry_large> H_hat(m);
			// lookup for whether a certain slot in the hash table contains an entry
			std::vector<bool> H_b_hat(m, false);
			std::cout << "creating " << r << " buckets" << std::endl;

			if (bad_m_r())
				return false;

			// find out what order we should do the hashing to optimize success rate
			auto buckets = create_buckets(data);
			std::cout << "jiggling offsets" << std::endl;

			for (IndexInt i = 0; i < buckets.size(); i++)
			{
				// if a bucket is empty, then the rest will also be empty
				if (buckets[i].size() == 0)
					break;
				if (i % (buckets.size() / 10) == 0)
					std::cout << (100 * i) / buckets.size() << "% done" << std::endl;

				// try to jiggle the offsets until an injective mapping is found
				if (!jiggle_offsets(H_hat, H_b_hat, phi_hat, buckets[i], m_dist))
				{
					return false;
				}
			}

			std::cout << "done!" << std::endl;
			phi = std::move(phi_hat);
			if (!hash_positions(data, domain_width, H_hat))
				return false;
			H.reserve(H_hat.size());
			std::copy(H_hat.begin(), H_hat.end(), std::back_inserter(H));

			return true;
		}

		// certain values for m_bar and r_bar are bad, empirically found to be if:
		// m_bar is coprime with r_bar <==> gcd(m_bar, r_bar) != 1 <==> m_bar % r_bar ∈ {1, r_bar - 1}
		// creds to Euclid
		bool bad_m_r()
		{
			auto m_mod_r = m_bar % r_bar;
			return m_mod_r == 1 || m_mod_r == r_bar - 1;
		}

		// creates buckets, each buckets corresponds to one entry in the offset table
		// they are then sorted by their index in the offset table so we can assign
		// the largest buckets first
		std::vector<bucket> create_buckets(const data_function& data)
		{
			std::vector<bucket> buckets;
			buckets.reserve(r);
			{
				IndexInt i = 0;
				std::generate_n(std::back_inserter(buckets), r, [&] {
						return bucket(i++);
					});
			}

			for (IndexInt i = 0; i < n; i++)
			{
				auto element = data(i);
				auto h1 = element.location * M1;
				buckets[point_to_index(h1, r_bar, r)].push_back(element);
			}

			std::cout << "buckets created" << std::endl;

			tbb::parallel_sort(buckets.begin(), buckets.end());
			std::cout << "buckets sorted" << std::endl;

			return buckets;
		}

		// jiggle offsets to avoid collisions
		bool jiggle_offsets(std::vector<entry_large>& H_hat, std::vector<bool>& H_b_hat,
			decltype(phi)& phi_hat, const bucket& b,
			std::uniform_int_distribution<IndexInt>& m_dist)
		{
			// start at a random point
			auto start_offset = m_dist(generator);

			bool found = false;
			point<d, PosInt> found_offset;
			tbb::mutex mutex;

			IndexInt chunk_index = 0;
			const IndexInt num_cores = std::thread::hardware_concurrency();
			const IndexInt group_size = r / num_cores + 1;

			tbb::parallel_pipeline(num_cores,
				// a serial filter picks up (num_offsets / num_cores) indices
				tbb::make_filter<void, IndexInt>(tbb::filter::serial,
					[&, group_size](tbb::flow_control& fc) {
						if (found || chunk_index >= r)
						{
							fc.stop();
						}
						chunk_index += group_size;
						return chunk_index;
					}) &
				// and runs each chunk in parallel
				tbb::make_filter<IndexInt, void>(tbb::filter::parallel,
					[&, group_size](IndexInt i0)
					{
						for (IndexInt i = i0; i < i0 + group_size && !found; i++)
						{
							// wrap around m to stay inside the table
							auto phi_offset = index_to_point<d>((start_offset + i) % m, m_bar, m);

							bool collision = false;
							for (auto& element : b)
							{
								auto h0 = element.location * M0;
								auto h1 = element.location * M1;
								auto index = point_to_index(h1, r_bar, r);
								// use existing offsets for others, but if the current index
								// is the one we're jiggling, we use the temporary offset
								auto offset = index == b.phi_index ? phi_offset : phi_hat[index];
								auto hash = h0 + offset;

								// if the index is already used, this offset is invalid
								collision = H_b_hat[point_to_index(hash, m_bar, m)];
								if (collision)
									break;
							}

							// if there were no collisions, we succeeded
							if (!collision)
							{
								// lock out other threads
								tbb::mutex::scoped_lock lock(mutex);
								if (!found)
								{
									// this MIGHT get overwritten more than once, but that's okay,
									// we only need one to succeed, not specifically the first
									found = true;
									found_offset = phi_offset;
								}
							}
						}
					})
				);
			if (found)
			{
				// if we found a valid offset, insert it
				phi_hat[b.phi_index] = found_offset;
				insert(b, H_hat, H_b_hat, phi_hat);
				return true;
			}
			return false;
		}

		// permanently inserts a bucket into a temporary hash table
		void insert(const bucket& b, std::vector<entry_large>& H_hat, std::vector<bool>& H_b_hat,
			const decltype(phi)& phi_hat)
		{
			for (auto& element : b)
			{
				auto hashed = h(element.location, phi_hat);
				auto i = point_to_index(hashed, m_bar, m);
				H_hat[i] = entry_large(element, M2);
				// mark off the slot as used
				H_b_hat[i] = true;
			}
		}

		bool hash_positions(const data_function& data, PosInt domain_width,
			std::vector<entry_large>& H_hat)
		{
			// domain_width - 1 to get the highest indices in each direction
			// width is assumed to be equal in all directions
			// and then add 1 to get the size, not the index
			IndexInt domain_i_max = point_to_index(
				point<d, PosInt>::repeating(domain_width) - PosInt(1),
				domain_width, IndexInt(-1)) + 1;

			tbb::mutex mutex;

			// in the first sweep we go through all points in the domain without a data entry
			std::vector<bool> indices(m, false);
			{
				std::vector<bool> data_b(domain_i_max);
				for (IndexInt i = 0; i < n; i++)
				{
					data_b[point_to_index(data(i).location, domain_width, domain_i_max)] = true;
				}
				tbb::parallel_for(IndexInt(0), domain_i_max, [&](IndexInt i)
					{
						if (data_b[i])
						{
							return;
						}

						auto p = index_to_point<d, PosInt>(i, domain_width, domain_i_max);
						auto l = point_to_index(h(p), m_bar, m);

						// if their position hash collides with the existing element..
						if (H_hat[l].hk == entry::h(p, M2, 1))
						{
							// ..remember the index
							indices[l] = true;
						}
					});
				std::cout << "data size: " << n << std::endl;
				std::cout << "indices size: " << indices.size() << std::endl;
			}

			// in the second sweep we go through the stored indices, and
			// remember all points in the domain that map to that same index,
			// regardless of whether that point has data or not
			std::unordered_map<IndexInt, std::vector<IndexInt>> collisions;
			tbb::parallel_for(IndexInt(0), domain_i_max, [&](IndexInt i)
				{
					// for each point p in original image

					auto p = index_to_point<d, PosInt>(i, domain_width, domain_i_max);
					auto l = point_to_index(h(p), m_bar, m);

					// collect everyone that maps to the same thing
					if (indices[l])
					{
						tbb::mutex::scoped_lock lock(mutex);
						collisions[l].push_back(i);
					}
				});

			// in the third sweep we try to change the positional hash parameter until it works
			bool success = true;
			tbb::parallel_for_each(collisions.begin(), collisions.end(),
				[&](const decltype(collisions)::value_type& kvp)
				{
					if (!fix_k(H_hat[kvp.first], kvp.second, domain_width))
					{
						tbb::mutex::scoped_lock lock(mutex);
						success = false;
					}
				});
			return success;
		}

		// try all values for the positional hash parameter until it works 
		bool fix_k(entry_large& H_entry, const std::vector<IndexInt>& collisions, PosInt domain_width)
		{
			H_entry.rehash(M2);
			// if k == 0, we've rolled around and already tried all the values
			if (H_entry.k == 0)
				return false;

			bool success = true;
			for (IndexInt i : collisions)
			{
				// i is the index in the domain
				// fail if one of these have the same positional hash as the entry in the hash table
				auto p = index_to_point<d, PosInt>(i, domain_width, IndexInt(-1));
				auto hk = entry::h(p, M2, H_entry.k);
				if (H_entry.location != p && H_entry.hk == hk)
				{
					success = false;
					break;
				}
			}
			// if we didn't find a valid k, recursively move on to the next k
			if (!success)
				return fix_k(H_entry, collisions, domain_width);
			return true;
		}
	};
}
