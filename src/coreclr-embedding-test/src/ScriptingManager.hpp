//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//
#pragma once
#include <dlfcn.h>
#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstdio>
#include <filesystem>
#include "../../utils.hpp"

class ScriptingManager {
public:
	ScriptingManager(const char* dotnetPath);
	~ScriptingManager();
	template<class T>
	T* InvokeCSharp(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, void* args...);
	
	void InvokeCSharp(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, void* args...);
private:
	// Declare function pointers for the coreclr functions
	hostfxr_initialize_for_dotnet_command_line_fn cmd_line_fptr;
	hostfxr_initialize_for_runtime_config_fn init_fptr;
	hostfxr_get_runtime_delegate_fn get_delegate_fptr;
	hostfxr_run_app_fn run_app_fptr;
	hostfxr_close_fn close_fptr;
	hostfxr_set_error_writer_fn error_writer_fptr;
	get_function_pointer_fn function_getter_fptr;
	void* hostFxrHandle;
	
	template <class T>
	T LoadSymbol(void *shared_library, const char *name);
	
	void InitDotnetCore();
	
	load_assembly_fn GetLoadAssembly(void* hostFxrHandle);
	
	get_function_pointer_fn GetFunctionPtr(void* hostFxrHandle);
	
	bool LoadAssemblyFromPath(load_assembly_fn assembly_loader);
};

