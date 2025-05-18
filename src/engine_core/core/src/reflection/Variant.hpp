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

		VariantView() : m_value(nullptr), m_typeId(TypeId{}) {}

		template <typename T>
		explicit VariantView(T *value)
			: m_value(value),
			  m_typeId(Hush::Reflection::GetTypeId<T>())
		{
			static_assert(!std::is_same_v<T, void>, "VariantView cannot hold void type");
		}

		template <typename T> requires (!std::is_pointer_v<T>)
		explicit VariantView(T &value)
			: m_value(&value),
			  m_typeId(Hush::Reflection::GetTypeId<T>())
		{
			static_assert(!std::is_same_v<T, void>, "VariantView cannot hold void type");
		}

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

		Variant(const Variant &) = delete;
		Variant &operator=(const Variant &) = delete;

		Variant(Variant &&rhs) noexcept
			: m_dtor(std::exchange(rhs.m_dtor, nullptr)),
			  m_status(std::exchange(rhs.m_status, EVariantStatus::None)),
			  m_typeId(std::exchange(rhs.m_typeId, TypeId{}))
		{
			memmove_s(m_data, sizeof(m_data), rhs.m_data, sizeof(m_data));
		}

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