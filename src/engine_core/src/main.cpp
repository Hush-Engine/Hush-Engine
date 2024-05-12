#include "HushEngine.hpp"
#include "utils/Assertions.hpp"
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

int main()
{
    (void)ASSEMBLY_TEST;
    (void)DOTNET_PATH;
    Hush::HushEngine engine;

    engine.Run();

    engine.Quit();
    return 0;
}
