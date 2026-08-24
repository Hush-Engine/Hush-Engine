/*! \file ModuleSystem.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief ISystem adapter for systems implemented in gameplay modules
*/

#include "ModuleSystem.hpp"

Hush::Modules::ModuleSystem::ModuleSystem(Scene &scene, ModuleHandle module, HushObjectHandle object,
										  HushSystemRuntimeOps runtimeOps, std::string_view name, std::uint16_t order,
										  std::uint32_t lifecycleMask)
	: ISystem(scene),
	  m_module(module),
	  m_object(object),
	  m_ops(runtimeOps),
	  m_name(name),
	  m_lifecycleMask(lifecycleMask)
{
	SetOrder(order);
}

Hush::Modules::ModuleSystem::~ModuleSystem()
{
	if (m_ops.destroy != nullptr && m_object.value != 0)
	{
		m_ops.destroy(m_object);
	}
}

void Hush::Modules::ModuleSystem::Init()
{
	if ((m_lifecycleMask & HushSystemLifecycle_Init) != 0 && m_ops.init != nullptr)
	{
		m_ops.init(m_object);
	}
}

void Hush::Modules::ModuleSystem::OnUpdate(float delta)
{
	if ((m_lifecycleMask & HushSystemLifecycle_Update) != 0 && m_ops.update != nullptr)
	{
		m_ops.update(m_object, delta);
	}
}

void Hush::Modules::ModuleSystem::OnFixedUpdate(float delta)
{
	if ((m_lifecycleMask & HushSystemLifecycle_FixedUpdate) != 0 && m_ops.fixedUpdate != nullptr)
	{
		m_ops.fixedUpdate(m_object, delta);
	}
}

void Hush::Modules::ModuleSystem::OnShutdown()
{
	if ((m_lifecycleMask & HushSystemLifecycle_Shutdown) != 0 && m_ops.shutdown != nullptr)
	{
		m_ops.shutdown(m_object);
	}
}

void Hush::Modules::ModuleSystem::OnPreRender()
{
	if ((m_lifecycleMask & HushSystemLifecycle_PreRender) != 0 && m_ops.preRender != nullptr)
	{
		m_ops.preRender(m_object);
	}
}

void Hush::Modules::ModuleSystem::OnRender()
{
	if ((m_lifecycleMask & HushSystemLifecycle_Render) != 0 && m_ops.render != nullptr)
	{
		m_ops.render(m_object);
	}
}

void Hush::Modules::ModuleSystem::OnPostRender()
{
	if ((m_lifecycleMask & HushSystemLifecycle_PostRender) != 0 && m_ops.postRender != nullptr)
	{
		m_ops.postRender(m_object);
	}
}
