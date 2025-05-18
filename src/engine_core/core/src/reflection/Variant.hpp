/*! \file Variant.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief Variant implementation
*/

#pragma once

#include "TypeTraits.hpp"
#include "TypeId.hpp"
#include "Result.hpp"

namespace Hush::Reflection
{

	/// A VariantView that holds a reference to any type.
	struct VariantView
	{
		enum class EVariantError : uint8_t
		{
			NonSameType = 0,
		};

		/**
 * @brief Constructs an empty VariantView with no referenced value.
 *
 * The resulting VariantView does not reference any value and its type identifier is unset.
 */
VariantView() : m_value(nullptr), m_typeId(TypeId{}) {}

		template <typename T>
		/**
		 * @brief Constructs a VariantView referencing a value of type T.
		 *
		 * @tparam T The type of the referenced value. Must not be void.
		 * @param value Pointer to the value to reference.
		 *
		 * @note The VariantView does not take ownership of the referenced value.
		 */
		explicit VariantView(T *value)
			: m_value(value),
			  m_typeId(Hush::Reflection::GetTypeId<T>())
		{
			static_assert(!std::is_same_v<T, void>, "VariantView cannot hold void type");
		}

		template <typename T> requires (!std::is_pointer_v<T>)
		/**
		 * @brief Constructs a VariantView referencing a value of type T.
		 *
		 * Stores a non-owning pointer to the provided value and its runtime type identifier.
		 *
		 * @tparam T The type of the referenced value. Must not be void or a pointer type.
		 * @param value Reference to the value to be referenced.
		 */
		explicit VariantView(T &value)
			: m_value(&value),
			  m_typeId(Hush::Reflection::GetTypeId<T>())
		{
			static_assert(!std::is_same_v<T, void>, "VariantView cannot hold void type");
		}

		/**
		 * @brief Constructs a VariantView from a raw pointer and a type identifier.
		 *
		 * @param value Pointer to the value to reference.
		 * @param typeId Runtime type identifier of the referenced value.
		 */
		explicit VariantView(void *value, std::uint64_t typeId)
			: m_value(value),
			  m_typeId(typeId)
		{
		}

		template <typename T>
		/**
		 * @brief Returns a pointer to the referenced value if its type matches the requested type.
		 *
		 * Performs a runtime type check and returns a typed pointer if the stored value's type matches `T`. Returns an error if the types do not match.
		 *
		 * @return Result<T*, EVariantError> Typed pointer to the value on success, or `EVariantError::NonSameType` if the type does not match.
		 */
		[[nodiscard]]
		Result<T *, EVariantError> Get() const
		{
			if (m_typeId != Reflection::GetTypeId<T>())
			{
				return EVariantError::NonSameType;
			}

			return static_cast<T *>(m_value);
		}


		/**
		 * @brief Returns the stored pointer if its type matches the given type identifier.
		 *
		 * @param id The type identifier to check against the stored value.
		 * @return void* Pointer to the stored value if the type matches; otherwise, nullptr.
		 */
		[[nodiscard]]
		void *GetRaw(TypeId id) const
		{
			if (m_typeId != id)
			{
				return nullptr;
			}

			return m_value;
		}

		/**
		 * @brief Returns the runtime type identifier of the stored value.
		 *
		 * @return TypeId The type identifier associated with the referenced value.
		 */
		TypeId GetTypeId() const
		{
			return m_typeId;
		}

	private:
		void *m_value = nullptr;
		TypeId m_typeId;
	};

	class Variant
	{
		enum class EVariantStatus : uint8_t
		{
			None = 0,
			Small = 1,
			Large = 2,
		};

		using Dtor = void (*)(void *);

	public:
		using EVariantError = VariantView::EVariantError;

		/**
		 * @brief Constructs an empty Variant with no stored value.
		 */
		Variant()
			: m_ptr(nullptr),
			  m_typeId(TypeId{})
		{
		}

		template <typename T>
		/**
		 * @brief Constructs a Variant by storing a value of any type, using small object optimization when possible.
		 *
		 * Stores the given value either in an internal buffer (if its size is at most 16 bytes) or on the heap for larger types. The constructor sets up appropriate destruction logic based on the type's characteristics.
		 *
		 * @tparam T The type of the value to store.
		 * @param value The value to be stored in the variant.
		 */
		explicit Variant(T &&value)
			: m_typeId(GetTypeId<T>())
		{
			if constexpr (sizeof(std::remove_reference_t<T>) <= MAX_SMALL_SIZE)
			{
				new (m_data) std::remove_reference_t<T>(std::forward<T>(value));
				m_status = EVariantStatus::Small;
				if constexpr (std::is_trivially_destructible_v<T>)
				{
					m_dtor = nullptr;
				}
				else
				{
					m_dtor = [](void *ptr) { static_cast<T *>(ptr)->~T(); };
				}
			}
			else
			{
				m_ptr = new T(std::forward<T>(value));
				m_dtor = [](void *ptr) { delete static_cast<T *>(ptr); };
				m_status = EVariantStatus::Large;
			}
		}

