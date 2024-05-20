/*! \file VulkanDeletionQueue.hpp
    \author Kyn21kx
    \date 2024-05-20
    \brief Deletion queue struct
*/

#pragma once
#include <deque>
#include <functional>

struct VulkanDeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void PushFunction(std::function<void()> &&function)
    {
        deletors.push_back(function);
    }

    void Flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)(); // call functors
        }

        deletors.clear();
    }
};