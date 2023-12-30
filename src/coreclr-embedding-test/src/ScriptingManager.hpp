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
#include "DotnetHost.hpp"
#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstdio>
#include <string_view>
#include <string>
#include <filesystem>

/// @brief A void C# method with any type of arguments
/// @tparam ...Types Types of the C# arguments list
template <class ... Types>
using VoidCSMethod = void (*)(Types...);

/// @brief A C# method with a return value (non complex value type)
/// @tparam R Return type
/// @tparam ...Types Types of the C# arguments list
template <class R, class ... Types>
using ReturnableCSMethod = R (*)(Types...);

/// @brief Class for bridging with .NET using hostfxr
class ScriptingManager {
public:
	/// <summary>
	/// Creates a new scripting manager to invoke C# methods
	/// </summary>
	/// <param name="host">The host connected to the C# runtime</param>
	/// <param name="targetAssembly">The desired assembly this manager is going to invoke methods from, THIS SHOULD BE CONSTANT WHEN POSSIBLE</param>
	ScriptingManager(std::shared_ptr<DotnetHost> host, std::string_view targetAssembly);

	template<class R, class ... Types>
	R InvokeCSharpWithReturn(const char* targetNamespace, const char* targetClass, const char* fnName, Types... args) {
		//TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every time
		std::string fullClassPath = this->BuildFullClassPath(this->targetAssembly.data(), targetNamespace, targetClass);

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
	void InvokeCSharp(const char* targetNamespace, const char* targetClass, const char* fnName, Types... args) {
		//TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every time
		std::string fullClassPath = this->BuildFullClassPath(this->targetAssembly.data(), targetNamespace, targetClass);

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
	std::string_view targetAssembly;
	std::shared_ptr<DotnetHost> host;

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
		//Retrieve getter from the host
		get_function_pointer_fn functionGetter = this->host.get()->GetFunctionGetterFuncPtr();

		int rc = functionGetter(
			classPath,
			targetFunction,
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			nullptr,
			outMethod);
		return rc;
	}
	
};

