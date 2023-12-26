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

	//Demo stuff
	auto scriptManager = ScriptingManager(DOTNET_PATH.data());
	const char* assembly = "assembly-test";
	const char* testNamespace = "Test";
	const char* testClass = "Class1";
	const char* testFunc = "SumTest";


	for (int i = 0; i < 10; i++)
	{
		float result = scriptManager.InvokeCSharpWithReturn<float>(assembly, testNamespace, testClass, "SqrRootTest", (float)i, 0.5f);
		std::cout << result << std::endl;
	}

	return 0;
}
