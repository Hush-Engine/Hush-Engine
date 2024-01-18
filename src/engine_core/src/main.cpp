#include "ScriptingManager.hpp"
#include <iostream>
#include <map>
#if WIN32
constexpr std::string_view DOTNET_PATH = "C:/Program Files/dotnet/";
#elif __APPLE__
constexpr std::string_view DOTNET_PATH = "/usr/local/share/dotnet";
#else
constexpr std::string_view DOTNET_PATH = "/usr/share/dotnet";
#endif

constexpr std::string_view ASSEMBLY_TEST = "assembly-test";

#if DEBUG
static int allocCntr = 0;

void* operator new(size_t size) {
	allocCntr++;
	LOG_DEBUG_LN("Allocated %zu bytes, current count: %d", size, allocCntr);
	return malloc(size);
}

void operator delete(void* p) {
	allocCntr--;
	LOG_DEBUG_LN("Deallocating a pointer, current count: %d", allocCntr);
	free(p);
}

#endif

int main()
{
	std::shared_ptr<DotnetHost> host = std::make_shared<DotnetHost>(DOTNET_PATH.data());
	LOG_INFO_LN("Just finished initializing our host!");
	//Demo stuff
	auto scriptManager = ScriptingManager(host, ASSEMBLY_TEST);
	LOG_INFO_LN("Now finished initializing our script manager");
	const char* testNamespace = "Test";
	const char* testClass = "Class1";
	const char* testFunc = "SumTest";
	for (size_t i = 0; i < 90000; i++)
	{
		char* result = scriptManager.InvokeCSharpWithReturn<char*>(testNamespace, testClass, "GetCsharpString");
		LOG_INFO_LN("C++ thinks the string is: %s", result);
		Sleep(100);
	}
	return 0;
}
