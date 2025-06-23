/*! \file Deserialization.hpp
	\author Alan Ramirez
	\date 2025-04-18
	\brief Deserialization types
*/

#pragma once

#include <Result.hpp>
#include <cstdint>
#include <concepts>
#include <rapidjson/document.h>
#include <string_view>
#include <map>
#include <unordered_map>

namespace Hush::Serialization
{
	///
	/// Describing format type.
	/// A self-describing format is a format that includes the type information in the format itself.
	/// For instance, JSON contains the name of the field along with the value (which is of a specific type).
	/// A non-self-describing format is a format that does not include the type information in the format itself.
	/// For instance, a binary format may not include the name of the field along with the value.
	enum class EFormatDescribingType
	{
		/// Self-describing format.
		SelfDescribing,
		/// Non-self-describing format.
		NonSelfDescribing,
	};

	///
	/// SerializationError enum class
	enum class EDeserializationError
	{
		None,
		InvalidType,
		InvalidData,
		InvalidFormat,
		InvalidKey,
		NotSupported,
	};

	template <typename A, typename T>
	concept IsDeserializer = requires(A a, T t) {
		{ t.Deserialize(a) } -> std::same_as<EDeserializationError>;
	};

	class IVisitor;

	// Is deserialize returns an instance of a class derived from IVisitor
	template <typename T>
	concept IsDeserializable = requires(T t) {
		{ t.Deserialize(static_cast<IVisitor *>(nullptr), EFormatDescribingType::NonSelfDescribing) };
	};

	///
	/// Base class for visitors.
	/// A Serialization visitor is a class that implements the visitor pattern to visit the different types of data
	/// that a format passes. For instance, when a JSON parser sees a number, it will call the VisitInt method of the
	/// visitor.
	///
	/// TODO(Alan): how a non-self-describing format will work? We might need to implement a VisitRaw(const char* data,
	/// size_t maxSize, size_t currentOffset)?
	class IVisitor
	{
	public:
		using Result = Hush::Result<IVisitor *, EDeserializationError>;

		IVisitor(IVisitor *parent, EFormatDescribingType format)
			: m_parentVisitor(parent)
		{
			// Do we need it in the visitor or just ot enforce a contract?
			(void)format;
		}

		virtual ~IVisitor() = default;

		/// Visit a signed 8-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitInt8(std::int8_t value)
		{
			(void)value;
			return EDeserializationError::NotSupported;
		}

		/// Visit a signed 16-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitInt16(std::int16_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a signed 32-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitInt32(std::int32_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a signed 64-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitInt64(std::int64_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 8-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitUInt8(std::uint8_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 16-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitUInt16(std::uint16_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 32-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitUInt32(std::uint32_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 64-bit integer.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitUInt64(std::uint64_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a float value.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitFloat(float value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a double value.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitDouble(double value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a boolean value.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitBool(bool value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a string value.
		/// @param value Value to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitString(std::string_view value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a null value.
		/// @return Result with the next visitor or an error.
		virtual Result VisitNull()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit an array start.
		/// @return Result with the next visitor or an error.
		virtual Result VisitArrayStart()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit the end of an array.
		/// @return Result with the next visitor or an error.
		virtual Result VisitArrayEnd()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit the start of an object.
		/// @return Result with the next visitor or an error.
		virtual Result VisitObjectStart()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit the end of an object.
		/// @return Result with the next visitor or an error.
		virtual Result VisitObjectEnd()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit a key in an object.
		/// @param value Key to visit
		/// @return Result with the next visitor or an error.
		virtual Result VisitKey(std::string_view value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a key in an object.
		/// @param parent Parent visitor
		void SetParentVisitor(IVisitor *parent)
		{
			m_parentVisitor = parent;
		}

		/// Get the parent visitor.
		/// @return Parent visitor
		[[nodiscard]]
		IVisitor *GetParentVisitor() const
		{
			return m_parentVisitor;
		}

		/// Get the starting visitor.
		/// @return Starting visitor
		[[nodiscard]]
		IVisitor *GetStartVisitor() const
		{
			return m_startingVisitor;
		}

	protected:
		IVisitor *SetStartingVisitor(IVisitor *startingVisitor)
		{
			m_startingVisitor = startingVisitor;
			return m_startingVisitor;
		}

	private:
		IVisitor *m_parentVisitor{nullptr};
		IVisitor *m_startingVisitor{nullptr};
	};

