/*! \file VkOperations.hpp
    \author Kyn21kx
    \date 2024-06-01
    \brief Utility operations for Vulkan
*/

#pragma once
#include "log/Logger.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/Shared/RenderObject.hpp"
#include "rendering/Vulkan/VulkanRenderer.hpp"
#include <vulkan/vulkan.h>
#include <string_view>
#include <fstream>
#include <vector>
#include <cstdint>

namespace Hush {
    class VkOperations
    {
      public:
        static bool LoadShaderModule(const std::string_view &filePath, VkDevice device, VkShaderModule *outShaderModule)
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
            auto *fileData = reinterpret_cast<char *>(
                buffer.data()); // We downsize this, but idk, this is how it expects us to use this
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

        static std::shared_ptr<LoadedGLTF> LoadGltf(IRenderer *baseRenderer, const std::string_view& filePath) {
        	//> load_1
         	LogFormat(ELogLevel::Debug, "Loading GLTF from path {}", filePath);
          	auto* engine = dynamic_cast<VulkanRenderer*>(baseRenderer);
            std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
            scene->SetCreator(engine);
            LoadedGLTF& file = *scene.get();

            fastgltf::Parser parser {};

            constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
            // fastgltf::Options::LoadExternalImages;

            fastgltf::GltfDataBuffer data;
            data.loadFromFile(filePath);

            fastgltf::Asset gltf;

            std::filesystem::path path = filePath;

            auto type = fastgltf::determineGltfFileType(&data);
            //TODO: Refactor this
            if (type == fastgltf::GltfType::glTF) {
                auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
                if (load) {
                    gltf = std::move(load.get());
                } else {
               		LogFormat(ELogLevel::Error, "Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
                	return nullptr;
                }
            } else if (type == fastgltf::GltfType::GLB) {
                auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
                if (load) {
                    gltf = std::move(load.get());
                } else {
                	LogFormat(ELogLevel::Error, "Failed to load GLTF: {}", fastgltf::to_underlying(load.error()));
                    return nullptr;
                }
            } else {
            	LogError("Failed to determine glTF container");
                return nullptr;
            }
        //< load_1
        //> load_2
            // we can stimate the descriptors we will need accurately
            std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

            file.GetDescriptorPool().Init(engine->GetVulkanDevice(), gltf.materials.size(), sizes);
        //< load_2
        //> load_samplers

            // load samplers
            for (fastgltf::Sampler& sampler : gltf.samplers) {

                VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
                sampl.maxLod = VK_LOD_CLAMP_NONE;
                sampl.minLod = 0;

                sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
                sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

                sampl.mipmapMode= extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

                VkSampler newSampler;
                vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);

                file.samplers.push_back(newSampler);
            }
        //< load_samplers
        //> load_arrays
            // temporal arrays for all the objects to use while creating the GLTF data
            std::vector<std::shared_ptr<MeshAsset>> meshes;
            std::vector<std::shared_ptr<Node>> nodes;
            std::vector<AllocatedImage> images;
            std::vector<std::shared_ptr<GLTFMaterial>> materials;
        //< load_arrays

            // load all textures
            for (fastgltf::Image& image : gltf.images) {
                std::optional<AllocatedImage> img = load_image(engine, gltf, image);

                if (img.has_value()) {
                    images.push_back(*img);
                    file.images[image.name.c_str()] = *img;
                } else {
                    // we failed to load, so lets give the slot a default white texture to not
                    // completely break loading
                    images.push_back(engine->_errorCheckerboardImage);
                    std::cout << "gltf failed to load texture " << image.name << std::endl;
                }
            }

        //> load_buffer
            // create buffer to hold the material data
            file.materialDataBuffer = engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            int data_index = 0;
            GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;
        //< load_buffer
            //
        //> load_material
            for (fastgltf::Material& mat : gltf.materials) {
                std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
                materials.push_back(newMat);
                file.materials[mat.name.c_str()] = newMat;

                GLTFMetallic_Roughness::MaterialConstants constants;
                constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
                constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
                constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
                constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

                constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
                constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
                // write material parameters to buffer
                sceneMaterialConstants[data_index] = constants;

                MaterialPass passType = MaterialPass::MainColor;
                if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
                    passType = MaterialPass::Transparent;
                }

                GLTFMetallic_Roughness::MaterialResources materialResources;
                // default the material textures
                materialResources.colorImage = engine->_whiteImage;
                materialResources.colorSampler = engine->_defaultSamplerLinear;
                materialResources.metalRoughImage = engine->_whiteImage;
                materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

                // set the uniform buffer for the material data
                materialResources.dataBuffer = file.materialDataBuffer.buffer;
                materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
                // grab textures from gltf file
                if (mat.pbrData.baseColorTexture.has_value()) {
                    size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                    size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                    materialResources.colorImage = images[img];
                    materialResources.colorSampler = file.samplers[sampler];
                }
                // build material
                newMat->data = engine->metalRoughMaterial.write_material(engine->_device, passType, materialResources, file.descriptorPool);

                data_index++;
            }
        //< load_material

