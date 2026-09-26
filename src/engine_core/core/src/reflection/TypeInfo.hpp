/*! \file TypeInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief TypeInfo implementation
*/

#pragma once
#include "TypeTraits.hpp"
#include "FunctionInfo.hpp"
#include "FieldInfo.hpp"
#include "Metadata.hpp"
#include "ModuleHandle.hpp"

#include <vector>
#include <optional>

namespace Hush::Reflection
{

	class TypeInfo : public MetadataHolder
	{

	public:
		enum class EInPlaceConstructorError
		{
			None = 0,
			InsufficientMemory = 1,
			NoInPlaceConstructors = 2,
			NonMatchingArgs = 3,
			InvalidType = 4,
		};

		using InPlaceCtorFunc = EInPlaceConstructorError (*)(void *mem, std::span<const VariantView>);

		struct InPlaceCtor
		{
			InPlaceCtor(InPlaceCtorFunc callFunc, std::span<const TypeId> args)
				: m_func(callFunc),
				  m_argsCount(static_cast<uint8_t>(args.size()))
			{
				if (m_argsCount > FunctionInfo::MAX_ARGS)
				{
					// TODO: Handle error, maybe a log message?
					return;
				}

				std::copy(args.begin(), args.end(), m_argsType.begin());
			}

			template <typename... Args>
				requires(sizeof...(Args) <= FunctionInfo::MAX_ARGS)
			static InPlaceCtor Create(InPlaceCtorFunc callFunc)
			{
				InPlaceCtor ctor(callFunc, std::span<const TypeId>({GetTypeId<std::remove_reference_t<Args>>()...}));
				return ctor;
			}

			[[nodiscard]]

			EInPlaceConstructorError ConstructUnchecked(void *mem, std::span<const VariantView> args) const
			{
				return m_func(mem, args);
			}

			[[nodiscard]]

			bool IsCallableWith(std::span<const VariantView> args) const
			{
				if (m_argsCount != args.size())
				{
					return false;
				}

				return std::equal(
					m_argsType.begin(), m_argsType.begin() + m_argsCount, args.begin(),
					[](const TypeId &typeId, const VariantView &arg) { return typeId == arg.GetTypeId(); });
			}

			std::array<TypeId, FunctionInfo::MAX_ARGS> m_argsType;
			InPlaceCtorFunc m_func;
			uint8_t m_argsCount{};
		};

	public:
		TypeInfo(TypeId id = {})
			: m_id(id)
		{
		}

		[[nodiscard]]
		TypeId GetId() const
		{
			return m_id;
		}

		void AddFunction(const FunctionInfo &function)
		{
			m_functions.push_back(function);
		}

		void AddField(const FieldInfo &field)
		{
			m_fields.push_back(field);
		}

		[[nodiscard]]
		std::span<const FunctionInfo> GetFunctions() const
		{
			return m_functions;
		}

		[[nodiscard]]
		std::span<const FieldInfo> GetFields() const
		{
			return m_fields;
		}

		[[nodiscard]]
		std::optional<std::reference_wrapper<const FieldInfo>> GetField(std::string_view name) const
		{
			for (auto &field : m_fields)
			{
				if (field.GetName() == name)
				{
					return std::ref(field);
				}
			}
			return std::nullopt;
		}

		[[nodiscard]]
		const std::string &GetName() const
		{
			return m_name;
		}

		void SetName(std::string_view name)
		{
			this->m_name = name;
		}

		[[nodiscard]]
		std::size_t GetSize() const
		{
			return m_size;
		}

		void SetSize(std::size_t size)
		{
			this->m_size = size;
		}

		[[nodiscard]]
		std::size_t GetAlignment() const
		{
			return m_alignment;
		}

		void SetAlignment(std::size_t alignment)
		{
			this->m_alignment = alignment;
		}

		/// Sets the module that owns this type. Called by the reflection
		/// database when the type is registered.
		void SetOwner(ModuleHandle module)
		{
			this->m_owner = module;
		}

		/// Gets the module that owns this type.
		[[nodiscard]]
		ModuleHandle GetOwner() const
		{
			return m_owner;
		}

		/// Returns true when this type belongs to the engine module, which
		/// means it is a built-in type that is never unloaded.
		[[nodiscard]]
		bool IsBuiltin() const
		{
			return m_owner == ENGINE_MODULE_HANDLE;
		}

		/// Creates an instance of this type with the given arguments and returns it as a Variant.
		/// If you wish to create an instance in a specific memory location, or avoid heap allocation, use
		/// \ref CreateInPlaceInstance(void *mem, size_t memSize, std::span<const VariantView> args) const instead.
		///
		/// \note This function might allocate memory for the instance, depending on the size of the type. See \ref
		/// Hush::Reflection::Variant::MAX_SIZE for the maximum size of a type that can be created without allocating in
		/// the heap.
		///
		///
		/// @param args Arguments to pass to the constructor.
		/// @return Result with the created instance or an error.
		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CreateInstance(std::span<const VariantView> args) const
		{
			for (const auto &constructor : m_constructors)
			{
				if (constructor.IsCallableWith(args))
				{
					return constructor.Call(args);
				}
			}

			return FunctionInfo::EFunctionInfoError::NonMatchingArgs;
		}

