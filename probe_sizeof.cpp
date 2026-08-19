#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <cstdio>
#include <cstddef>

int main()
{
	std::printf("sizeof(fastgltf::Asset) = %zu\n", sizeof(fastgltf::Asset));
	std::printf("alignof(fastgltf::Asset) = %zu\n", alignof(fastgltf::Asset));
	return 0;
}
