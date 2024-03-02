#include "HushEngine.hpp"
#include <memory>

#if _WIN32
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
    LOG_DEBUG_LN("Allocated %zu bytes, current count: %d", size, allocCntr);
    return malloc(size);
}

void operator delete(void *p)
{
    allocCntr--;
    LOG_DEBUG_LN("Deallocating a pointer, current count: %d", allocCntr);
    free(p);
}

#endif

int main()
{
    HushEngine engine;

    engine.Run();

    engine.Quit();
    return 0;
}
