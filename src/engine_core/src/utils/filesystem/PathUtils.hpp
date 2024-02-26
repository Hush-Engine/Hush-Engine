/*! \file PathUtils.hpp
	\author Kyn21kx
	\date 2024-02-25
	\brief Provides utility methods for handling path operations such as finding child directories
*/

#pragma once

#include <filesystem>
#include "Logger.hpp"

class PathUtils
{
  public:
    static bool FindAndAppendSubDirectory(std::filesystem::path &path, const char *targetDirectorySubString);
};
