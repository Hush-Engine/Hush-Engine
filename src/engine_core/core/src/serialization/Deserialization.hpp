/*! \file Deserialization.hpp
	\author Alan Ramirez
	\date 2025-04-18
	\brief Deserialization types
*/

#pragma once

#include <Result.hpp>
#include <cstdint>
#include <concepts>
#include <optional>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/document.h>
#include <string_view>
#include <span>
#include <stack>
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
		NoneSelfDescribing,
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
		{ t.Deserialize(EFormatDescribingType::NoneSelfDescribing) };
	};

	///
	/// Base class for visitors.
	/// A Serialization visitor is a class that implements the visitor pattern to visit the different types of data
	/// that a format passes. For instance, when a JSON parser sees a number, it will call the VisitInt method of the
	/// visitor.
	///
	/// TODO: how a non-self-describing format will work? We might need to implement a VisitRaw(const char* data, size_t maxSize, size_t currentOffset)?
	class IVisitor
	{
	public:
		IVisitor *parentVisitor{nullptr};
		using Result = Hush::Result<IVisitor *, EDeserializationError>;

		/**
		 * @brief Constructs an IVisitor with an optional parent visitor and format type.
		 *
		 * @param parent Pointer to the parent visitor in the visitor hierarchy.
		 * @param format Specifies whether the data format is self-describing or not.
		 */
		IVisitor(IVisitor *parent, EFormatDescribingType format)
			: parentVisitor(parent)
		{
			// Do we need it in the visitor or just ot enforce a contract?
			(void)format;
		}

		/**
 * @brief Destroys the visitor instance.
 *
 * Ensures proper cleanup of resources in derived visitor classes.
 */
virtual ~IVisitor() = default;

		/// Visit a signed 8-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles visiting an 8-bit signed integer during deserialization.
		 *
		 * @param value The 8-bit signed integer to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitInt8(std::int8_t value)
		{
			(void)value;
			return EDeserializationError::NotSupported;
		}

		/// Visit a signed 16-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles a 16-bit signed integer during deserialization.
		 *
		 * @param value The 16-bit signed integer to process.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitInt16(std::int16_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a signed 32-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles visiting a 32-bit signed integer during deserialization.
		 *
		 * @param value The 32-bit signed integer value to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitInt32(std::int32_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a signed 64-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles a 64-bit signed integer value during deserialization.
		 *
		 * @param value The 64-bit signed integer to process.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitInt64(std::int64_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 8-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles visiting an unsigned 8-bit integer during deserialization.
		 *
		 * @param value The unsigned 8-bit integer value to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitUInt8(std::uint8_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 16-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles visiting a 16-bit unsigned integer during deserialization.
		 *
		 * @param value The 16-bit unsigned integer value to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitUInt16(std::uint16_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 32-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles visiting a 32-bit unsigned integer during deserialization.
		 *
		 * @param value The 32-bit unsigned integer to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitUInt32(std::uint32_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit an unsigned 64-bit integer.
		/// @param value Value to visit
		/**
		 * @brief Handles visiting a 64-bit unsigned integer during deserialization.
		 *
		 * @param value The 64-bit unsigned integer to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitUInt64(std::uint64_t value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a float value.
		/// @param value Value to visit
		/**
		 * @brief Handles a float value during deserialization.
		 *
		 * @param value The float value to process.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitFloat(float value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a double value.
		/// @param value Value to visit
		/**
		 * @brief Handles a double-precision floating-point value during deserialization.
		 *
		 * @param value The double value to process.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitDouble(double value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a boolean value.
		/// @param value Value to visit
		/**
		 * @brief Handles a boolean value during deserialization.
		 *
		 * @param value The boolean value to visit.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitBool(bool value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a string value.
		/// @param value Value to visit
		/**
		 * @brief Handles a string value during deserialization.
		 *
		 * @param value The string value to process.
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitString(std::string_view value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a raw number value.
		/**
		 * @brief Handles a null value during deserialization.
		 *
		 * @return Result containing the next visitor or an error code.
		 */
		virtual Result VisitNull()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit a raw number value.
		/**
		 * @brief Handles the start of an array during deserialization.
		 *
		 * @return Result containing the next visitor for array elements, or an error if arrays are not supported.
		 */
		virtual Result VisitArrayStart()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit the end of an array.
		/**
		 * @brief Handles the end of an array during deserialization.
		 *
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitArrayEnd()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit the start of an object.
		/**
		 * @brief Handles the start of an object during deserialization.
		 *
		 * @return Result containing the next visitor for the object, or an error if not supported.
		 */
		virtual Result VisitObjectStart()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit the end of an object.
		/**
		 * @brief Handles the end of an object during deserialization.
		 *
		 * @return Result containing the next visitor or a deserialization error.
		 */
		virtual Result VisitObjectEnd()
		{
			return EDeserializationError::NotSupported;
		}

		/// Visit a key in an object.
		/// @param value Key to visit
		/**
		 * @brief Handles a key encountered during object deserialization.
		 *
		 * @param value The key as a string view.
		 * @return Result containing the next visitor to handle the value associated with the key, or an error if not supported.
		 */
		virtual Result VisitKey(std::string_view value)
		{
			(void)value;

			return EDeserializationError::NotSupported;
		}

		/// Visit a key in an object.
		/**
		 * @brief Sets the parent visitor for this visitor.
		 *
		 * Updates the internal pointer to the parent visitor, enabling hierarchical traversal during deserialization.
		 */
		void SetParent(IVisitor *parent)
		{
			parentVisitor = parent;
		}
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

			/**
			 * @brief Constructs an integer visitor for deserialization.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target integer value, and the format describing type.
			 */
			IntVisitor(IVisitor *parent, IntType *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			/**
			 * @brief Handles deserialization of an 8-bit unsigned integer value.
			 *
			 * Assigns the provided value to the associated storage and returns the parent visitor.
			 *
			 * @param v The 8-bit unsigned integer to deserialize.
			 * @return Result containing the parent visitor on success.
			 */
			Result VisitUInt8(std::uint8_t v) override
			{
				*value = v;

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of a 16-bit unsigned integer value for an integral target type.
			 *
			 * Assigns the provided `std::uint16_t` value to the target integral type, clamping to the maximum representable value of the target type if necessary. Returns the parent visitor on success.
			 *
			 * @param v The 16-bit unsigned integer value to deserialize.
			 * @return Result The parent visitor on success.
			 */
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

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of a 32-bit unsigned integer into the target integral type.
			 *
			 * If the input value exceeds the maximum representable value of the target type, it is clamped to that maximum. The converted value is assigned to the target, and the parent visitor is returned.
			 *
			 * @param v The 32-bit unsigned integer value to deserialize.
			 * @return Result containing the parent visitor on success.
			 */
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

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of a 64-bit unsigned integer value for an integral type visitor.
			 *
			 * If the input value exceeds the maximum representable by the target integral type, it is clamped to that maximum before assignment.
			 *
			 * @param v The 64-bit unsigned integer value to be deserialized.
			 * @return Result containing the parent visitor on success.
			 */
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

				*value = static_cast<std::uint8_t>(v);

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of an 8-bit signed integer value for an integral type visitor.
			 *
			 * Performs range and sign checks as appropriate for the target integral type, clamps the value if necessary, and assigns it to the underlying storage.
			 *
			 * @param v The 8-bit signed integer value to visit.
			 * @return Result Returns the parent visitor on success, or an error if the value is invalid for the target type.
			 */
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

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of a 16-bit signed integer value for an integral type visitor.
			 *
			 * Performs range and sign checks as appropriate for the target integral type. Assigns the value to the target if valid, clamps to the maximum representable value if necessary, or returns an error for invalid data.
			 *
			 * @param v The 16-bit signed integer value to visit.
			 * @return Result Returns the parent visitor on success, or an error code if the value is invalid for the target type.
			 */
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

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of a 32-bit integer value for an integral type.
			 *
			 * Accepts a 32-bit integer, checks for negative values if the target type is unsigned, clamps the value to the maximum representable by the target type if necessary, assigns it, and returns the parent visitor or an error.
			 *
			 * @param v The 32-bit integer value to deserialize.
			 * @return Result Returns the parent visitor on success, or an error if the value is invalid for the target type.
			 */
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

				return parentVisitor;
			}

			/**
			 * @brief Handles deserialization of a 64-bit signed integer into the target integral type.
			 *
			 * If the target type is unsigned and the input value is negative, returns `InvalidData`. If the input exceeds the maximum representable value of the target type, clamps it to the maximum. On success, assigns the converted value and returns the parent visitor.
			 *
			 * @param v The 64-bit signed integer value to deserialize.
			 * @return Result containing the parent visitor on success, or an error code on failure.
			 */
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

				return parentVisitor;
			}
		};

		template <>
		struct Visitor<int8_t> : public IntVisitor<int8_t>
		{
			using Exists = std::true_type;

			/**
			 * @brief Constructs a visitor for deserializing an 8-bit signed integer.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target int8_t value, and the format describing type.
			 */
			Visitor(IVisitor *parent, int8_t *value, EFormatDescribingType describingType)
				: IntVisitor<int8_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<int16_t> : public IntVisitor<int16_t>
		{
			/**
			 * @brief Constructs a visitor for deserializing 16-bit signed integers.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target int16_t value, and the format describing type.
			 */
			Visitor(IVisitor *parent, int16_t *value, EFormatDescribingType describingType)
				: IntVisitor<int16_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<int32_t> : public IntVisitor<int32_t>
		{
			/**
			 * @brief Constructs a visitor for deserializing 32-bit signed integers.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target int32_t value, and the format describing type.
			 */
			Visitor(IVisitor *parent, int32_t *value, EFormatDescribingType describingType)
				: IntVisitor<int32_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<int64_t> : public IntVisitor<int64_t>
		{
			/**
			 * @brief Visitor for deserializing 64-bit signed integers.
			 *
			 * Initializes an integer visitor for handling deserialization of `int64_t` values.
			 */
			Visitor(IVisitor *parent, int64_t *value, EFormatDescribingType describingType)
				: IntVisitor<int64_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint8_t> : public IntVisitor<uint8_t>
		{
			/**
			 * @brief Constructs a visitor for deserializing an 8-bit unsigned integer.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target value, and the format describing type.
			 */
			Visitor(IVisitor *parent, uint8_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint8_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint16_t> : public IntVisitor<uint16_t>
		{
			/**
			 * @brief Constructs a visitor for deserializing a 16-bit unsigned integer value.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target value, and the format describing type.
			 */
			Visitor(IVisitor *parent, uint16_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint16_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint32_t> : public IntVisitor<uint32_t>
		{
			/**
			 * @brief Constructs a visitor for deserializing a 32-bit unsigned integer.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target value, and the format describing type.
			 */
			Visitor(IVisitor *parent, uint32_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint32_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<uint64_t> : public IntVisitor<uint64_t>
		{
			/**
			 * @brief Constructs a visitor for deserializing a 64-bit unsigned integer.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target value, and the format describing type.
			 */
			Visitor(IVisitor *parent, uint64_t *value, EFormatDescribingType describingType)
				: IntVisitor<uint64_t>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<bool> : public IVisitor
		{
			/**
			 * @brief Constructs a boolean visitor for deserialization.
			 *
			 * Initializes the visitor with a pointer to the parent visitor, a pointer to the target boolean value, and the format describing type.
			 */
			Visitor(IVisitor *parent, bool *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			bool *value{};

			/**
			 * @brief Assigns the provided boolean value to the target and returns this visitor.
			 *
			 * @param v The boolean value to assign.
			 * @return Result Pointer to this visitor on success.
			 */
			Result VisitBool(bool v) override
			{
				*value = v;
				return this;
			}
		};

		template <typename F>
			requires(std::is_floating_point_v<F>)
		struct FloatVisitor : public IVisitor
		{
			using Exists = std::true_type;

			F *value;

			/**
			 * @brief Constructs a floating-point visitor for deserialization.
			 *
			 * @param parent Pointer to the parent visitor in the deserialization hierarchy.
			 * @param value Pointer to the floating-point variable to store the deserialized value.
			 * @param describingType Indicates whether the format is self-describing or not.
			 */
			FloatVisitor(IVisitor *parent, F *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			/**
			 * @brief Handles visiting a float value during deserialization.
			 *
			 * Assigns the provided float value to the target floating-point variable, clamping it to the maximum representable value of type `F` if necessary. Returns the parent visitor on success.
			 *
			 * @param v The float value to assign.
			 * @return Result The parent visitor on success.
			 */
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

				return parentVisitor;
			}

			/**
			 * @brief Visits a double value and assigns it to the target floating-point variable, clamping if necessary.
			 *
			 * If the target type has a smaller range than double, values exceeding the maximum representable value are clamped.
			 * Returns the parent visitor on success.
			 *
			 * @param v The double value to visit.
			 * @return Result The parent visitor on success.
			 */
			Result VisitDouble(double v) override
			{
				if constexpr (std::numeric_limits<F>::max() < std::numeric_limits<double>::max())
				{
					if (*value > std::numeric_limits<F>::max())
					{
						*value = std::numeric_limits<F>::max();
					}
				}

				*this->value = static_cast<F>(v);

				return parentVisitor;
			}
		};

		template <>
		struct Visitor<float> : public FloatVisitor<float>
		{
			/**
			 * @brief Constructs a visitor for deserializing a float value.
			 *
			 * Initializes the float visitor with the given parent visitor, target float pointer, and format describing type.
			 */
			Visitor(IVisitor *parent, float *value, EFormatDescribingType describingType)
				: FloatVisitor<float>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<double> : public FloatVisitor<double>
		{
			/**
			 * @brief Constructs a visitor for deserializing double-precision floating-point values.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target double value, and the format describing type.
			 */
			Visitor(IVisitor *parent, double *value, EFormatDescribingType describingType)
				: FloatVisitor<double>(parent, value, describingType)
			{
			}
		};

		template <>
		struct Visitor<std::string> : public IVisitor
		{
			using Exists = std::true_type;

			/**
			 * @brief Constructs a string visitor for deserialization.
			 *
			 * Initializes the visitor with a parent visitor, a pointer to the target string value, and the format describing type.
			 */
			Visitor(IVisitor *parent, std::string *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType),
				  value(value)
			{
			}

			std::string *value{};

			/**
			 * @brief Assigns the provided string value to the target string during deserialization.
			 *
			 * @param v The string view containing the deserialized value.
			 * @return This visitor instance.
			 */
			Result VisitString(std::string_view v) override
			{
				*value = std::string(v);
				return this;
			}
		};

		template <>
		struct Visitor<std::map<std::string, std::string>> : public IVisitor
		{
			using Exists = std::true_type;

			bool insideObject{false};

			/**
			 * @brief Constructs a visitor for deserializing objects into a map of string keys and values.
			 *
			 * @param parent Pointer to the parent visitor in the deserialization hierarchy.
			 * @param value Pointer to the map where deserialized key-value pairs will be stored.
			 * @param describingType Indicates whether the format is self-describing or not.
			 */
			Visitor(IVisitor *parent, std::map<std::string, std::string> *value, EFormatDescribingType describingType)
				: IVisitor(parent, describingType), value(value)
			{
			}

			std::map<std::string, std::string>* value{};
			std::string currentKey{};

			/**
			 * @brief Marks the beginning of an object during deserialization.
			 *
			 * Sets the internal state to indicate that object parsing has started and returns this visitor.
			 * @return This visitor instance.
			 */
			Result VisitObjectStart() override
			{
				insideObject = true;
				return this;
			}

			/**
			 * @brief Handles the end of an object during deserialization.
			 *
			 * Returns the parent visitor to resume traversal after completing the current object.
			 *
			 * @return Result containing the parent visitor.
			 */
			Result VisitObjectEnd() override
			{
				return this->parentVisitor;
			}

			/**
			 * @brief Handles a key encountered during object deserialization.
			 *
			 * Sets the current key if inside an object; returns an error if not.
			 *
			 * @param v The key as a string view.
			 * @return Result Returns this visitor on success, or `InvalidData` if not inside an object.
			 */
			Result VisitKey(std::string_view v) override
			{
				if (!insideObject)
				{
					return EDeserializationError::InvalidData;
				}

				currentKey = v;

				return this;
			}

			/**
			 * @brief Handles a string value for the current key during object deserialization.
			 *
			 * Inserts or assigns the string value to the map using the current key if inside an object. Returns an error if not inside an object or if the key is empty. After insertion, signals that no further values are supported at this point.
			 *
			 * @param v The string value to associate with the current key.
			 * @return Result Returns NotSupported after insertion, or InvalidData if not inside an object or the key is empty.
			 */
			Result VisitString(std::string_view v) override
			{
				if (!insideObject || currentKey.empty())
				{
					return EDeserializationError::InvalidData;
				}

				this->value->insert_or_assign(std::move(currentKey), std::string(v));

				return EDeserializationError::NotSupported;
			}
		};

		template <typename T>
		concept ExistsBuiltinVisitor = requires() {
			{ BuiltinVisitors::Visitor<T>::Exists } -> std::same_as<std::true_type>;
		};

	} // namespace BuiltinVisitors
} // namespace Hush::Serialization