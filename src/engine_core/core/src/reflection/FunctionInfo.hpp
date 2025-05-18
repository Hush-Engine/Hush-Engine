/*! \file FunctionInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief Function info implementation
*/

#pragma once
#include "Variant.hpp"
#include "Result.hpp"

#include <array>

namespace Hush::Reflection
{
	class FunctionInfo
	{
	public:
		enum class EFunctionInfoError : uint8_t
		{
			None = 0,
			InvalidType = 1,
			InvalidArgsCount = 2,
			InvalidArgsType = 3,
			NonMatchingArgs = 4,
		};
		using CallFunc = Result<Variant, EFunctionInfoError> (*)(std::span<VariantView>);

		template <typename... Args>
		/**
		 * @brief Constructs a FunctionInfo object with specified argument types, function pointer, and name.
		 *
		 * Initializes the function metadata, including argument type IDs, function pointer, and name. Enforces a maximum argument count of 16 at compile time.
		 *
		 * @tparam Args Types of the function's arguments.
		 * @param argsCount Number of arguments expected by the function.
		 * @param callFunc Pointer to the function to be invoked.
		 * @param name Name of the function.
		 */
		FunctionInfo(std::uint8_t argsCount, CallFunc callFunc, std::string name)
			: m_argsType({GetTypeId<std::remove_reference_t<Args>>()...}),
			  m_name(std::move(name)),
			  m_callFunc(callFunc),
			  m_argsCount(argsCount)

		{
			static_assert(sizeof...(Args) <= MAX_ARGS, "Too many arguments");
		}

		///
		/// Calls the function with the given arguments.
		///
		/// @param returnVal Return value type.
		/// @param args Arguments to the function.
		///
		/**
		 * @brief Invokes the stored function with the provided arguments after validating count and types.
		 *
		 * Checks that the number and types of arguments match the expected function signature. Returns the function's result or an error code if validation fails or the function pointer is invalid.
		 *
		 * @param args Span of arguments to pass to the function.
		 * @return Result containing the function's return value or an EFunctionInfoError on failure.
		 */
		[[nodiscard]]
		Result<Variant, EFunctionInfoError> Call(std::span<VariantView> args) const
		{
			if (m_argsCount != args.size())
			{
				return EFunctionInfoError::InvalidArgsCount;
			}

			if (!std::equal(m_argsType.begin(), m_argsType.begin() + m_argsCount, args.begin(),
							[](const TypeId &typeId, const VariantView &arg) { return typeId == arg.GetTypeId(); }))
			{
				return EFunctionInfoError::InvalidArgsType;
			}

			if (m_callFunc == nullptr)
			{
				return EFunctionInfoError::InvalidType;
			}

			return m_callFunc(args);
		}

		///
		/// Calls the function with the given arguments.
		/// @tparam Args Arguments to the function.
		/// @param returnVal Return value type.
		/// @param args Arguments to the function.
		/// @return Result with the return value or an error.
		template <typename... Args>
		/**
		 * @brief Invokes the stored function with the provided arguments after converting them to VariantView.
		 *
		 * Converts the given arguments into an array of VariantView and calls the stored function, performing runtime checks on argument count and types.
		 *
		 * @tparam Args Types of the arguments to pass to the function.
		 * @param args Arguments to invoke the function with.
		 * @return Result containing the function's return value as a Variant, or an EFunctionInfoError if invocation fails.
		 */
		Result<Variant, EFunctionInfoError> Call(Args &&...args) const
		{
			std::array<VariantView, sizeof...(Args)> argsArray{std::forward<Args>(args)...};
			return Call(argsArray);
		}

		/**
		 * @brief Returns the number of arguments expected by the function.
		 *
		 * @return The expected argument count.
		 */
		std::uint64_t GetArgsCount() const
		{
			return m_argsCount;
		}

		/**
		 * @brief Checks if the provided arguments match the expected count and types for this function.
		 *
		 * @param args Span of arguments to validate.
		 * @return true if the argument count and types match the function signature; false otherwise.
		 */
		bool IsCallableWith(std::span<VariantView> args) const
		{
			return m_argsCount == args.size() && std::equal(m_argsType.begin(), m_argsType.begin() + m_argsCount,
														   args.begin(),
														   [](const TypeId &typeId, const VariantView &arg) {
															   return typeId == arg.GetTypeId();
														   });
		}

	private:
		static constexpr std::uint8_t MAX_ARGS = 16;

		std::array<TypeId, MAX_ARGS> m_argsType;
		std::string m_name;
		CallFunc m_callFunc = nullptr;
		std::uint8_t m_argsCount = 0;
	};

}