#include "ScriptingManager.hpp"
#include <iostream>

#if WIN32
constexpr std::string_view DOTNET_PATH = "C:/Program Files/dotnet/";
#else
constexpr std::string_view DOTNET_PATH = "/usr/local/share/dotnet";
#endif

struct DemoStruct
{
	int32_t a;
	int32_t b;
};

int main()
{
	//Demo stuff
	auto scriptManager = ScriptingManager(DOTNET_PATH.data());
	const char* assembly = "assembly-test";
	int a, b;
	printf("Hi there, this is a C++ app, please input two numbers: ");
	std::cin >> a;
	printf("Second one: ");
	std::cin >> b;
	const char* testNamespace = "Test";
	const char* testClass = "Class1";
	const char* testFunc = "SumTest";
	int res = scriptManager.InvokeCSharpWithReturn<int>(assembly, testNamespace, testClass, testFunc, a, b);
	std::cout << "Result: " << res << std::endl;
	scriptManager.InvokeCSharp(assembly, testNamespace, testClass, "MultipleArgs", a, b);
	return 0;
}
