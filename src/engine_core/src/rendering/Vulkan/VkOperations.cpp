#include "VkOperations.hpp"
#include "VulkanRenderer.hpp"
#include "VkUtilsFactory.hpp"
#include "../Shared/Colors.hpp"
#include <fastgltf/tools.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>


bool Hush::VkOperations::LoadShaderModule(const std::string_view &filePath, VkDevice device,
                                          VkShaderModule *outShaderModule)
{
    // open the file. With cursor at the end
    std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    uint32_t fileSize = static_cast<uint32_t>(file.tellg());

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    auto *fileData =
        reinterpret_cast<char *>(buffer.data()); // We downsize this, but idk, this is how it expects us to use this
    file.read(fileData, fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule = nullptr;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

std::shared_ptr<Hush::LoadedGLTF> Hush::VkOperations::LoadGltf(IRenderer *baseRenderer,
                                                               const std::string_view &filePath)
{
    //> load_1
    LogFormat(ELogLevel::Debug, "Loading GLTF from path {}", filePath);
    auto *engine = dynamic_cast<VulkanRenderer *>(baseRenderer);
    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->SetCreator(engine);
    LoadedGLTF &file = *scene;

    fastgltf::Parser parser{};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = fastgltf::determineGltfFileType(&data);
    // TODO: Refactor this
    if (type == fastgltf::GltfType::glTF)
    {
        auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
        if (load)
        {
            gltf = std::move(load.get());
        }
        else
        {
            LogFormat(ELogLevel::Error, "Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
            return nullptr;
        }
    }
    else if (type == fastgltf::GltfType::GLB)
    {
        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
        if (load)
        {
            gltf = std::move(load.get());
        }
        else
        {
            LogFormat(ELogLevel::Error, "Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
            return nullptr;
        }
    }
    else
    {
        LogError("Failed to determine glTF container");
        return nullptr;
    }
    //< load_1
    //> load_2
    // we can stimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
                                                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                                                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

    file.GetDescriptorPool().Init(engine->GetVulkanDevice(), gltf.materials.size(), sizes);
    //< load_2
    //> load_samplers

    // load samplers
    for (fastgltf::Sampler &sampler : gltf.samplers)
    {

        VkSamplerCreateInfo sampl = {};
        sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampl.pNext = nullptr;
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;

        sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode = ExtractMipMapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler = nullptr;
        vkCreateSampler(engine->GetVulkanDevice(), &sampl, nullptr, &newSampler);

        file.AddSampler(newSampler);
    }
    //< load_samplers
    //> load_arrays
    // temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<INode>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;
    //< load_arrays

    // load all textures
    for (fastgltf::Image &image : gltf.images)
    {
        AllocatedImage img = {};
        bool loadedImage = LoadImageToRender(engine, gltf, image, &img);

        if (loadedImage)
        {
            images.push_back(img);
            file.AddImage(image.name, img);
            continue;
        }
        // we failed to load, so lets give the slot a default white texture to not
        // completely break loading
        // 3 default textures, white, grey, black. 1 pixel each

        uint32_t black = Color::ColorAsInteger(Color::s_black);

        uint32_t magenta = Color::ColorAsInteger(Color::s_magenta);
        std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                // FIXME: Fix casting
                pixels.at(y * 16 + x) = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        auto *rawImageData = static_cast<void *>(pixels.data());
        // TODO: Make this a constructor
        AllocatedImage errorDefaultImage = engine->CreateImage(
            rawImageData, VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
        images.push_back(errorDefaultImage);
        LogFormat(ELogLevel::Error, "Gltf failed to load texture {}", image.name);
    }

    //> load_buffer
    // create buffer to hold the material data
    constexpr uint32_t materialConstantsSize = sizeof(GLTFMetallicRoughness::MaterialConstants);
    VulkanVertexBuffer buffer(materialConstantsSize * gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU, engine->GetVmaAllocator());
    
    auto *materialConstants = static_cast<GLTFMetallicRoughness::MaterialConstants *>(buffer.GetAllocationInfo().pMappedData);

    file.SetMaterialDataBuffer(buffer);
    
    int32_t dataIndex = 0;
    GLTFMetallicRoughness::MaterialConstants *sceneMaterialConstants = materialConstants;
    //< load_buffer
    //
    //> load_material
    for (fastgltf::Material &mat : gltf.materials)
    {
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.AddMaterial(mat.name, newMat);

        GLTFMetallicRoughness::MaterialConstants constants = {};
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metalRoughFactors.x = mat.pbrData.metallicFactor;
        constants.metalRoughFactors.y = mat.pbrData.roughnessFactor;
        // write material parameters to buffer
        sceneMaterialConstants[dataIndex] = constants;

        EMaterialPass passType = EMaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend)
        {
            passType = EMaterialPass::Transparent;
        }

        GLTFMetallicRoughness::MaterialResources materialResources = {};
        //TODO: Get these default images from a static constexpr resource
        // default the material textures
        materialResources.colorImage = engine->GetWhiteImage();
        materialResources.colorSampler = engine->GetDefaultLinearSampler();
        materialResources.metalRoughImage = engine->GetWhiteImage();
        materialResources.metalRoughSampler = engine->GetDefaultLinearSampler();

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.GetMaterialDataBuffer().GetBuffer();
        //TODO: Make sure this points to the constexpr
        materialResources.dataBufferOffset = dataIndex * sizeof(GLTFMetallicRoughness::MaterialConstants);
        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value())
        {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.GetSampler(sampler);
        }
        // build material
        newMat->data = engine->GetMetallicRoughnessMaterial().WriteMaterial(engine->GetVulkanDevice(), passType, materialResources,
                                                                 file.GetDescriptorPool());

        dataIndex++;
    }
    //< load_material

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh &mesh : gltf.meshes)
    {
        std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
        newMesh->name = mesh.name;
        meshes.push_back(newMesh);
        file.AddMesh(mesh.name, newMesh);

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto &&p : mesh.primitives)
        {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initialVtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor &indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                    gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initialVtx); });
            }

            // load vertex positions
            {
                fastgltf::Accessor &posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                    Vertex createdVertex = {};
                    createdVertex.position = v;
                    createdVertex.normal = {1, 0, 0};
                    createdVertex.color = glm::vec4{1.f};
                    createdVertex.uvX = 0;
                    createdVertex.uvY = 0;
                    vertices[initialVtx + index] = createdVertex;
                });
            }

            // load vertex normals
            auto* normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) { vertices[initialVtx + index].normal = v; });
            }

            // load UVs
            auto* uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                                                              [&](glm::vec2 v, size_t index) {
                                                                  vertices[initialVtx + index].uvX = v.x;
                                                                  vertices[initialVtx + index].uvY = v.y;
                                                              });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) { vertices[initialVtx + index].color = v; });
            }

            if (p.materialIndex.has_value())
            {
                newSurface.material = materials[p.materialIndex.value()];
            }
            else
            {
                newSurface.material = materials[0];
            }

            glm::vec3 minpos = vertices[initialVtx].position;
            glm::vec3 maxpos = vertices[initialVtx].position;
            for (int32_t i = initialVtx; i < vertices.size(); i++)
            {
                minpos = glm::min(minpos, vertices[i].position);
                maxpos = glm::max(maxpos, vertices[i].position);
            }

            newSurface.bounds.origin = (maxpos + minpos) / 2.f;
            newSurface.bounds.extents = (maxpos - minpos) / 2.f;
            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
            newMesh->surfaces.push_back(newSurface);
        }
        newMesh->meshBuffers = engine->UploadMesh(indices, vertices);
    }
    //> load_nodes
    // load all nodes and their meshes
    for (fastgltf::Node &node : gltf.nodes)
    {
        std::shared_ptr<INode> newNode;

        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode
        // class
        if (node.meshIndex.has_value())
        {
            newNode = std::make_shared<MeshNode>();
            dynamic_cast<MeshNode *>(newNode.get())->mesh = meshes[*node.meshIndex];
        }
        else
        {
            newNode = std::make_shared<INode>();
        }

        nodes.push_back(newNode);
        //file.nodes[node.name.c_str()]; Idk what this is for(?

        std::visit(fastgltf::visitor{[&](fastgltf::Node::TransformMatrix matrix) {
                                        auto *rawTransformData =
                                             static_cast<void *>(&newNode->GetLocalTransform());                    
                                        memcpy(rawTransformData, matrix.data(), sizeof(matrix));
                                     },
                                     [&](fastgltf::Node::TRS transform) {
                                         glm::vec3 tl(transform.translation[0], transform.translation[1],
                                                      transform.translation[2]);
                                         glm::quat rot(transform.rotation[3], transform.rotation[0],
                                                       transform.rotation[1], transform.rotation[2]);
                                         glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                         glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         glm::mat4 rm = glm::toMat4(rot);
                                         glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         newNode->SetTransform(tm * rm * sm);
                                     }},
        node.transform);
    }
    //< load_nodes
    //> load_graph
    // run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node &node = gltf.nodes[i];
        std::shared_ptr<INode> &sceneNode = nodes[i];

        for (auto &c : node.children)
        {
            sceneNode->AddChild(nodes[c]);
            nodes[c]->SetParent(sceneNode);
        }
    }

    // find the top nodes, with no parents
    for (auto &node : nodes)
    {
        if (node->GetParent().lock() == nullptr)
        {
            file.AddTopNode(node);
            node->RefreshTransform(glm::mat4{1.f});
        }
    }
    return scene;
    //< load_graph
}

