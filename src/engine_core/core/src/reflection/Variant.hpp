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
	class Variant;

	/// A VariantView that holds a reference to any type.
	struct VariantView
	{
		enum class EVariantError : uint8_t
		{
			NonSameType = 0,
		};

		VariantView()
			: m_value(nullptr),
			  m_typeId(TypeId{})
		{
		}

		template <typename T>
		explicit VariantView(T *value)
			: m_value(value),
			  m_typeId(Hush::Reflection::GetTypeId<T>())
		{
			static_assert(!std::is_same_v<T, void>, "VariantView cannot hold void type");
		}

		template <typename T>
			requires(!std::is_pointer_v<T>)
		explicit VariantView(T &value)
			: m_value(&value),
			  m_typeId(Hush::Reflection::GetTypeId<T>())
		{
			static_assert(!std::is_same_v<T, void>, "VariantView cannot hold void type");
		}

		VariantView(const Variant &variant);

		explicit VariantView(void *value, std::uint64_t typeId)
			: m_value(value),
			  m_typeId(typeId)
		{
		}

		template <typename T>
		[[nodiscard]]
		Result<T *, EVariantError> Get() const
		{
			if (m_typeId != Reflection::GetTypeId<T>())
			{
				return EVariantError::NonSameType;
			}

			return static_cast<T *>(m_value);
		}

		[[nodiscard]]
		void *GetRaw(TypeId id) const
		{
			if (m_typeId != id)
			{
				return nullptr;
			}

			return m_value;
		}

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

		Variant()
			: m_ptr(nullptr),
			  m_typeId(TypeId{})
		{
		}

		template <typename T>
		explicit Variant(T &&value)
			: m_typeId(GetTypeId<std::remove_cvref_t<T>>())
		{
			// T can be deduced as a reference when the caller passes an lvalue (e.g. Variant(instance->field)),
			// so always strip the reference before storing it. Allocating a reference (new T) is ill-formed.
			using ValueType = std::remove_reference_t<T>;

			if constexpr (sizeof(ValueType) <= MAX_SMALL_SIZE)
			{
				new (m_data) ValueType(std::forward<T>(value));
				m_status = EVariantStatus::Small;
				if constexpr (std::is_trivially_destructible_v<ValueType>)
				{
					m_dtor = nullptr;
				}
				else
				{
					m_dtor = [](void *ptr) { static_cast<ValueType *>(ptr)->~ValueType(); };
				}
			}
			else
			{
				m_ptr = new ValueType(std::forward<T>(value));
				m_dtor = [](void *ptr) { delete static_cast<ValueType *>(ptr); };
				m_status = EVariantStatus::Large;
			}
		}

		template <typename T, typename... Args>
		static Variant CreateInPlace(Args &&...args)
		{
			static_assert(std::is_constructible_v<T, Args...>,
						  "Type T is not constructible with the provided arguments");

			Variant variant;
			variant.m_typeId = GetTypeId<T>();

			if constexpr (sizeof(T) <= MAX_SMALL_SIZE)
			{
				new (variant.m_data) T(std::forward<Args>(args)...);
				variant.m_status = EVariantStatus::Small;
				if constexpr (std::is_trivially_destructible_v<T>)
				{
					variant.m_dtor = nullptr;
				}
				else
				{
					variant.m_dtor = [](void *ptr) { static_cast<T *>(ptr)->~T(); };
				}
			}
			else
			{
				variant.m_ptr = new T(std::forward<Args>(args)...);
				variant.m_dtor = [](void *ptr) { delete static_cast<T *>(ptr); };
				variant.m_status = EVariantStatus::Large;
			}

			return variant;
		}

		Variant(const Variant &) = delete;
		Variant &operator=(const Variant &) = delete;

		Variant(Variant &&rhs) noexcept;

		~Variant();

		Variant &operator=(Variant &&rhs) noexcept
		{
			if (this != &rhs)
			{
				Clear();

				m_dtor = std::exchange(rhs.m_dtor, nullptr);
				m_status = std::exchange(rhs.m_status, EVariantStatus::None);
				m_typeId = std::exchange(rhs.m_typeId, TypeId{});

				std::memcpy(&m_data, &rhs.m_data, sizeof(m_data));
			}

			return *this;
		}

		template <typename T>
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
				return static_cast<T *>(const_cast<void *>(static_cast<const void *>(m_data)));
			}

			return static_cast<T *>(m_ptr);
		}

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

		void Clear()
		{
			if (m_dtor != nullptr && m_status != EVariantStatus::None)
			{
				void *ptrToFree = m_status == EVariantStatus::Small ? reinterpret_cast<void *>(&m_data[0]) : m_ptr;
				m_dtor(ptrToFree);
			}

			m_status = EVariantStatus::None;
			m_dtor = nullptr;
			m_typeId = TypeId{};
		}

		template <typename T>
		[[nodiscard]]
		bool IsType() const
		{
			return m_typeId == GetTypeId<T>();
		}

		[[nodiscard]]
		bool IsType(TypeId id) const
		{
			return m_typeId == id;
		}

		TypeId StoredTypeId() const
		{
			return m_typeId;
		}

	private:
		friend struct VariantView;
		static constexpr std::size_t MAX_SMALL_SIZE = 16;

		union {
			alignas(std::max_align_t) char m_data[MAX_SMALL_SIZE]{};
			void *m_ptr;
		};
		Dtor m_dtor = nullptr;
		EVariantStatus m_status = EVariantStatus::None;
		TypeId m_typeId;
	};

} // namespace Hush::Reflection