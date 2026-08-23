/*! \file bindings.cpp
	\author Alan Ramirez
	\date 2025-03-30
	\brief Hush Engine bindings container
*/
// NOLINTBEGIN
#include "bindings.hpp"

#if !defined(HUSH_HEADER_PARSING) && __has_include("./HushBindings.cpp")
#define HUSH_STATIC_BINDING
#include "./HushBindings.cpp"
#undef HUSH_STATIC_BINDING
#endif

// NOLINTEND
