/*! \file FunctionInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief Function info implementation
*/

#pragma once
#include "Variant.hpp"
#include "Result.hpp"
#include "Metadata.hpp"

#include <array>

namespace Hush::Reflection
{
	class FunctionInfo : public MetadataHolder
	{
	public:
		static constexpr std::uint8_t MAX_ARGS = 16;

		enum class EFunctionInfoError : uint8_t
		{
			None = 0,
			InvalidType = 1,
			InvalidArgsCount = 2,
			InvalidArgsType = 3,
			NonMatchingArgs = 4,
		};
		using CallFunc = Result<Variant, EFunctionInfoError> (*)(std::span<const VariantView>);

		FunctionInfo(CallFunc callFunc, std::string name, std::span<const TypeId> argsType,
					 MetadataMap metadata = {})
			: m_name(std::move(name)),
			  m_callFunc(callFunc),
			  m_argsCount(static_cast<uint8_t>(argsType.size()))
		{
			SetMetadata(std::move(metadata));
			if (m_argsCount > MAX_ARGS)
			{
				// TODO: Handle error, maybe a log message?
			}
			std::copy(argsType.begin(), argsType.end(), m_argsType.begin());
		}

		template <typename... Args>
			requires(sizeof...(Args) <= MAX_ARGS)
		static FunctionInfo Create(CallFunc callFunc, std::string name, MetadataMap metadata = {})
		{
			FunctionInfo funcInfo(callFunc, std::move(name),
								  std::span<const TypeId>({GetTypeId<std::remove_reference_t<Args>>()...}),
								  std::move(metadata));

			return funcInfo;
		}

		///
		/// Calls the function with the given arguments.
		///
		/// @param returnVal Return value type.
		/// @param args Arguments to the function.
		///
		/// @return Result with the return value or an error.
		[[nodiscard]]
		Result<Variant, EFunctionInfoError> Call(std::span<const VariantView> args) const
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

		[[nodiscard]]
		std::uint64_t GetArgsCount() const
		{
			return m_argsCount;
		}

		[[nodiscard]]
		bool IsCallableWith(std::span<const VariantView> args) const
		{
			return m_argsCount == args.size() &&
				   std::equal(m_argsType.begin(), m_argsType.begin() + m_argsCount, args.begin(),
							  [](const TypeId &typeId, const VariantView &arg) { return typeId == arg.GetTypeId(); });
		}

		[[nodiscard]]
		std::string_view GetName() const
		{
			return m_name;
		}

	private:
		std::array<TypeId, MAX_ARGS> m_argsType;
		std::string m_name;
		CallFunc m_callFunc = nullptr;
		std::uint8_t m_argsCount = 0;
	};

} // namespace Hush::Reflection