	namespace BuiltinVisitors
	{
		template <typename T>
		struct Visitor : public IVisitor
		{
			using Exists = std::false_type;
		};

		template <typename IntType>
		struct IntVisitor : public IVisitor
		{
			using Exists = std::true_type;

			IntType *value;

			static constexpr bool IsUnsigned = std::is_unsigned_v<IntType>;
			static_assert(std::is_integral_v<IntType>, "IntVisitor must be specialized for integral types");

			IntVisitor(IVisitor *parent, IntType *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			Result VisitUInt8(std::uint8_t v) override
			{
				*value = v;

				return GetParentVisitor();
			}

			Result VisitUInt16(std::uint16_t v) override
			{
				if constexpr (std::numeric_limits<IntType>::max() < std::numeric_limits<std::uint16_t>::max())
				{
					if (v > std::numeric_limits<IntType>::max())
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}

			Result VisitUInt32(std::uint32_t v) override
			{
				if constexpr (static_cast<uint64_t>(std::numeric_limits<IntType>::max()) <
							  static_cast<uint64_t>(std::numeric_limits<std::uint32_t>::max()))
				{
					if (v > 0 && static_cast<uint64_t>(v) > std::numeric_limits<IntType>::max())
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}

			Result VisitUInt64(std::uint64_t v) override
			{
				if constexpr (static_cast<uint64_t>(std::numeric_limits<IntType>::max()) <
							  std::numeric_limits<std::uint64_t>::max())
				{
					if (v > static_cast<uint64_t>(std::numeric_limits<IntType>::max()))
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}

			Result VisitInt8(std::int8_t v) override
			{
				if constexpr (IsUnsigned)
				{
					if (v < 0)
					{
						return EDeserializationError::InvalidData;
					}
				}

				if constexpr (static_cast<uint64_t>(std::numeric_limits<IntType>::max()) <
							  static_cast<uint64_t>(std::numeric_limits<std::int8_t>::max()))
				{
					if (v > std::numeric_limits<IntType>::max())
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}

			Result VisitInt16(std::int16_t v) override
			{
				if constexpr (IsUnsigned)
				{
					if (v < 0)
					{
						return EDeserializationError::InvalidData;
					}
				}

				if constexpr (static_cast<uint64_t>(std::numeric_limits<IntType>::max()) <
							  static_cast<uint64_t>(std::numeric_limits<std::int16_t>::max()))
				{
					if (v > std::numeric_limits<IntType>::max())
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}

			Result VisitInt32(std::int32_t v) override
			{
				if constexpr (IsUnsigned)
				{
					if (v < 0)
					{
						return EDeserializationError::InvalidData;
					}
				}

				if constexpr (static_cast<uint64_t>(std::numeric_limits<IntType>::max()) <
							  static_cast<uint64_t>(std::numeric_limits<std::int32_t>::max()))
				{
					if (v > std::numeric_limits<IntType>::max())
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}

			Result VisitInt64(std::int64_t v) override
			{
				if constexpr (IsUnsigned)
				{
					if (v < 0)
					{
						return EDeserializationError::InvalidData;
					}
				}

				using BiggerType = std::conditional_t<IsUnsigned, std::uint64_t, std::int64_t>;

				if constexpr (static_cast<BiggerType>(std::numeric_limits<IntType>::max()) <
							  static_cast<BiggerType>(std::numeric_limits<std::int64_t>::max()))
				{
					if (v > std::numeric_limits<IntType>::max())
					{
						v = std::numeric_limits<IntType>::max();
					}
				}

				*value = static_cast<IntType>(v);

				return GetParentVisitor();
			}
		};

		template <>
		struct Visitor<int8_t> : public IntVisitor<int8_t>
		{
			using Exists = std::true_type;

			Visitor(IVisitor *parent, int8_t *value, EFormatDescribingType describingType)
				: IntVisitor<int8_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<int16_t> : public IntVisitor<int16_t>
		{
			Visitor(IVisitor *parent, int16_t *value, EFormatDescribingType describingType)
				: IntVisitor<int16_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<int32_t> : public IntVisitor<int32_t>
		{
			Visitor(IVisitor *parent, int32_t *value, EFormatDescribingType describingType)
				: IntVisitor<int32_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<int64_t> : public IntVisitor<int64_t>
		{
			Visitor(IVisitor *parent, int64_t *value, EFormatDescribingType describingType)
				: IntVisitor<int64_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint8_t> : public IntVisitor<uint8_t>
		{
			Visitor(IVisitor *parent, uint8_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint8_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint16_t> : public IntVisitor<uint16_t>
		{
			Visitor(IVisitor *parent, uint16_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint16_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint32_t> : public IntVisitor<uint32_t>
		{
			Visitor(IVisitor *parent, uint32_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint32_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint64_t> : public IntVisitor<uint64_t>
		{
			Visitor(IVisitor *parent, uint64_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint64_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<bool> : public IVisitor
		{
			Visitor(IVisitor *parent, bool *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			bool *value{};

			Result VisitBool(bool v) override
			{
				*value = v;
				return GetParentVisitor();
			}
		};

		template <typename F>
			requires(std::is_floating_point_v<F>)
		struct FloatVisitor : public IVisitor
		{
			using Exists = std::true_type;

			F *value;

			FloatVisitor(IVisitor *parent, F *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			Result VisitFloat(float v) override
			{
				if constexpr (std::numeric_limits<F>::max() < std::numeric_limits<float>::max())
				{
					if (v > std::numeric_limits<F>::max())
					{
						v = std::numeric_limits<F>::max();
					}
				}

				*value = static_cast<F>(v);

				return GetParentVisitor();
			}

			Result VisitDouble(double v) override
			{
				if constexpr (std::numeric_limits<F>::max() < std::numeric_limits<double>::max())
				{
					if (v > std::numeric_limits<F>::max())
					{
						v = std::numeric_limits<F>::max();
					}
				}
				*this->value = static_cast<F>(v);

				return GetParentVisitor();
			}
		};

		template <>
		struct Visitor<float> : public FloatVisitor<float>
		{
			using Exists = std::true_type;
			Visitor(IVisitor *parent, float *value, EFormatDescribingType describingType)
				: FloatVisitor<float>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<double> : public FloatVisitor<double>
		{
			Visitor(IVisitor *parent, double *value, EFormatDescribingType describingType)
				: FloatVisitor<double>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<std::string> : public IVisitor
		{
			using Exists = std::true_type;

			Visitor(IVisitor *parent, std::string *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			std::string *value{};

			Result VisitString(std::string_view v) override
			{
				*value = std::string(v);
				return GetParentVisitor();
			}
		};

		template <>
		struct Visitor<std::map<std::string, std::string>> : public IVisitor
		{
			using Exists = std::true_type;

			bool insideObject{false};

			Visitor(IVisitor *parent, std::map<std::string, std::string> *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			std::map<std::string, std::string> *value{};
			std::string currentKey{};

			Result VisitObjectStart() override
			{
				insideObject = true;
				return this;
			}

			Result VisitObjectEnd() override
			{
				return this->GetParentVisitor();
			}

			Result VisitKey(std::string_view v) override
			{
				if (!insideObject)
				{
					return EDeserializationError::InvalidData;
				}

				currentKey = v;

				return this;
			}

			Result VisitString(std::string_view v) override
			{
				if (!insideObject || currentKey.empty())
				{
					return EDeserializationError::InvalidData;
				}

				this->value->insert_or_assign(std::move(currentKey), std::string(v));

				return this;
			}
		};

		template <typename T>
		concept ExistsBuiltinVisitor = std::same_as<typename BuiltinVisitors::Visitor<T>::Exists, std::true_type>;
	} // namespace BuiltinVisitors

	template <typename T, typename = void>
	struct Visitor : public IVisitor
	{
		Visitor(IVisitor *parent, T *value, EFormatDescribingType describingType) : IVisitor(parent, describingType)
		{
			(void)parent;
			(void)value;
			(void)describingType;
		}
	};

	template <typename T> requires (BuiltinVisitors::ExistsBuiltinVisitor<T> && !IsDeserializable<T>)
	struct Visitor<T> : public BuiltinVisitors::Visitor<T>
	{
		Visitor(IVisitor *parent, T *value, EFormatDescribingType describingType)
			: BuiltinVisitors::Visitor<T>(parent, value, describingType)
		{
		}
	};

	template <IsDeserializable T>
	struct Visitor<T> : public decltype(std::declval<T&>().Deserialize(std::declval<IVisitor*>(), EFormatDescribingType::NonSelfDescribing))
	{
		using Parent = decltype(std::declval<T&>().Deserialize(std::declval<IVisitor*>(), EFormatDescribingType::NonSelfDescribing));
		using Type = T;

		Visitor(IVisitor *parent, T *value, EFormatDescribingType describingType)
			: Parent(parent, value, describingType)
		{
		}
	};


} // namespace Hush::Serialization