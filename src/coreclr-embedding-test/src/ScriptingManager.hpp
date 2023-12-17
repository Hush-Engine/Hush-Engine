//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//
#pragma once
#include "utils/LibManager.hpp"
#include "utils/StringUtils.hpp"
#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstdio>
#include <string_view>
#include <string>
#include <filesystem>

template <class ... Types>
using VoidCSMethod = void (*)(Types...);

class ScriptingManager {
public:
	ScriptingManager(const char* dotnetPath);
	
	~ScriptingManager();
	
	template<class T, class ... Types>
	T* InvokeCSharpWithReturn(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, Types... args);
	
	template<class ... Types>
	void InvokeCSharp(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, Types... args) {
		//TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every time
		//I'll make this more readable in the future, and we'll probably accept some heap allocs, for now I want to test if it works being performant
		//Concatenate them as {targetNamespace.targetClass}, {assemblyName}
		const int MAX_ASSEMBLY_DECL = 2048; //Dedicate 2MBs to the target
		std::string fullClassPath;
		fullClassPath.reserve(MAX_ASSEMBLY_DECL);
		fullClassPath += targetNamespace;
		fullClassPath += '.';
		fullClassPath += targetClass;
		fullClassPath += ", ";
		fullClassPath += targetAssembly;
		
		//Allocate memory to concatenate the string
		//Get the correct type of function pointer
		VoidCSMethod<Types ...> testDelegate;
		int rc = this->GetMethodFromCS(fullClassPath.c_str(), fnName, &testDelegate);
		if (rc != 0) {
			fputs("Error invoking CSharp method with name: ", stderr);
			fputs(fnName, stderr);
			fputs(". Please verify the signature\n", stderr);
			return;
		}
		testDelegate(args...);
	}

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
	
	template <class T>
	T LoadSymbol(void *shared_library, const char *name);
	
	void InitDotnetCore();
	
	template <class ... Types>
	int GetMethodFromCS(const char* fullClassPath, const char* fnName, VoidCSMethod<Types...>* outMethod) {
#if WIN32
		const char_t* classPath =  StringUtils::ToWString(fullClassPath).data();
		const char_t* targetFunction = StringUtils::ToWString(fnName).data();
#else
		const char* classPath = fullClassPath;
		const char* targetFunction = fnName;
#endif
		int rc = this->function_getter_fptr(
			classPath,
			targetFunction,
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			nullptr,
			reinterpret_cast<void**>(outMethod));
		return rc;
	}

	load_assembly_fn GetLoadAssembly(void* hostFxrHandle);
	
	get_function_pointer_fn GetFunctionPtr(void* hostFxrHandle);
	
	bool LoadAssemblyFromPath(load_assembly_fn assembly_loader);
};

