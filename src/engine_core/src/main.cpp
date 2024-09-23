#include "HushEngine.hpp"
#include "Assertions.hpp"
#include <memory>

#include "Logger.hpp"

int main()
{
    Hush::HushEngine engine;

    engine.Run();

    engine.Quit();
    return 0;
}
