/*! \file DotnetHost.hpp
	\author Leonidas Gonzalez
	\date 2023-12-29
	\brief This structure will hold references to all .NET function pointers with a valid initialization, to pass onto a scripting manager
*/

#pragma once
#include "Logger.hpp"
#include "utils/LibManager.hpp"
#include "utils/StringUtils.hpp"

#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <filesystem>
#include <cstdio>
#include <string_view>
#include <string>

/// <summary>
/// hold references to all .NET function pointers with a valid initialization, to pass onto <see cref="ScriptingManager"/>
/// </summary>
class DotnetHost {
public:
	/// <summary>
	/// Creates a new .NET host using hostfxr with the provided path of the runtime
	/// </summary>
	DotnetHost(const char* dotnetPath);

	~DotnetHost();

	get_function_pointer_fn GetFunctionGetterFuncPtr();


private:
	// Declare function pointers for the coreclr functions
	hostfxr_initialize_for_dotnet_command_line_fn cmdLineFuncPtr;
	hostfxr_initialize_for_runtime_config_fn initFuncPtr;
	hostfxr_get_runtime_delegate_fn getDelegateFuncPtr;
	hostfxr_run_app_fn runAppFuncPtr;
	hostfxr_close_fn closeFuncPtr;
	hostfxr_set_error_writer_fn errorWriterFuncPtr;
	get_function_pointer_fn function_getter_fptr;
	void* hostFxrHandle;

	void InitDotnetCore();

	load_assembly_fn GetLoadAssembly(void* hostFxrHandle);

	get_function_pointer_fn GetFunctionPtr(void* hostFxrHandle);

	bool LoadAssemblyFromPath(load_assembly_fn assembly_loader);

	template<class T>
	T LoadSymbol(void* sharedLibrary, const char* name);

};