            // use the same vectors for all meshes so that the memory doesnt reallocate as
            // often
            std::vector<uint32_t> indices;
            std::vector<Vertex> vertices;

            for (fastgltf::Mesh& mesh : gltf.meshes) {
                std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
                meshes.push_back(newmesh);
                file.meshes[mesh.name.c_str()] = newmesh;
                newmesh->name = mesh.name;

                // clear the mesh arrays each mesh, we dont want to merge them by error
                indices.clear();
                vertices.clear();

                for (auto&& p : mesh.primitives) {
                    GeoSurface newSurface;
                    newSurface.startIndex = (uint32_t)indices.size();
                    newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

                    size_t initial_vtx = vertices.size();

                    // load indexes
                    {
                        fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                        indices.reserve(indices.size() + indexaccessor.count);

                        fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                            [&](std::uint32_t idx) {
                                indices.push_back(idx + initial_vtx);
                            });
                    }

                    // load vertex positions
                    {
                        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                        vertices.resize(vertices.size() + posAccessor.count);

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                            [&](glm::vec3 v, size_t index) {
                                Vertex newvtx;
                                newvtx.position = v;
                                newvtx.normal = { 1, 0, 0 };
                                newvtx.color = glm::vec4 { 1.f };
                                newvtx.uv_x = 0;
                                newvtx.uv_y = 0;
                                vertices[initial_vtx + index] = newvtx;
                            });
                    }

                    // load vertex normals
                    auto normals = p.findAttribute("NORMAL");
                    if (normals != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                            [&](glm::vec3 v, size_t index) {
                                vertices[initial_vtx + index].normal = v;
                            });
                    }

                    // load UVs
                    auto uv = p.findAttribute("TEXCOORD_0");
                    if (uv != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                            [&](glm::vec2 v, size_t index) {
                                vertices[initial_vtx + index].uv_x = v.x;
                                vertices[initial_vtx + index].uv_y = v.y;
                            });
                    }

                    // load vertex colors
                    auto colors = p.findAttribute("COLOR_0");
                    if (colors != p.attributes.end()) {

                        fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                            [&](glm::vec4 v, size_t index) {
                                vertices[initial_vtx + index].color = v;
                            });
                    }

                    if (p.materialIndex.has_value()) {
                        newSurface.material = materials[p.materialIndex.value()];
                    } else {
                        newSurface.material = materials[0];
                    }

                    glm::vec3 minpos = vertices[initial_vtx].position;
                    glm::vec3 maxpos = vertices[initial_vtx].position;
                    for (int i = initial_vtx; i < vertices.size(); i++) {
                        minpos = glm::min(minpos, vertices[i].position);
                        maxpos = glm::max(maxpos, vertices[i].position);
                    }

                    newSurface.bounds.origin = (maxpos + minpos) / 2.f;
                    newSurface.bounds.extents = (maxpos - minpos) / 2.f;
                    newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
                    newmesh->surfaces.push_back(newSurface);
                }

                newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
            }
        //> load_nodes
            // load all nodes and their meshes
            for (fastgltf::Node& node : gltf.nodes) {
                std::shared_ptr<Node> newNode;

                // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
                if (node.meshIndex.has_value()) {
                    newNode = std::make_shared<MeshNode>();
                    static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
                } else {
                    newNode = std::make_shared<Node>();
                }

                nodes.push_back(newNode);
                file.nodes[node.name.c_str()];

                std::visit(fastgltf::visitor { [&](fastgltf::Node::TransformMatrix matrix) {
                                                  memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                                              },
                               [&](fastgltf::Node::TRS transform) {
                                   glm::vec3 tl(transform.translation[0], transform.translation[1],
                                       transform.translation[2]);
                                   glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
                                       transform.rotation[2]);
                                   glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                   glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                   glm::mat4 rm = glm::toMat4(rot);
                                   glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                   newNode->localTransform = tm * rm * sm;
                               } },
                    node.transform);
            }
        //< load_nodes
        //> load_graph
            // run loop again to setup transform hierarchy
            for (int i = 0; i < gltf.nodes.size(); i++) {
                fastgltf::Node& node = gltf.nodes[i];
                std::shared_ptr<Node>& sceneNode = nodes[i];

                for (auto& c : node.children) {
                    sceneNode->children.push_back(nodes[c]);
                    nodes[c]->parent = sceneNode;
                }
            }

            // find the top nodes, with no parents
            for (auto& node : nodes) {
                if (node->parent.lock() == nullptr) {
                    file.topNodes.push_back(node);
                    node->refreshTransform(glm::mat4 { 1.f });
                }
            }
            return scene;
        //< load_graph
        }
    };
}
