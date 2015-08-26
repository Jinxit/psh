#pragma once

namespace psh
{
	template<uint d, class Scalar>
	struct point
	{
		Scalar data[d]{0};

		template<class F>
		explicit operator point<d, F>() const
		{
			point<d, F> output;
			for (uint i = 0; i < d; i++)
			{
				output[i] = F(data[i]);
			}
			return output;
		}

		constexpr Scalar operator[](uint i) const { return data[i]; }
		Scalar& operator[](uint i) { return data[i]; }

		// returns {0, 1, 2..., d}
		static constexpr point increasing_linear()
		{
			point output;
			for (uint i = 1; i < d; i++)
			{
				output[i] = i;
			}
			return output;
		}

		// returns {k, k * k, k * k * k..., k^(d+1)}
		static constexpr point increasing_pow(Scalar k)
		{
			point output;
			output[0] = k;
			for (uint i = 1; i < d; i++)
			{
				output[i] = output[i - 1] * k;
			}
			return output;
		}

		// returns {k, k, k...}
		static constexpr point repeating(Scalar k)
		{
			point output;
			for (uint i = 0; i < d; i++)
			{
				output[i] = k;
			}
			return output;
		}

		template<class F>
		friend point<d, F> operator*(const point& p, F other)
		{
			point<d, F> output;
			for (uint i = 0; i < d; i++)
				output[i] = F(p[i]) * other;
			return output;
		}
		friend uint operator*(const point& lhs, const point& rhs)
		{
			uint output = 0;
			for (uint i = 0; i < d; i++)
				output += lhs[i] * rhs[i];
			return output;
		}

		template<class F>
		friend point<d, F> operator+(const point& p, F other)
		{
			point<d, F> output;
			for (uint i = 0; i < d; i++)
				output[i] += F(p[i]) + other;
			return output;
		}
		friend point operator+(const point& lhs, const point& rhs)
		{
			point output = lhs;
			for (uint i = 0; i < d; i++)
				output[i] += rhs[i];
			return output;
		}
		template<class F>
		friend point<d, F> operator+(const point<d, Scalar>& lhs, const point<d, F>& rhs)
		{
			point<d, F> output = rhs;
			for (uint i = 0; i < d; i++)
				output[i] += lhs[i];
			return output;
		}

		template<class F>
		friend point<d, F> operator-(const point& p, F other)
		{
			point<d, F> output = p;
			for (uint i = 0; i < d; i++)
				output[i] -= other;
			return output;
		}

		friend bool operator==(const point& lhs, const point& rhs)
		{
			for (uint i = 0; i < d; i++)
				if (lhs[i] != rhs[i])
					return false;
			return true;
		}
		friend bool operator!=(const point& lhs, const point& rhs)
		{
			return !(lhs == rhs);
		}

		friend std::ostream& operator<< (std::ostream& stream, const point& p)
		{
			stream << "(";
			for (uint i = 0; i < d; i++)
			{
				stream << p.data[i];
				if (i != d - 1)
					stream << ", ";
			}
			stream << ")";
			return stream;
		}
	};
}
