/*! \file DotnetHost.hpp
    \author Leonidas Gonzalez
    \date 2023-12-29
    \brief This structure will hold references to all .NET function pointers with a valid initialization, to pass onto a
   scripting manager
*/

#pragma once
#include "core/Logger.hpp"
#include "core/utils/LibManager.hpp"
#include "core/utils/StringUtils.hpp"

#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

/// <summary>
/// hold references to all .NET function pointers with a valid initialization, to pass onto <see
/// cref="ScriptingManager"/>
/// </summary>
class DotnetHost
{
  public:
    /// <summary>
    /// Creates a new .NET host using hostfxr with the provided path of the runtime
    /// </summary>
    DotnetHost(const char *dotnetPath);

    DotnetHost(const DotnetHost &other) = delete;

    DotnetHost(DotnetHost &&other) noexcept;

    DotnetHost &operator=(const DotnetHost &other) = delete;

    DotnetHost &operator=(DotnetHost &&other) noexcept;

    ~DotnetHost();

    get_function_pointer_fn GetFunctionGetterFuncPtr();

  private:
    // Declare function pointers for the coreclr functions
    hostfxr_initialize_for_dotnet_command_line_fn m_cmdLineFuncPtr;
    hostfxr_initialize_for_runtime_config_fn m_initFuncPtr;
    hostfxr_get_runtime_delegate_fn m_getDelegateFuncPtr;
    hostfxr_run_app_fn m_runAppFuncPtr;
    hostfxr_close_fn m_closeFuncPtr;
    hostfxr_set_error_writer_fn m_errorWriterFuncPtr;
    get_function_pointer_fn m_functionGetterFuncPtr{};
    void *m_hostFxrHandle{};

    /// @brief Initializes the .NET core runtime
    void InitDotnetCore();

    load_assembly_fn GetLoadAssembly(void *hostFxrHandle);

    get_function_pointer_fn GetFunctionPtr(void *hostFxrHandle);

    bool LoadAssemblyFromPath(load_assembly_fn assemblyLoader);

    template <class T> T LoadSymbol(void *sharedLibrary, const char *name);
};
