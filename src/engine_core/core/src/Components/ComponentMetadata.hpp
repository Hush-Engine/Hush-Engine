#pragma once

// Comp metadata components go here

#include "reflection/TypeId.hpp"

struct [[hush::export]] InspectableComponent
{
	[[hush::export]]
	Hush::Reflection::TypeId reflectedTypeId;
}; // Raw tag for user-defined components
