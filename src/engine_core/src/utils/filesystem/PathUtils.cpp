/*! \file PathUtils.cpp
    \author Kyn21kx
    \date 2024-02-25
    \brief Implementation of PathUtils.hpp
*/

#include "PathUtils.hpp"

#include "log/Logger.hpp"

using PathIterator_t = std::filesystem::recursive_directory_iterator;

bool PathUtils::FindAndAppendSubDirectory(std::filesystem::path &path, const char *targetDirectorySubString)
{
    PathIterator_t iterator = PathIterator_t(path);
    iterator++;
    std::filesystem::path nextPath = iterator->path();
    if (!iterator->exists() || nextPath.string().find(targetDirectorySubString) == std::string::npos)
    {
        Hush::LogFormat(Hush::ELogLevel::Debug, "FindAndAppendSubDirectory, no child path was found for directory {}",
                        path.string());
        return false;
    }
    path.assign(nextPath);
    return true;
}