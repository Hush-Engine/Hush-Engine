#define DEBUG 1
#include "ScriptingManager.hpp"
#include <iostream>

#if WIN32
constexpr std::string_view DOTNET_PATH = "C:/Program Files/dotnet/";
#elif __APPLE__
constexpr std::string_view DOTNET_PATH = "/usr/local/share/dotnet";
#else
constexpr std::string_view DOTNET_PATH = "/usr/share/dotnet";
#endif

constexpr std::string_view ASSEMBLY_TEST = "assembly-test";

struct DemoStruct
{
	int32_t a;
	int32_t b;
};

int main()
{
	LOG_INFO_LN("Heeeey, this is an info log");
	LOG_DEBUG_LN("Hey, this should only show up to the devs, y'all are handsome ;)");
	LOG_WARN_LN("This is a warning, you should probably reconsider how you coded this");
	LOG_ERROR_LN("Oh, no, an error! D:");
	std::shared_ptr<DotnetHost> host = std::make_shared<DotnetHost>(DOTNET_PATH.data());
	//Demo stuff
	auto scriptManager = ScriptingManager(host, ASSEMBLY_TEST);
	const char* testNamespace = "Test";
	const char* testClass = "Class1";
	const char* testFunc = "SumTest";


	for (int i = 0; i < 10; i++)
	{
		char* result = scriptManager.InvokeCSharpWithReturn<char*>(testNamespace, testClass, "GetCsharpString");
		LOG_INFO_LN("C++ thinks the string is: %s", result);
	}
	return 0;
}
