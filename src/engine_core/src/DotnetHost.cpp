#include "DotnetHost.hpp"

#include "Logger.hpp"
#include "utils/LibManager.hpp"
#include "utils/StringUtils.hpp"

constexpr std::string_view DOTNET_CMD("hostfxr_initialize_for_dotnet_command_line");
constexpr std::string_view DOTNET_RUNTIME_INIT_CONFIG("hostfxr_initialize_for_runtime_config");
constexpr std::string_view DOTNET_RUNTIME_DELEGATE("hostfxr_get_runtime_delegate");
constexpr std::string_view DOTNET_RUN_FUNCTION("hostfxr_run_app");
constexpr std::string_view DOTNET_CLOSE_FUNCTION("hostfxr_close");
constexpr std::string_view DOTNET_ERROR_WRITER("hostfxr_set_error_writer");

constexpr std::string_view RUNTIME_CONFIG_JSON("assembly-test.runtimeconfig.json");

#if WIN32
#define HOST_FXR_PATH "host/fxr/8.0.0/hostfxr.dll"
#elif __linux__
#define HOST_FXR_PATH "host/fxr/8.0.0/libhostfxr.so"
#elif __APPLE__
#define HOST_FXR_PATH "host/fxr/8.0.0/libhostfxr.dylib"
#endif

DotnetHost::DotnetHost(const char* dotnetPath)
{
	std::filesystem::path targetPath = dotnetPath;
	targetPath /= HOST_FXR_PATH;
#if WIN32
	std::string strLibPath = targetPath.string();
	const char* libPath = strLibPath.c_str();
#else
	const char* libPath = targetPath.c_str();
#endif
	void* sharedLibrary = LibManager::LibraryOpen(libPath);
	if (!sharedLibrary)
	{
		LOG_ERROR_LN("Failed to load %s", libPath);
		return;
	}
	//TODO: See which of these can stop being cached and just pass them as params to the initdotnetcore
	this->cmdLineFuncPtr = LoadSymbol<hostfxr_initialize_for_dotnet_command_line_fn>(sharedLibrary, DOTNET_CMD.data());
	this->initFuncPtr = LoadSymbol<hostfxr_initialize_for_runtime_config_fn>(sharedLibrary, DOTNET_RUNTIME_INIT_CONFIG.data());
	this->getDelegateFuncPtr = LoadSymbol<hostfxr_get_runtime_delegate_fn>(sharedLibrary, DOTNET_RUNTIME_DELEGATE.data());
	this->runAppFuncPtr = LoadSymbol<hostfxr_run_app_fn>(sharedLibrary, DOTNET_RUN_FUNCTION.data());
	this->closeFuncPtr = LoadSymbol<hostfxr_close_fn>(sharedLibrary, DOTNET_CLOSE_FUNCTION.data());
	this->errorWriterFuncPtr = LoadSymbol<hostfxr_set_error_writer_fn>(sharedLibrary, DOTNET_ERROR_WRITER.data());
	//Add logging for any errors in C#
#if WIN32
	this->errorWriterFuncPtr([](const char_t* message) {
		std::wstring wStrMessage = message;
		std::string strMessage = StringUtils::FromWString(wStrMessage);
		const char* cMessage = strMessage.c_str();
		LOG_ERROR_LN("Received an error from C# runtime %s", cMessage);
	});
#else
	this->errorWriterFuncPtr([](const char* message) {
		LOG_ERROR_LN("Received an error from C# runtime %s", message);
	});
#endif
	this->InitDotnetCore();
}

DotnetHost::~DotnetHost()
{
	this->closeFuncPtr(this->hostFxrHandle);
}

get_function_pointer_fn DotnetHost::GetFunctionGetterFuncPtr()
{
	return this->function_getter_fptr;
}

void DotnetHost::InitDotnetCore()
{
	std::filesystem::path runtimeConfigPath = LibManager::GetCurrentExecutablePath();
	runtimeConfigPath /= RUNTIME_CONFIG_JSON.data();
#if WIN32
	std::wstring configStr = runtimeConfigPath.wstring();
	const char_t* runtime_config = configStr.data();
#else
	const char* runtime_config = runtimeConfigPath.c_str();
#endif

	// Load and initialize .NET Core
	int rc = this->initFuncPtr(runtime_config, nullptr, &this->hostFxrHandle);
	if (rc != 0)
	{
		LOG_ERROR_LN("Init failed");
		return;
	}
	LOG_INFO_LN("Init .NET core succeeded!");
	load_assembly_fn assemblyLoader = this->GetLoadAssembly(this->hostFxrHandle);

	if (assemblyLoader == nullptr) {
		LOG_ERROR_LN("Could not get load assembly ptr");
		return;
	}

	this->function_getter_fptr = this->GetFunctionPtr(this->hostFxrHandle);

	if (this->function_getter_fptr == nullptr) {
		LOG_ERROR_LN("Get function ptr failed");
		return;
	}
	//Actually load the assembly
	bool isAssemblyLoaded = LoadAssemblyFromPath(assemblyLoader);

	if (!isAssemblyLoaded) {
		LOG_ERROR_LN("Failed to load the assembly");
		return;
	}
}

load_assembly_fn DotnetHost::GetLoadAssembly(void* hostFxrHandle) {
	// Get the load_assembly_and_get_function_pointer function pointer
	load_assembly_fn load_assembly = nullptr;
	getDelegateFuncPtr(hostFxrHandle, hdt_load_assembly, reinterpret_cast<void**>(&load_assembly));
	return load_assembly;
}

get_function_pointer_fn DotnetHost::GetFunctionPtr(void* hostFxrHandle)
{
	get_function_pointer_fn get_function_pointer = nullptr;
	getDelegateFuncPtr(hostFxrHandle, hdt_get_function_pointer, reinterpret_cast<void**>(&get_function_pointer));
	return get_function_pointer;
}

bool DotnetHost::LoadAssemblyFromPath(load_assembly_fn assembly_loader)
{
	std::filesystem::path assembly_path = LibManager::GetCurrentExecutablePath() / "assembly-test.dll";
	int rc = assembly_loader(assembly_path.c_str(), nullptr, nullptr);
	return rc == 0;
}

template<class T>
T DotnetHost::LoadSymbol(void* sharedLibrary, const char* name)
{
	void* libraryPtr = LibManager::DynamicLoadSymbol(sharedLibrary, name);
	return reinterpret_cast<T>(libraryPtr);
}
