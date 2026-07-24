/*! \file MakeNullTerminated.test.cpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Tests for the MakeNullTerminated string helper (utils), exercised through a
		   ThreadLocalMemoryResourcePool arena.
*/

#include "Hush/Memory/ThreadLocalMemoryResourcePool.hpp"
#include "StringAllocation.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string_view>

TEST_CASE("MakeNullTerminated null-terminates an arbitrary view", "[memory][string]")
{
	Hush::Memory::ThreadLocalMemoryResourcePool pool;

	// A view that is deliberately NOT null-terminated: a slice of a larger buffer.
	constexpr std::string_view backing = "hello world";
	const std::string_view slice = backing.substr(0, 5); // "hello", followed by ' ' not '\0'

	const Hush::NullTerminatedStringView z = Hush::MakeNullTerminated(slice, &pool);

	REQUIRE(z.size() == 5);
	REQUIRE(std::string_view(z) == "hello");
	REQUIRE(z.c_str()[z.size()] == '\0');
	// Usable as a C string.
	REQUIRE(std::strlen(z.c_str()) == 5);
	// The copy is independent of the backing storage.
	REQUIRE(z.c_str() != slice.data());
}

TEST_CASE("MakeNullTerminated handles the empty view", "[memory][string]")
{
	Hush::Memory::ThreadLocalMemoryResourcePool pool;

	const Hush::NullTerminatedStringView z = Hush::MakeNullTerminated(std::string_view{}, &pool);

	REQUIRE(z.size() == 0);
	REQUIRE(z.empty());
	REQUIRE(z.c_str() != nullptr);
	REQUIRE(z.c_str()[0] == '\0');
}

TEST_CASE("MakeNullTerminated preserves embedded content and length", "[memory][string]")
{
	Hush::Memory::ThreadLocalMemoryResourcePool pool;

	const std::string_view source = "entity/name/with/slashes";
	const Hush::NullTerminatedStringView z = Hush::MakeNullTerminated(source, &pool);

	REQUIRE(z.size() == source.size());
	REQUIRE(std::string_view(z) == source);
	REQUIRE(std::memcmp(z.c_str(), source.data(), source.size()) == 0);
	REQUIRE(z.c_str()[source.size()] == '\0');
}