		/// Creates an instance of this type with the given arguments. See \ref CreateInstance(std::span<const
		/// VariantView> args) const for more information.
		/// @param args Arguments to pass to the constructor.
		/// @return Result with the created instance or an error.
		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CreateInstance(
			std::initializer_list<const VariantView> args) const
		{
			return CreateInstance(std::span(args));
		}

		/// Creates an instance of this type in-place using the provided memory and arguments.
		/// This function checks if the provided memory is sufficient and if there are any in-place constructors
		/// available.
		///
		/// When using this function, ensure that the memory provided is properly aligned for the type being
		/// constructed. Also, the memory must be large enough to hold the type's data.
		///
		/// \warning Keep in mind that this function does not keep track of the lifetime of the created instance.
		/// **You're responsible for managing the memory and ensuring that the instance is destroyed properly.**
		///
		/// \note This function is designed for advanced use cases where you need to create an instance of a type in a
		/// specific memory location, This function never allocates memory for the instance.
		///
		/// @param mem Pointer to the memory where the instance should be created.
		/// @param memSize Size of the memory in bytes. Must be at least as large as the size of the type.
		/// @param args Arguments to pass to the in-place constructor.
		/// @return An optional error if the in-place construction fails, or an empty optional if it succeeds.
		[[nodiscard]]
		std::optional<EInPlaceConstructorError> CreateInPlaceInstance(void *mem, size_t memSize,
																	  std::span<const VariantView> args) const
		{
			if (memSize < m_size)
			{
				return EInPlaceConstructorError::InsufficientMemory;
			}

			if (m_inPlaceCtors.empty())
			{
				return EInPlaceConstructorError::NoInPlaceConstructors;
			}

			for (const InPlaceCtor &ctor : m_inPlaceCtors)
			{
				if (ctor.IsCallableWith(args))
				{
					if (auto error = ctor.ConstructUnchecked(mem, args); error != EInPlaceConstructorError::None)
					{
						return error;
					}

					return {};
				}
			}

			return EInPlaceConstructorError::NonMatchingArgs;
		}

		/// Creates an instance of this type in-place using the provided memory and arguments.
		/// See \ref CreateInPlaceInstance(void *mem, size_t memSize, std::span<const VariantView> args) const
		/// for more information.
		/// @param mem Pointer to the memory where the instance should be created.
		/// @param memSize Size of the memory in bytes.
		/// @param args Arguments to pass to the in-place constructor.
		/// @return An optional error if the in-place construction fails, or an empty optional if it succeeds.
		[[nodiscard]]
		std::optional<EInPlaceConstructorError> CreateInPlaceInstance(
			void *mem, size_t memSize, std::initializer_list<const VariantView> args) const
		{
			return CreateInPlaceInstance(mem, memSize, std::span(args));
		}

		/// Calls a function with the given name and arguments.
		/// This helper function searches for a function in O(n) time, where n is the number of functions in this type.
		/// This also supports overloaded functions, as it checks the argument types to find a matching function.
		///
		/// @param name Name of the function to call.
		/// @param args Arguments to pass to the function.
		/// @return Result with the return value or an error.
		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CallFunction(std::string_view name,
																	   std::span<const VariantView> args) const
		{
			for (const FunctionInfo &function : m_functions)
			{
				if (function.GetName() == name && function.IsCallableWith(args))
				{
					return function.Call(args);
				}
			}

			return FunctionInfo::EFunctionInfoError::NonMatchingArgs;
		}

		/// Calls a function with the given name and arguments.
		/// This is a convenience overload that allows passing arguments as an initializer list. For more information,
		/// see the
		/// \ref CallFunction(std::string_view name, std::span<const VariantView> args) const
		/// function.
		///
		/// @param name Name of the function to call.
		/// @param args Arguments to pass to the function.
		/// @return Result with the return value or an error.
		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CallFunction(
			std::string_view name, std::initializer_list<const VariantView> args) const
		{
			return CallFunction(name, std::span(args));
		}

		void SetConstructors(std::vector<FunctionInfo> &&constructors)
		{
			m_constructors = std::move(constructors);
		}

		void SetFunctions(std::vector<FunctionInfo> &&functions)
		{
			m_functions = std::move(functions);
		}

		void SetFields(std::vector<FieldInfo> &&fields)
		{
			m_fields = std::move(fields);
		}

		/// Sets the in-place constructors for this type.
		/// @param inPlaceCtor A vector of in-place constructors to set.
		void SetInPlaceCtors(std::vector<InPlaceCtor> &&inPlaceCtor)
		{
			m_inPlaceCtors = std::move(inPlaceCtor);
		}

	private:
		TypeId m_id;
		ModuleHandle m_owner = ENGINE_MODULE_HANDLE;
		std::vector<FunctionInfo> m_constructors;
		std::vector<InPlaceCtor> m_inPlaceCtors;
		std::vector<FunctionInfo> m_functions;
		std::vector<FieldInfo> m_fields;
		std::string m_name;
		std::size_t m_size{0};
		std::size_t m_alignment{0};
	};

	template <typename T>
	[[nodiscard]]
	inline TypeInfo GetTypeInfo()
	{
		static_assert(false, "TypeInfo is not defined for this type");
		return {};
	}

	template <ReflectedType T>
	[[nodiscard]]
	inline TypeInfo GetTypeInfo()
	{
		return T::GetTypeInfo();
	}

	template <>
	[[nodiscard]]
	inline TypeInfo GetTypeInfo<void>()
	{
		return TypeInfo{};
	}

} // namespace Hush::Reflection