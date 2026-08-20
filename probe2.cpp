#include <fastgltf/core.hpp>
#include <cstdio>
int main()
{
	std::printf("sizeof=%zu align=%zu\n", sizeof(fastgltf::Asset), alignof(fastgltf::Asset));
	return 0;
}
