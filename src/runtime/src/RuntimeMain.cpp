/*! \file RuntimeMain.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Bundled application entry of the runtime player executable
*/

#include "RuntimeApplication.hpp"

#include <HushEngine.hpp>

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
	return true;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) // NOLINT(*-identifier-naming)
{
	return new Hush::RuntimeApplication(engine);
}
