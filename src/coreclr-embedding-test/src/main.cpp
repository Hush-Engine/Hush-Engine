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
	printf("Hi there, this is a C++ app, please input a name: ");
	char name[200];
	std::cin >> name;
	const char* testNamespace = "Test";
	const char* testClass = "Class1";
	const char* testFunc = "SayName";
	int length = strlen(name);
	scriptManager.InvokeCSharp(assembly, testNamespace, testClass, "Func");
	scriptManager.InvokeCSharp(assembly, testNamespace, testClass, testFunc, name, length);
	// Get the function pointer for the managed code method
	/*
	char version[100];
	dotnet_version(version, 100);
	std::cout << "DotNetVersion: " << version << std::endl;
	
	using test_struct_fn = void (*)(DemoStruct*);
	test_struct_fn test_struct;
	rc = get_function_pointer(
							  "Test.Class1, assembly-test",
							  "GetStruct",
							  UNMANAGEDCALLERSONLY_METHOD,
							  nullptr,
							  nullptr,
							  reinterpret_cast<void**>(&test_struct));
	
	if (rc != 0)
	{
		fputs("Get function pointer failed\n", stderr);
		return -1;
	}
	
	DemoStruct demo_struct;
	demo_struct.a = 1;
	demo_struct.b = 2;
	test_struct(&demo_struct);
	std::cout << "DemoStruct: " << demo_struct.a << ", " << demo_struct.b << std::endl;
	// Run the managed code
	test_delegate();
	*/
	
	return 0;
}
