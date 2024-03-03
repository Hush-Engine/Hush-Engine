#include "HushEngine.hpp"
#include <memory>

#include "log/Logger.hpp"

#if defined(_WIN32)
constexpr std::string_view DOTNET_PATH = "C:/Program Files/dotnet/";
#elif defined(__APPLE__)
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
    Hush::LogFormat(Hush::ELogLevel::Debug, "Allocated {} bytes, current count: {}", size, allocCntr);
    return malloc(size);
}

void operator delete(void *p)
{
    allocCntr--;
    Hush::LogFormat(Hush::ELogLevel::Debug, "Deallocating a pointer, current count: {}", allocCntr);
    free(p);
}

#endif

int main()
{
    (void)ASSEMBLY_TEST;
    (void)DOTNET_PATH;
    HushEngine engine;

    engine.Run();

    engine.Quit();
    return 0;
}
