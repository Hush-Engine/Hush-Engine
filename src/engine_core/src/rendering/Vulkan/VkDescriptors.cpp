#include "VkDescriptors.hpp"

void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::Clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext,
                                                     VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto &b : bindings)
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout set = nullptr;
    HUSH_VK_ASSERT(vkCreateDescriptorSetLayout(device, &info, nullptr, &set), "Failed to create descriptor set layout!");

    return set;
}

void DescriptorWriter::WriteImage(int32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout,
                                  VkDescriptorType type)
{
    VkDescriptorImageInfo toInsert = {};
    toInsert.sampler = sampler;
    toInsert.imageView = image;
    toInsert.imageLayout = layout;

    VkDescriptorImageInfo &info = imageInfos.emplace_back(toInsert);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::WriteBuffer(int32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo toInsert = {};
    toInsert.buffer = buffer;
    toInsert.offset = offset;
    toInsert.range = size;

    VkDescriptorBufferInfo &info = bufferInfos.emplace_back(toInsert);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::Clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet &write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void DescriptorAllocator::InitPool(VkDevice device, uint32_t maxSets, const std::vector<PoolSizeRatio> &poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        VkDescriptorPoolSize toInsert = {};
        toInsert.type = ratio.type;
        // NOLINTNEXTLINE
        toInsert.descriptorCount = uint32_t(ratio.ratio * maxSets);
        poolSizes.push_back(toInsert);
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
}

void DescriptorAllocator::ClearDescriptors(VkDevice device) const
{
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::DestroyPool(VkDevice device) const
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout) const
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds = nullptr;
    HUSH_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, &ds), "Failed to allocate descriptor sets!");

    return ds;
}

void DescriptorAllocatorGrowable::Init(VkDevice device, uint32_t initialSets,
                                       const std::vector<PoolSizeRatio> &poolRatios)
{
    this->m_ratios.clear();

    for (auto r : poolRatios)
    {
        this->m_ratios.push_back(r);
    }

    VkDescriptorPool newPool = this->CreatePool(device, initialSets, poolRatios);

    // NOLINTNEXTLINE
    this->m_setsPerPool = static_cast<uint32_t>(initialSets * 1.5); // grow it next allocation

    this->m_readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::ClearPool(VkDevice device)
{
    for (auto *p : this->m_readyPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto *p : this->m_fullPools)
    {
        vkResetDescriptorPool(device, p, 0);
        this->m_readyPools.push_back(p);
    }
    this->m_fullPools.clear();
}

void DescriptorAllocatorGrowable::DestroyPool(VkDevice device)
{
    for (auto *p : this->m_readyPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    this->m_readyPools.clear();
    for (auto *p : this->m_fullPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    this->m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::Allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext)
{
    // get or create a pool to allocate from
    VkDescriptorPool poolToUse = this->GetPool(device);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext = pNext;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds = nullptr;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    // allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {

        this->m_fullPools.push_back(poolToUse);

        poolToUse = this->GetPool(device);
        allocInfo.descriptorPool = poolToUse;

        HUSH_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, &ds), "Failed to allocate descriptor sets!");
    }

    this->m_readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice device)
{
    VkDescriptorPool newPool = nullptr;
    if (!this->m_readyPools.empty())
    {
        newPool = this->m_readyPools.back();
        this->m_readyPools.pop_back();
        return newPool;
    }

    // need to create a new pool
    newPool = this->CreatePool(device, this->m_setsPerPool, this->m_ratios);

    // NOLINTNEXTLINE
    this->m_setsPerPool = static_cast<uint32_t>(this->m_setsPerPool * 1.5);
    const int32_t maxSetsPerPool = 4092;
    if (this->m_setsPerPool > maxSetsPerPool)
    {
        this->m_setsPerPool = maxSetsPerPool;
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice device, uint32_t setCount,
                                                         const std::vector<PoolSizeRatio> &poolRatios)
{
    (void)device;
    (void)setCount;
    (void)poolRatios;
    return VkDescriptorPool();
}
