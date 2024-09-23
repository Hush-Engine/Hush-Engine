#include "ApplicationLoader.hpp"

extern "C" bool BundledAppExists_Internal_() HUSH_WEAK
{
    return false;
}

extern "C" Hush::IApplication *BundledApp_Internal_()
{
    return nullptr;
}