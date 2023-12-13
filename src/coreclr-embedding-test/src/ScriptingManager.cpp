//
//  ScriptingManager.cpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//

#include "ScriptingManager.hpp"

constexpr std::string_view DOTNET_CMD("hostfxr_initialize_for_dotnet_command_line");
constexpr std::string_view DOTNET_RUNTIME_INIT_CONFIG("hostfxr_initialize_for_runtime_config");
constexpr std::string_view DOTNET_RUNTIME_DELEGATE("hostfxr_get_runtime_delegate");
constexpr std::string_view DOTNET_RUN_FUNCTION("hostfxr_run_app");
constexpr std::string_view DOTNET_CLOSE_FUNCTION ("hostfxr_close");
constexpr std::string_view DOTNET_ERROR_WRITER ("hostfxr_set_error_writer");

#if WIN32
#define HOST_FXR_PATH "hostfxr.dll"
#elif __linux__
#define HOST_FXR_PATH "host/fxr/8.0.0/libhostfxr.so"
#elif __APPLE__
#define HOST_FXR_PATH "host/fxr/8.0.0/libhostfxr.dylib"
#endif

template<class T, class ... Types>
T* ScriptingManager::InvokeCSharpWithReturn(const char* targetAssembly, const char* targetNamespace, const char* targetClass, const char* fnName, Types... args) {
	//InvokeCSharp(targetAssembly, targetNamespace, targetClass, fnName, args);
	return nullptr;
}

template <class T>
T ScriptingManager::LoadSymbol(void *sharedLibrary, const char *name) {
	//TODO: OS specific stuff
	return reinterpret_cast<T>(dlsym(sharedLibrary, name));
}

ScriptingManager::ScriptingManager(const char* dotnetPath) {
	//Get env for dotnet here and use that as our path
	//auto sharedLibrary = dlopen("/usr/share/dotnet/host/fxr/8.0.0/libhostfxr.so", RTLD_LAZY);
	std::filesystem::path targetPath = dotnetPath;
	targetPath /= HOST_FXR_PATH;
	void* sharedLibrary = dlopen(targetPath.c_str(), RTLD_LAZY);
	if (!sharedLibrary)
	{
		fputs("Failed to load ", stderr);
		fputs(targetPath.c_str(), stderr);
		fputs("\n", stderr);
		return;
	}
	//TODO: See which of these can stop being cached and just pass them as params to the initdotnetcore
	this->cmdLineFuncPtr = LoadSymbol<hostfxr_initialize_for_dotnet_command_line_fn>(sharedLibrary, DOTNET_CMD.data());
	this->initFuncPtr = LoadSymbol<hostfxr_initialize_for_runtime_config_fn>(sharedLibrary, DOTNET_RUNTIME_INIT_CONFIG.data());
	this->getDelegateFuncPtr = LoadSymbol<hostfxr_get_runtime_delegate_fn>(sharedLibrary, DOTNET_RUNTIME_DELEGATE.data());
	this->runAppFuncPtr = LoadSymbol<hostfxr_run_app_fn>(sharedLibrary, DOTNET_RUN_FUNCTION.data());
	this->closeFuncPtr = LoadSymbol<hostfxr_close_fn>(sharedLibrary, DOTNET_CLOSE_FUNCTION.data());
	this->errorWriterFuncPtr = LoadSymbol<hostfxr_set_error_writer_fn>(sharedLibrary, DOTNET_ERROR_WRITER.data());
	this->errorWriterFuncPtr([](const char *message) { fputs(message, stderr); });
	this->InitDotnetCore();
}

ScriptingManager::~ScriptingManager() {
	this->closeFuncPtr(this->hostFxrHandle);
}

void ScriptingManager::InitDotnetCore() {
	const char* runtime_config = "assembly-test.runtimeconfig.json";
	
	// Load and initialize .NET Core
	int rc = this->initFuncPtr(runtime_config, nullptr, &this->hostFxrHandle);
	if (rc != 0)
	{
		fputs("Init failed\n", stderr);
		return;
	}
	load_assembly_fn assemblyLoader = this->GetLoadAssembly(this->hostFxrHandle);
	
	if (assemblyLoader == nullptr) {
		fputs("Could not get load assembly ptr\n", stderr);
		return;
	}
	
	this->function_getter_fptr = this->GetFunctionPtr(this->hostFxrHandle);
	
	if (this->function_getter_fptr == nullptr) {
		fputs("Get function ptr failed\n", stderr);
		return;
	}
	//Actually load the assembly
	bool isAssemblyLoaded = LoadAssemblyFromPath(assemblyLoader);

	if (!isAssemblyLoaded) {
		fputs("Failed to load the assembly\n", stderr);
		return;
	}
	
}

load_assembly_fn ScriptingManager::GetLoadAssembly(void* hostFxrHandle) {
	// Get the load_assembly_and_get_function_pointer function pointer
	load_assembly_fn load_assembly = nullptr;
	getDelegateFuncPtr(hostFxrHandle, hdt_load_assembly, reinterpret_cast<void**>(&load_assembly));
	return load_assembly;
}

get_function_pointer_fn ScriptingManager::GetFunctionPtr(void* hostFxrHandle) {
	get_function_pointer_fn get_function_pointer = nullptr;
	getDelegateFuncPtr(hostFxrHandle, hdt_get_function_pointer, reinterpret_cast<void**>(&get_function_pointer));
	return get_function_pointer;
}

bool ScriptingManager::LoadAssemblyFromPath(load_assembly_fn assembly_loader) {
	auto assembly_path = std::filesystem::current_path() / "assembly-test.dll";
	int rc = assembly_loader(assembly_path.c_str(), nullptr, nullptr);
	return rc == 0;
}
