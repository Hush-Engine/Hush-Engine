#include "EcsTerms.hpp"
#include <flecs.h>

#define DEFINE_TERM(Name, FlecsTerm) const Hush::Entity::EntityId Hush::EcsTerms::Name = FlecsTerm

DEFINE_TERM(WILDCARD, EcsWildcard);
DEFINE_TERM(ANY, EcsAny);
DEFINE_TERM(CHILD_OF, EcsChildOf);
