/*! \file ICommandQueue.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Command queue interface for graphics abstraction
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "ICommandList.hpp"
#include <span>

namespace Hush::Graphics
{
	// ============================================================================
	// Command Queue Interface
	// ============================================================================

	/// @brief Abstract command queue for command submission
	class ICommandQueue
	{
	public:
		ICommandQueue() = default;
		virtual ~ICommandQueue() = default;

		ICommandQueue(const ICommandQueue&) = delete;
		ICommandQueue& operator=(const ICommandQueue&) = delete;
		ICommandQueue(ICommandQueue&&) = delete;
		ICommandQueue& operator=(ICommandQueue&&) = delete;

		/// @brief Get queue type
		[[nodiscard]] virtual EQueueType GetQueueType() const = 0;

		/// @brief Submit commands for execution
		/// @param commandLists Array of command lists to submit
		virtual void Submit(std::span<ICommandList*> commandLists) = 0;

		/// @brief Wait for all commands to complete
		virtual void WaitIdle() = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]] virtual void* GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
