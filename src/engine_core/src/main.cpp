#include "HushEngine.hpp"
#include "Scene.hpp"

int main(int argc, const char *argv[])
{
	Hush::HushEngine engine;
	engine.Run({argv, static_cast<size_t>(argc)});
	engine.Quit();
	return 0;
}
