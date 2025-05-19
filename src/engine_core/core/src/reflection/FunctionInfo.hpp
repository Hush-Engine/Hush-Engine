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
		static constexpr std::uint8_t MAX_ARGS = 16;
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

		template <typename... Args> requires (sizeof...(Args) <= MAX_ARGS)
		FunctionInfo(CallFunc callFunc, std::string name)
			: m_argsType({GetTypeId<std::remove_reference_t<Args>>()...}),
			  m_name(std::move(name)),
			  m_callFunc(callFunc),
			  m_argsCount(static_cast<uint8_t>(sizeof...(Args)))

		{
		}

		///
		/// Calls the function with the given arguments.
		///
		/// @param returnVal Return value type.
		/// @param args Arguments to the function.
		///
		/// @return Result with the return value or an error.
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
		Result<Variant, EFunctionInfoError> Call(Args &&...args) const
		{
			std::array<VariantView, sizeof...(Args)> argsArray{std::forward<Args>(args)...};
			return Call(argsArray);
		}

		std::uint64_t GetArgsCount() const
		{
			return m_argsCount;
		}

		bool IsCallableWith(std::span<VariantView> args) const
		{
			return m_argsCount == args.size() && std::equal(m_argsType.begin(), m_argsType.begin() + m_argsCount,
														   args.begin(),
														   [](const TypeId &typeId, const VariantView &arg) {
															   return typeId == arg.GetTypeId();
														   });
		}

	private:

		std::array<TypeId, MAX_ARGS> m_argsType;
		std::string m_name;
		CallFunc m_callFunc = nullptr;
		std::uint8_t m_argsCount = 0;
	};

}