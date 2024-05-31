/*! \file VkDescriptors.hpp
    \author Kyn21kx
    \date 2024-05-31
    \brief Descriptor definitions
*/

#pragma once


#include <vector>
#include "VkTypes.hpp"
#include <deque>
#include <span>
#include <span>

//> descriptor_layout
struct DescriptorLayoutBuilder
{

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);
};
//< descriptor_layout
//
//> writer
struct DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    void WriteImage(int32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void WriteBuffer(int32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);
};
//< writer
//
//> descriptor_allocator
struct DescriptorAllocator
{

    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void InitPool(VkDevice device, uint32_t maxSets, const std::vector<PoolSizeRatio> &poolRatios);
    void ClearDescriptors(VkDevice device) const;
    void DestroyPool(VkDevice device) const;

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout) const;
};
//< descriptor_allocator

//> descriptor_allocator_grow
struct DescriptorAllocatorGrowable
{
  public:
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    void Init(VkDevice device, uint32_t initialSets, const std::vector<PoolSizeRatio> &poolRatios);
    void ClearPool(VkDevice device);
    void DestroyPool(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);

  private:
    VkDescriptorPool GetPool(VkDevice device);
    VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, const std::vector<PoolSizeRatio> &poolRatios);

    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    uint32_t m_setsPerPool;
};
//< descriptor_allocator_grow