bool Hush::VkOperations::LoadImageToRender(IRenderer *baseRenderer, fastgltf::Asset &asset, fastgltf::Image &image, AllocatedImage* outImage)
{
    return false;
}

void Hush::VkOperations::GenerateMipMaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize)
{
    int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
    for (int mip = 0; mip < mipLevels; mip++)
    {

        VkExtent2D halfSize = imageSize;
        halfSize.width /= 2;
        halfSize.height /= 2;

        VkImageMemoryBarrier2 imageBarrier = {};

        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarrier.pNext = nullptr;
        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = VkUtilsFactory::ImageSubResourceRange(aspectMask);
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = mip;
        imageBarrier.image = image;

        VkDependencyInfo depInfo = {};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1)
        {
            VkImageBlit2 blitRegion = {};

            blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
            blitRegion.pNext = nullptr;

            blitRegion.srcOffsets[1].x = imageSize.width;
            blitRegion.srcOffsets[1].y = imageSize.height;
            blitRegion.srcOffsets[1].z = 1;

            blitRegion.dstOffsets[1].x = halfSize.width;
            blitRegion.dstOffsets[1].y = halfSize.height;
            blitRegion.dstOffsets[1].z = 1;

            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcSubresource.mipLevel = mip;

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstSubresource.mipLevel = mip + 1;

            VkBlitImageInfo2 blitInfo = {};
            blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
            blitInfo.pNext = nullptr;
            blitInfo.dstImage = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            imageSize = halfSize;
        }
    }

    // transition all mip levels into the final read_only layout
    TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Hush::VkOperations::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
                                         VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.pNext = nullptr;

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask =
        (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = VkUtilsFactory::ImageSubResourceRange(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}
