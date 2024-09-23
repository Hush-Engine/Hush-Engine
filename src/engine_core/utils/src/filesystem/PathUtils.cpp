/*! \file PathUtils.cpp
    \author Kyn21kx
    \date 2024-02-25
    \brief Implementation of PathUtils.hpp
*/

#include "PathUtils.hpp"

#include "Logger.hpp"

using PathIterator_t = std::filesystem::recursive_directory_iterator;

bool PathUtils::FindAndAppendSubDirectory(std::filesystem::path &path, const char *targetDirectorySubString)
{
    PathIterator_t iterator = PathIterator_t(path);


    for (auto it = iterator; it != PathIterator_t(); ++it)
    {
        std::filesystem::path nextPath = it->path();

        if (nextPath.string().find(targetDirectorySubString) != std::string::npos)
        {
            // Iterate next.
            auto next = ++it;
            path.assign(next->path());
            return true;
        }
    }

    Hush::LogFormat(Hush::ELogLevel::Debug, "FindAndAppendSubDirectory, no child path was found for directory {}",
                    path.string());
    return false;
}