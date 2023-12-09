#include "ScriptingManager.hpp"

struct DemoStruct
{
	int32_t a;
	int32_t b;
};

int main()
{
	auto scriptManager = ScriptingManager("/usr/local/share/dotnet");
	const char* assembly = "assembly-test";
	
	scriptManager.InvokeCSharp(assembly, "Test", "Class1", "Func", nullptr);
	
	// Get the function pointer for the managed code method
	/*
	if (rc != 0)
	{
		fputs("Get function pointer failed\n", stderr);
		return -1;
	}
	
	using dotnet_version_fn = void (*)(char*, int32_t);
	dotnet_version_fn dotnet_version;
	rc = get_function_pointer(
							  "Test.Class1, assembly-test",
							  "GetDotNetVersion",
							  UNMANAGEDCALLERSONLY_METHOD,
							  nullptr,
							  nullptr,
							  reinterpret_cast<void**>(&dotnet_version));
	
	if (rc != 0)
	{
		fputs("Get function pointer failed\n", stderr);
		return -1;
	}
	
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
	// Clean up
	
	return 0;
}
