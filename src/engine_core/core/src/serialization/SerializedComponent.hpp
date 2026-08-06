#include <string>
#include <Hushgen.hpp>

#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>

#if __has_include("SerializedComponent.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "SerializedComponent.hushgen.hpp"
#endif

namespace Hush
{

	struct [[hush::reflect]] SerializedComponent
	{
		HUSH_GENERATED_BODY
	public:
		std::string key;
		// JSON needs to be turned into component data at runtime, we shouldn't own a void* or any other templated
		// type into it
		std::string jsonData;
	};
} // namespace Hush
