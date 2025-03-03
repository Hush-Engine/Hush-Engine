#include "Entity.hpp"
#include "HushEngine.hpp"
#include "Scene.hpp"

#include <flecs.h>

struct RGBA
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

struct MyComplexType
{
    std::string a;

    MyComplexType(int)
    {
        std::cout << "MyComplexType constructor" << std::endl;
    }

    ~MyComplexType()
    {
        std::cout << "MyComplexType destructor" << std::endl;
    }
};

struct Position
{
    float x;
    float y;
};

struct A
{
    int a = 234;
};

int main()
{
    Hush::HushEngine engine;
    engine.Run();
    engine.Quit();
    return 0;
}
