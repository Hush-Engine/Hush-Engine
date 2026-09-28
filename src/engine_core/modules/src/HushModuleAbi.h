/*! \file HushModuleAbi.h
	\author Alan Ramirez
	\date 2025-11-21
	\brief C ABI between the Hush runtime and gameplay modules

	This header is the only contract that gameplay modules (C++, Rust, C#)
	need to implement. It is intentionally plain C: no C++ classes, no STL
	types and no ownership conventions cross this boundary.
*/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/// Version of this ABI. The engine checks it before calling into a module.
#define HUSH_MODULE_ABI_VERSION 1u

/// Name of the entry point every dynamically loaded module must export.
#define HUSH_REGISTER_MODULE_NAME "HushRegisterModule"

/// Export visibility for the module entry point.
#if defined(_WIN32)
#define HUSH_MODULE_EXPORT __declspec(dllexport)
#else
#define HUSH_MODULE_EXPORT __attribute__((visibility("default")))
#endif

	/// Result codes returned by a module entry point.
	typedef enum HushModuleResult
	{
		/// The module registered everything correctly.
		HushModuleResult_Ok = 0,
		/// The module was built for a different ABI version.
		HushModuleResult_AbiVersionMismatch = 1,
		/// The module failed to register one of its types or systems.
		HushModuleResult_RegistrationFailed = 2,
		/// The module caught a managed exception while registering.
		HushModuleResult_ManagedException = 3,
		/// The module caught a Rust panic while registering.
		HushModuleResult_Panic = 4,
		/// Any other error.
		HushModuleResult_UnknownError = 5
	} HushModuleResult;

	/// Pointer + size UTF-8 string. The pointed memory is only valid during the
	/// call that receives it, unless documented otherwise.
	typedef struct HushStringView
	{
		const char *data;
		uint32_t size;
	} HushStringView;

	/// Opaque handle to an object that lives inside a module (for example a C#
	/// GCHandle or a pointer into a Rust arena). The engine never dereferences
	/// it, it only passes it back to the module that created it. Zero is reserved
	/// as the invalid handle returned when object creation fails.
	typedef struct HushObjectHandle
	{
		uintptr_t value;
	} HushObjectHandle;

	/// Bit mask of the lifecycle functions a foreign system implements. The
	/// engine uses it to skip calls the module does not care about.
	typedef enum HushSystemLifecycle
	{
		HushSystemLifecycle_None = 0,
		HushSystemLifecycle_Init = 1 << 0,
		HushSystemLifecycle_Update = 1 << 1,
		HushSystemLifecycle_FixedUpdate = 1 << 2,
		HushSystemLifecycle_Shutdown = 1 << 3,
		HushSystemLifecycle_PreRender = 1 << 4,
		HushSystemLifecycle_Render = 1 << 5,
		HushSystemLifecycle_PostRender = 1 << 6
	} HushSystemLifecycle;

	/// Callback table of a foreign language runtime. One table is registered per
	/// module and is shared by every system the module creates.
	///
	/// All callbacks can be called from engine worker threads, so modules must
	/// not assume they run on the main thread.
	typedef struct HushSystemRuntimeOps
	{
		/// Creates a system object of the given type id. The scene pointer is an
		/// opaque Hush::Scene handle owned by the engine.
		HushObjectHandle (*create)(uint64_t typeId, void *scene);
		/// Destroys an object created by create. Always called by the engine
		/// before the module is unloaded.
		void (*destroy)(HushObjectHandle object);
		void (*init)(HushObjectHandle object);
		void (*update)(HushObjectHandle object, float delta);
		void (*fixedUpdate)(HushObjectHandle object, float delta);
		void (*shutdown)(HushObjectHandle object);
		void (*preRender)(HushObjectHandle object);
		void (*render)(HushObjectHandle object);
		void (*postRender)(HushObjectHandle object);
	} HushSystemRuntimeOps;

	/// Description of a system implemented in a foreign language. Passed to the
	/// engine when the module registers its systems.
	typedef struct HushSystemDescriptor
	{
		/// Size of this struct, used for versioning.
		uint32_t structSize;
		/// ABI version the module was built against.
		uint32_t abiVersion;
		/// Type id of the system: the FNV-1a 64 hash of the canonical name.
		uint64_t typeId;
		/// Canonical name, for example "MyGame.TrafficSystem".
		HushStringView name;
		/// Update order, between 0 and 255.
		uint16_t order;
		/// Bit mask of HushSystemLifecycle values the system implements.
		uint32_t lifecycleMask;
	} HushSystemDescriptor;

	/* The generated Hush bindings table, defined in HushBindings.h. */
	struct HushFuncPtrTable;

	/// Context passed to the module entry point. Everything the module needs to
	/// talk to the engine is reachable from here.
	typedef struct HushModuleContext
	{
		/// Size of this struct, used for versioning.
		uint32_t structSize;
		/// ABI version of the engine that loads the module.
		uint32_t abiVersion;
		/// Opaque engine handle. Foreign modules pass it back to the generated
		/// Hush API, they must not dereference it.
		void *engine;
		/// Handle of this module, created by the engine before calling the entry
		/// point. Every type the module registers is owned by this handle.
		uint64_t module;
		/// Generated Hush API function table.
		const struct HushFuncPtrTable *hushApi;
		/// Engine internal data for native C++ modules. Foreign language modules
		/// must ignore this field.
		void *hostData;
	} HushModuleContext;

	/// Signature of the entry point every module exports. Implementations must
	/// translate language failures into HushModuleResult and never unwind across
	/// this ABI boundary.
	typedef HushModuleResult (*HushRegisterModuleFn)(const HushModuleContext *context);

#ifdef __cplusplus
} // extern "C"
#endif
