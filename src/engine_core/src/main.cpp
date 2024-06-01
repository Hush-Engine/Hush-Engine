#include "HushEngine.hpp"
#include "utils/Assertions.hpp"
#include <memory>

#include "log/Logger.hpp"

int main()
{
    Hush::HushEngine engine;

    engine.Run();

    engine.Quit();
    return 0;
}
