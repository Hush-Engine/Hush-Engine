//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//
#pragma once
#include "utils/LibManager.hpp"
#include "utils/StringUtils.hpp"
#include "../../Core/Logger.hpp"
#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstdio>
#include <string_view>
#include <string>
#include <filesystem>

template <class ... Types>
using VoidCSMethod = void (*)(Types...);

template <class R, class ... Types>
using ReturnableCSMethod = R (*)(Types...);

class ScriptingManager {
public:
	ScriptingManager(const char* dotnetPath);
	
	~ScriptingManager();
	
	template<class R, class ... Types>
	R InvokeCSharpWithReturn(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, Types... args) {
		//TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every time
		std::string fullClassPath = this->BuildFullClassPath(targetAssembly, targetNamespace, targetClass);

		//Get the correct type of function pointer
		ReturnableCSMethod<R, Types ...> testDelegate;
		int rc = this->GetMethodFromCS(fullClassPath.c_str(), fnName, reinterpret_cast<void**>(&testDelegate));
		if (rc != 0) {
			//TODO: Error handling
			LOG_ERROR_LN("Failed to invoke C# method with name %s. Please verify the signature", fnName);
			return R();
		}
		return testDelegate(args...);
	}
	
	template<class ... Types>
	void InvokeCSharp(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, Types... args) {
		//TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every time
		std::string fullClassPath = this->BuildFullClassPath(targetAssembly, targetNamespace, targetClass);

		//Get the correct type of function pointer
		VoidCSMethod<Types ...> testDelegate;
		int rc = this->GetMethodFromCS(fullClassPath.c_str(), fnName, reinterpret_cast<void**>(&testDelegate));
		if (rc != 0) {
			LOG_ERROR_LN("Failed to invoke C# method with name %s. Please verify the signature", fnName);
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
	
	load_assembly_fn GetLoadAssembly(void* hostFxrHandle);

	get_function_pointer_fn GetFunctionPtr(void* hostFxrHandle);
	
	bool LoadAssemblyFromPath(load_assembly_fn assembly_loader);

	std::string BuildFullClassPath(const char* targetAssembly, const char* targetNamespace, const char* targetClass);
	
	template <class ... Types>
	int GetMethodFromCS(const char* fullClassPath, const char* fnName, void** outMethod) {
#if WIN32
		std::wstring pathStr = StringUtils::ToWString(fullClassPath);
		const char_t* classPath = pathStr.data();
		std::wstring funcStr = StringUtils::ToWString(fnName);
		const char_t* targetFunction = funcStr.data();
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
			outMethod);
		return rc;
	}
	
};

