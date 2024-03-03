//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//
#pragma once
#include "DotnetHost.hpp"
#include "Logger.hpp"
#include "utils/LibManager.hpp"
#include "utils/StringUtils.hpp"
#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

/// @brief A void C# method with any type of arguments
/// @tparam ...Types Types of the C# arguments list
template <class... Types> using VoidCSMethod = void (*)(Types...);

/// @brief A C# method with a return value (non complex value type)
/// @tparam R Return type
/// @tparam ...Types Types of the C# arguments list
template <class R, class... Types> using ReturnableCSMethod = R (*)(Types...);

/// @brief Class for bridging with .NET using hostfxr
class ScriptingManager
{
  public:
    /// <summary>
    /// Creates a new scripting manager to invoke C# methods
    /// </summary>
    /// <param name="host">The host connected to the C# runtime</param>
    /// <param name="m_targetAssembly">The desired assembly this manager is going to invoke methods from, THIS SHOULD BE
    /// CONSTANT WHEN POSSIBLE</param>
    ScriptingManager(std::shared_ptr<DotnetHost> host, std::string_view targetAssembly);

    template <class R, class... Types>
    R InvokeCSharpWithReturn(const char *targetNamespace, const char *targetClass, const char *fnName, Types... args)
    {
        // TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every
        // time
        std::string fullClassPath =
            this->BuildFullClassPath(this->m_targetAssembly.data(), targetNamespace, targetClass);

        // Get the correct type of function pointer
        ReturnableCSMethod<R, Types...> testDelegate = nullptr;
        int rc = this->GetMethodFromCS(fullClassPath.c_str(), fnName, reinterpret_cast<void **>(&testDelegate));
        if (rc != 0)
        {
            // TODO: Error handling
            LogError("Failed to invoke C# method with name {}. Please verify the signature", fnName);
            return R();
        }
        return testDelegate(args...);
    }

    template <class... Types>
    void InvokeCSharp(const char *targetNamespace, const char *targetClass, const char *fnName, Types... args)
    {
        // TODO: Consider caching the functions in memory to a map so that we don't have to constantly load them every
        // time
        std::string fullClassPath =
            this->BuildFullClassPath(this->m_targetAssembly.data(), targetNamespace, targetClass);

        // Get the correct type of function pointer
        VoidCSMethod<Types...> testDelegate = nullptr;
        int rc = this->GetMethodFromCS(fullClassPath.c_str(), fnName, reinterpret_cast<void **>(&testDelegate));
        if (rc != 0)
        {
            LogError("Failed to invoke C# method with name {}. Please verify the signature, error code: {}", fnName,
                     rc);
            return;
        }
        testDelegate(args...);
    }

  private:
    std::string_view m_targetAssembly;
    // This pointer to a host is shared with other scripting managers, hence the shared ptr nature
    std::shared_ptr<DotnetHost> m_host;

    std::string BuildFullClassPath(const char *targetAssembly, const char *targetNamespace,
                                   const char *targetClass) const;

    template <class... Types> int GetMethodFromCS(const char *fullClassPath, const char *fnName, void **outMethod)
    {
#if _WIN32
        std::wstring pathStr = StringUtils::ToWString(fullClassPath);
        const char_t *classPath = pathStr.data();
        std::wstring funcStr = StringUtils::ToWString(fnName);
        const char_t *targetFunction = funcStr.data();
#else
        const char *classPath = fullClassPath;
        const char *targetFunction = fnName;
#endif
        // Retrieve getter from the host
        get_function_pointer_fn functionGetter = this->m_host->GetFunctionGetterFuncPtr();
        // NOLINTNEXTLINE (Unamaged callers does a C style cast)
        int rc = functionGetter(classPath, targetFunction, UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, outMethod);
        return rc;
    }
};
