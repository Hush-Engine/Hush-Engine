/*! \file DotnetHost.hpp
	\author Leonidas Gonzalez
	\date 2023-12-29
	\brief This structure will hold references to all .NET function pointers with a valid initialization, to pass onto a scripting manager
*/

#pragma once

#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstddef>
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
	
	DotnetHost(const DotnetHost &other) = default;

	DotnetHost(DotnetHost &&other) = default;

	DotnetHost &operator=(const DotnetHost &) = default;
	
	DotnetHost &operator=(DotnetHost &&) = default;



	~DotnetHost();

	get_function_pointer_fn GetFunctionGetterFuncPtr();


private:
	// Declare function pointers for the coreclr functions
	hostfxr_initialize_for_dotnet_command_line_fn m_cmdLineFuncPtr = nullptr;
	hostfxr_initialize_for_runtime_config_fn m_initFuncPtr = nullptr;
	hostfxr_get_runtime_delegate_fn m_getDelegateFuncPtr = nullptr;
	hostfxr_run_app_fn m_runAppFuncPtr = nullptr;
	hostfxr_close_fn m_closeFuncPtr = nullptr;
	hostfxr_set_error_writer_fn m_errorWriterFuncPtr = nullptr;
	get_function_pointer_fn m_functionGetterFuncPtr = nullptr;
	void* m_hostFxrHandle = nullptr;

	void InitDotnetCore();

	load_assembly_fn GetLoadAssembly(void* hostFxrHandle);

	get_function_pointer_fn GetFunctionPtr(void* hostFxrHandle);

	bool LoadAssemblyFromPath(load_assembly_fn assemblyLoader);

	template<class T>
	T LoadSymbol(void* sharedLibrary, const char* name);

};
