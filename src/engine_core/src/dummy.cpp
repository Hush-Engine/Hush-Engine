#include "../internal/AppSupport.hpp"
#include "ApplicationLoader.hpp"

extern "C" HUSH_WEAK bool BundledAppExists_Internal_()
{
	return false;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine)
{
	(void)engine;
	return nullptr;
}