		/**
 * @brief Copy constructor is deleted to prevent copying of Variant instances.
 */
Variant(const Variant &) = delete;
		/**
 * @brief Copy assignment is disabled for Variant.
 *
 * Prevents copying of Variant instances to ensure unique ownership and correct resource management.
 */
Variant &operator=(const Variant &) = delete;

		/**
		 * @brief Move constructs a Variant, transferring ownership of the stored value from another Variant.
		 *
		 * After the move, the source Variant is left empty and its destructor is reset.
		 */
		Variant(Variant &&rhs) noexcept
			: m_dtor(std::exchange(rhs.m_dtor, nullptr)),
			  m_status(std::exchange(rhs.m_status, EVariantStatus::None)),
			  m_typeId(std::exchange(rhs.m_typeId, TypeId{}))
		{
			memmove_s(m_data, sizeof(m_data), rhs.m_data, sizeof(m_data));
		}

		/**
		 * @brief Moves the contents of another Variant into this one, releasing any previously held value.
		 *
		 * Transfers ownership of the stored value, destructor, status, and type identifier from the source Variant. Any value previously held by this Variant is properly destroyed. The source Variant is left in an empty state.
		 *
		 * @param rhs The Variant to move from.
		 * @return Reference to this Variant.
		 */
		Variant &operator=(Variant &&rhs) noexcept
		{
			if (this != &rhs)
			{
				if (m_dtor != nullptr)
				{
					m_dtor(m_ptr);
				}

				m_dtor = std::exchange(rhs.m_dtor, nullptr);
				m_status = std::exchange(rhs.m_status, EVariantStatus::None);
				m_typeId = std::exchange(rhs.m_typeId, TypeId{});

				memmove_s(m_data, sizeof(m_data), rhs.m_data, sizeof(m_data));
			}

			return *this;
		}

		template <typename T>
		/**
		 * @brief Returns a pointer to the stored value if its type matches the requested type.
		 *
		 * Performs a runtime type check and returns a pointer to the stored value of type `T` if the variant is not empty and the stored type matches `T`. Returns an error if the types do not match or if the variant is empty.
		 *
		 * @return Result<T*, EVariantError> Pointer to the stored value on success, or `EVariantError::NonSameType` on type mismatch or if empty.
		 */
		[[nodiscard]]
		Result<T *, EVariantError> Get() const
		{
			if (m_status == EVariantStatus::None)
			{
				return EVariantError::NonSameType;
			}

			if (m_typeId != GetTypeId<T>())
			{
				return EVariantError::NonSameType;
			}

			if (m_status == EVariantStatus::Small)
			{
				return reinterpret_cast<T *>(&m_data);
			}

			return static_cast<T *>(m_ptr);
		}

		/**
		 * @brief Returns a raw pointer to the stored value if the type identifier matches.
		 *
		 * If the variant is empty or the stored type does not match the provided type identifier, returns an error.
		 *
		 * @param id The runtime type identifier to check against the stored value.
		 * @return Result<void*, EVariantError> Raw pointer to the stored value on success, or EVariantError::NonSameType on type mismatch or if empty.
		 */
		[[nodiscard]]
		Result<void *, EVariantError> GetRaw(TypeId id) const
		{
			if (m_status == EVariantStatus::None)
			{
				return EVariantError::NonSameType;
			}

			if (m_typeId != id)
			{
				return EVariantError::NonSameType;
			}

			if (m_status == EVariantStatus::Small)
			{
				return const_cast<void *>(static_cast<const void *>(&m_data));
			}

			return m_ptr;
		}

	private:
		static constexpr std::size_t MAX_SMALL_SIZE = 16;

		union {
			alignas(std::max_align_t) char m_data[MAX_SMALL_SIZE]{};
			void *m_ptr;
		};
		Dtor m_dtor = nullptr;
		EVariantStatus m_status = EVariantStatus::None;
		TypeId m_typeId;
	};

}