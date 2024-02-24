#include "core/scripting/ScriptingManager.hpp"
#include <iostream>
#include <map>
#include <thread>

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

void *operator new(size_t size)
{
    allocCntr++;
    LogDebug("Allocated {} bytes, current count: {}", size, allocCntr);
    return malloc(size);
}

void operator delete(void *p)
{
    allocCntr--;
    LogDebug("Deallocating a pointer, current count: {}", allocCntr);
    free(p);
}

#endif

int main()
{
    std::shared_ptr<DotnetHost> host = std::make_shared<DotnetHost>(DOTNET_PATH.data());
    LogTrace("Just finished initializing our host!");
    // Demo stuff
    auto scriptManager = ScriptingManager(host, ASSEMBLY_TEST);
    LogDebug("Now finished initializing our script manager");
    const char *testNamespace = "Test";
    const char *testClass = "Class1";
    const char *testFunc = "SumTest";
    auto logHandler = [](std::uint32_t level, const char *message) {
        LogMessage(static_cast<LogLevel>(level), message);
    };
    LogInfo("Log handler address: {}", reinterpret_cast<void *>(+logHandler));
    scriptManager.InvokeCSharp(testNamespace, testClass, "SetLogHandler", +logHandler);
    for (size_t i = 0; i < 90000; i++)
    {
        char *result = scriptManager.InvokeCSharpWithReturn<char *>(testNamespace, testClass, "GetCsharpString");
        // LogInfo("C++ thinks the string is: {}", result);
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));

        scriptManager.InvokeCSharp<char *>(testNamespace, testClass, "DeallocateString", result);
    }
    return 0;
}
