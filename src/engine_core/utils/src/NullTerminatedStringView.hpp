/*! \file NullTerminatedStringView.hpp
	\author Alan Ramirez Herrera
	\date 2026-05-20
	\brief A string view that guarantees null-termination, useful for C APIs and interop scenarios where null-terminated
   strings are required.
*/

#pragma once

#include "Assertions.hpp"
#include <string_view>
#include <cstring>
#include <string>

namespace Hush
{
	class NullTerminatedStringView
	{
	public:
		using value_type = char;
		using const_pointer = const char *;
		using size_type = std::size_t;

		constexpr NullTerminatedStringView() noexcept = default;

		/// Construct from a string literal or any `const char[N]` array.
		///
		/// This matches any const char array, not only string literals — that's the limit
		/// of what the type system can detect at compile time. Arrays passed here are
		/// assumed to be properly null-terminated at index `N - 1`.
		template <std::size_t N>
		constexpr NullTerminatedStringView(const char (&literal)[N]) noexcept
			: m_data(literal),
			  m_size(N - 1)
		{
		}

		/// Disable construction from a non-const char array. Without this, mutable buffers
		/// like `char buf[64]; NullTerminatedStringView z = buf;` would slip through the
		/// literal constructor above and capture whatever happens to be at `buf[strlen(buf)]`.
		template <std::size_t N>
		NullTerminatedStringView(char (&)[N]) = delete;

		/// Construct from a `std::string`. `c_str()` is null-terminated since C++11.
		NullTerminatedStringView(const std::string &s) noexcept
			: m_data(s.c_str()),
			  m_size(s.size())
		{
		}

		/// Explicit, named factory for constructing from a `std::string_view`.
		///
		/// The caller asserts that the byte at `sv.data() + sv.size()` is `'\0'`.
		/// This is checked in debug builds via `HUSH_ASSERT`.
		[[nodiscard]]
		static NullTerminatedStringView promise_null_terminated(std::string_view sv) noexcept
		{
			HUSH_ASSERT(sv.data() != nullptr, "promise_null_terminated requires a non-null string_view");
			HUSH_ASSERT(sv.data()[sv.size()] == '\0',
						"promise_null_terminated requires the byte at data()+size() to be a null terminator");
			return NullTerminatedStringView{sv.data(), sv.size()};
		}

		/// Implicit conversion to the weaker view type.
		constexpr operator std::string_view() const noexcept
		{
			return {m_data, m_size};
		}

		/// Pointer to the null-terminated character data. Never null after construction
		/// from any of the supported sources (empty zstring_view points at `""`).
		[[nodiscard]]
		constexpr const char *c_str() const noexcept
		{
			return m_data;
		}
		[[nodiscard]]
		constexpr const char *data() const noexcept
		{
			return m_data;
		}
		[[nodiscard]]
		constexpr size_type size() const noexcept
		{
			return m_size;
		}
		[[nodiscard]]
		constexpr bool empty() const noexcept
		{
			return m_size == 0;
		}

		[[nodiscard]]
		constexpr const char &operator[](size_type i) const noexcept
		{
			return m_data[i];
		}

		// Equality and ordering delegate to string_view for consistency.
		friend constexpr bool operator==(NullTerminatedStringView a, NullTerminatedStringView b) noexcept
		{
			return std::string_view(a) == std::string_view(b);
		}

		friend constexpr auto operator<=>(NullTerminatedStringView a, NullTerminatedStringView b) noexcept
		{
			return std::string_view(a) <=> std::string_view(b);
		}

	private:
		constexpr NullTerminatedStringView(const char *data, size_type size) noexcept
			: m_data(data),
			  m_size(size)
		{
		}

		// Empty zstring_view still satisfies the invariant: points at a static "".
		static constexpr const char EMPTY_LITERAL[] = "";
		const char *m_data = EMPTY_LITERAL;
		size_type m_size = 0;
	};
} // namespace Hush