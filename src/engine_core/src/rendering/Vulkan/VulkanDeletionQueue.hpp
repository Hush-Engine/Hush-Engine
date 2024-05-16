/*! \file VulkanDeletionQueue.hpp
    \author Leonidas Gonzalez
    \date 2024-05-13
    \brief Deletion queue struct for Vulkan
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