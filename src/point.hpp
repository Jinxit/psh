#pragma once

namespace psh
{
	template<uint d>
	struct point
	{
		uint data[d]{0};

		constexpr uint operator[](uint i) const { return data[i]; }
		uint& operator[](uint i) { return data[i]; }

		template<class F>
		friend point operator*(const point& p, F scalar)
		{
			point output = p;
			for (uint i = 0; i < d; i++)
				output[i] *= scalar;
			return output;
		}
		template<class F>
		friend point operator*(F scalar, const point& p)
		{
			return p * scalar;
		}

		template<class F>
		friend point operator+(const point& p, F scalar)
		{
			point output = p;
			for (uint i = 0; i < d; i++)
				output[i] += scalar;
			return output;
		}
		template<class F>
		friend point operator+(F scalar, const point& p)
		{
			return p + scalar;
		}

		friend point operator+(const point& lhs, const point& rhs)
		{
			point output = lhs;
			for (uint i = 0; i < d; i++)
				output[i] += rhs[i];
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
