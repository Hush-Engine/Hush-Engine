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
#include <optional>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

		IVisitor(const IVisitor &) = default;
		IVisitor(IVisitor &&) = delete;
		IVisitor &operator=(const IVisitor &) = default;
		IVisitor &operator=(IVisitor &&) = delete;

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

			static constexpr bool IS_UNSIGNED = std::is_unsigned_v<IntType>;
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
				if constexpr (IS_UNSIGNED)
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
				if constexpr (IS_UNSIGNED)
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
				if constexpr (IS_UNSIGNED)
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
				if constexpr (IS_UNSIGNED)
				{
					if (v < 0)
					{
						return EDeserializationError::InvalidData;
					}
				}

				using BiggerType = std::conditional_t<IS_UNSIGNED, std::uint64_t, std::int64_t>;

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
		struct Visitor<glm::mat4> : public IVisitor
		{
			using Exists = std::true_type;

			glm::mat4 *value{};
			std::int32_t m_index = 0;
			bool insideArray{false};

			Visitor(IVisitor *parent, glm::mat4 *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			Result VisitArrayStart() override
			{
				if (insideArray)
				{
					return EDeserializationError::InvalidData;
				}

				insideArray = true;
				m_index = 0;

				return this;
			}

			Result VisitArrayEnd() override
			{
				if (!insideArray || m_index != 16)
				{
					return EDeserializationError::InvalidData;
				}

				insideArray = false;

				return GetParentVisitor();
			}

			Result VisitFloat(float v) override
			{
				if (!insideArray || m_index >= 16)
				{
					return EDeserializationError::InvalidData;
				}

				glm::value_ptr(*value)[m_index++] = v;

				return this;
			}

			Result VisitDouble(double v) override
			{
				return VisitFloat(static_cast<float>(v));
			}
		};

		template <glm::length_t L, glm::qualifier Q>
		struct Visitor<glm::vec<L, float, Q>> : public IVisitor
		{
			using Exists = std::true_type;

			glm::vec<L, float, Q> *value{};
			glm::length_t m_index = 0;
			bool insideArray{false};

			Visitor(IVisitor *parent, glm::vec<L, float, Q> *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			Result VisitArrayStart() override
			{
				if (insideArray)
				{
					return EDeserializationError::InvalidData;
				}

				insideArray = true;
				m_index = 0;

				return this;
			}

			Result VisitArrayEnd() override
			{
				if (!insideArray || m_index != L)
				{
					return EDeserializationError::InvalidData;
				}

				insideArray = false;

				return GetParentVisitor();
			}

			Result VisitFloat(float v) override
			{
				if (!insideArray || m_index >= L)
				{
					return EDeserializationError::InvalidData;
				}

				glm::value_ptr(*value)[m_index++] = v;

				return this;
			}

			Result VisitDouble(double v) override
			{
				return VisitFloat(static_cast<float>(v));
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

		// BUG: This visitor's @ref VisitKey returns `this` (line 792) even though it
		// handles the key+value itself (the value arrives via @ref VisitString). The
		// JsonDeserializer::RapidjsonVisitor bridge interprets "VisitKey returned the
		// same visitor" as an unknown member and skips the following value, so this
		// visitor's values would never be consumed. It is currently unused anywhere
		// in the codebase; if it is ever needed, VisitKey must instead return a
		// dedicated sub-visitor (or otherwise signal that the value is handled).
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
			std::string currentKey;

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
		Visitor(IVisitor *parent, T *value, EFormatDescribingType describingType)
			: IVisitor(parent, describingType)
		{
			(void)parent;
			(void)value;
			(void)describingType;
		}
	};

	template <typename T>
		requires(BuiltinVisitors::ExistsBuiltinVisitor<T> && !IsDeserializable<T>)
	struct Visitor<T> : public BuiltinVisitors::Visitor<T>
	{
		Visitor(IVisitor *parent, T *value, EFormatDescribingType describingType)
			: BuiltinVisitors::Visitor<T>(parent, value, describingType)
		{
		}
	};

	template <IsDeserializable T>
	struct Visitor<T> : public decltype(std::declval<T &>().Deserialize(std::declval<IVisitor *>(),
																		EFormatDescribingType::NonSelfDescribing))
	{
		using Parent = decltype(std::declval<T &>().Deserialize(std::declval<IVisitor *>(),
																EFormatDescribingType::NonSelfDescribing));
		using Type = T;

		Visitor(IVisitor * parent, T * value, EFormatDescribingType describingType)
			: Parent(parent, value, describingType)
		{
		}
	};

	/// Specialization of @ref Visitor for vectors of reflected (deserializable) element types.
	/// Each array element is deserialized through a per-element @ref Visitor<T> that is constructed
	/// when the element object starts. Once the element object ends, control returns to this visitor
	/// so the next array element (or the array end) can be handled.
	/// For vectors of primitive element types, see @ref Visitor<std::vector<BuiltinVisitors::ExistsBuiltinVisitor>>
	/// below.
	template <IsDeserializable T>
	struct Visitor<std::vector<T>> : public IVisitor
	{
		std::vector<T> *value;
		EFormatDescribingType m_format;
		bool insideArray{false};
		std::optional<Visitor<T>> m_elementVisitor;

		Visitor(IVisitor *parent, std::vector<T> *value, EFormatDescribingType describingType)
			: IVisitor(parent, describingType),
			  value(value),
			  m_format(describingType)
		{
		}

		Result VisitArrayStart() override
		{
			if (insideArray)
			{
				return EDeserializationError::InvalidData;
			}

			insideArray = true;
			value->clear();

			return this;
		}

		Result VisitArrayEnd() override
		{
			if (!insideArray)
			{
				return EDeserializationError::InvalidData;
			}

			insideArray = false;
			m_elementVisitor.reset();

			return GetParentVisitor();
		}

		Result VisitObjectStart() override
		{
			if (!insideArray || m_elementVisitor.has_value())
			{
				return EDeserializationError::InvalidData;
			}

			value->emplace_back();
			m_elementVisitor.emplace(this, &value->back(), m_format);

			return &*m_elementVisitor;
		}

		Result VisitObjectEnd() override
		{
			if (!insideArray || !m_elementVisitor.has_value())
			{
				return EDeserializationError::InvalidData;
			}

			m_elementVisitor.reset();

			return this;
		}
	};

	/// Specialization of @ref Visitor for vectors of builtin (primitive) element types.
	/// Scalar array elements are deserialized through a per-element
	/// @ref BuiltinVisitors::Visitor<T> that writes into `value->back()` and then hands
	/// control back to this visitor so the next element (or the array end) can be handled.
	template <BuiltinVisitors::ExistsBuiltinVisitor T>
		requires(!IsDeserializable<T>)
	struct Visitor<std::vector<T>> : public IVisitor
	{
		std::vector<T> *value;
		EFormatDescribingType m_format;
		bool insideArray{false};
		std::optional<BuiltinVisitors::Visitor<T>> m_elementVisitor;

		Visitor(IVisitor *parent, std::vector<T> *value, EFormatDescribingType describingType)
			: IVisitor(parent, describingType),
			  value(value),
			  m_format(describingType)
		{
		}

		Result VisitArrayStart() override
		{
			if (insideArray)
			{
				return EDeserializationError::InvalidData;
			}

			insideArray = true;
			value->clear();

			return this;
		}

		Result VisitArrayEnd() override
		{
			if (!insideArray)
			{
				return EDeserializationError::InvalidData;
			}

			insideArray = false;
			m_elementVisitor.reset();

			return GetParentVisitor();
		}

		Result VisitInt32(std::int32_t v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitInt32(v); });
		}

		Result VisitUInt32(std::uint32_t v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitUInt32(v); });
		}

		Result VisitInt64(std::int64_t v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitInt64(v); });
		}

		Result VisitUInt64(std::uint64_t v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitUInt64(v); });
		}

		Result VisitFloat(float v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitFloat(v); });
		}

		Result VisitDouble(double v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitDouble(v); });
		}

		Result VisitBool(bool v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitBool(v); });
		}

		Result VisitString(std::string_view v) override
		{
			return ConsumeElement([&](BuiltinVisitors::Visitor<T> &element) { return element.VisitString(v); });
		}

	private:
		template <typename ConsumeFn>
		Result ConsumeElement(ConsumeFn &&consume)
		{
			if (!insideArray || m_elementVisitor.has_value())
			{
				return EDeserializationError::InvalidData;
			}

			value->emplace_back();
			m_elementVisitor.emplace(this, &value->back(), m_format);

			Result consumed = std::forward<ConsumeFn>(consume)(*m_elementVisitor);
			if (consumed.has_error())
			{
				return consumed.error();
			}

			m_elementVisitor.reset();

			return this;
		}
	};

} // namespace Hush::Serialization
