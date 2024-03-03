/*! \file EnumUtils.hpp
    \author Kyn21kx
    \date 2024-03-01
    \brief THIS SHOULD BE A DEBUGGING TOOL, DO NOT CALL THESE FUNCTIONS ON RELEASE OR DIST BUILDS XD
*/

#pragma once
// Taken from https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings
#undef DECL_ENUM_ELEMENT
#undef BEGIN_ENUM
#undef END_ENUM

#ifndef GENERATE_ENUM_STRINGS
#define DECL_ENUM_ELEMENT(element) element
#define BEGIN_ENUM(ENUM_NAME) typedef enum class tag##ENUM_NAME
#define END_ENUM(ENUM_NAME)                                                                                            \
    ENUM_NAME;                                                                                                         \
    char *GetString##ENUM_NAME(enum tag##ENUM_NAME index);
#else
#define DECL_ENUM_ELEMENT(element) #element
#define BEGIN_ENUM(ENUM_NAME) char *gs_##ENUM_NAME[] =
#define END_ENUM(ENUM_NAME)                                                                                            \
    ;                                                                                                                  \
    char *GetString##ENUM_NAME(enum tag##ENUM_NAME index)                                                              \
    {                                                                                                                  \
        return gs_##ENUM_NAME[index];                                                                                  \
    }
#endif