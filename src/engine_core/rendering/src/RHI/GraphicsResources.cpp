/*! \file GraphicsResources.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-18
	\brief Graphics resource type implementations
*/

#include "IGraphicsDevice.hpp"
#include "GraphicsResources.hpp"

namespace Hush::Graphics
{
	void TextureResource::CreateResource(const TextureDescriptor &textureDescriptor, IGraphicsDevice *device)
	{
		this->descriptor = textureDescriptor;
		this->texture = device->CreateTexture(descriptor);
	}

	void TextureResource::DestroyResource([[maybe_unused]] const TextureDescriptor &textureDescriptor,
										  [[maybe_unused]] IGraphicsDevice *device)
	{
		this->texture.reset();
	}

	void BufferResource::CreateResource(const BufferDescriptor &bufferDescriptor, IGraphicsDevice *device)
	{
		this->descriptor = bufferDescriptor;
		this->buffer = device->CreateBuffer(descriptor);
	}

	void BufferResource::DestroyResource([[maybe_unused]] const BufferDescriptor &bufferDescriptor,
										 [[maybe_unused]] IGraphicsDevice *device)
	{
		this->buffer.reset();
	}

	void ShaderResource::CreateResource(const ShaderModuleDescriptor &shaderDescriptor, IGraphicsDevice *device)
	{
		this->descriptor = shaderDescriptor;
		this->shaderModule = device->CreateShaderModule(descriptor);
	}

	void ShaderResource::DestroyResource([[maybe_unused]] const ShaderModuleDescriptor &shaderDescriptor,
										 [[maybe_unused]] IGraphicsDevice *device)
	{
		this->shaderModule.reset();
	}

	void BindGroupLayoutResource::CreateResource(const BindGroupLayoutDescriptor &layoutDescriptor,
												 IGraphicsDevice *device)
	{
		this->descriptor = layoutDescriptor;
		this->layout = device->CreateBindGroupLayout(descriptor);
	}

	void BindGroupLayoutResource::DestroyResource([[maybe_unused]] const BindGroupLayoutDescriptor &layoutDescriptor,
												  [[maybe_unused]] IGraphicsDevice *device)
	{
		this->layout.reset();
	}

	void BindGroupResource::CreateResource(const BindGroupDescriptor &bindGroupDescriptor, IGraphicsDevice *device)
	{
		this->descriptor = bindGroupDescriptor;
		this->bindGroup = device->CreateBindGroup(descriptor);
	}

	void BindGroupResource::DestroyResource([[maybe_unused]] const BindGroupDescriptor &bindGroupDescriptor,
											[[maybe_unused]] IGraphicsDevice *device)
	{
		this->bindGroup.reset();
	}

	void GraphicsPipelineResource::CreateResource(const GraphicsPipelineDescriptor &pipelineDescriptor,
												  IGraphicsDevice *device)
	{
		this->descriptor = pipelineDescriptor;
		this->pipeline = device->CreateGraphicsPipeline(descriptor);
	}

	void GraphicsPipelineResource::DestroyResource(
		[[maybe_unused]] const GraphicsPipelineDescriptor &pipelineDescriptor, [[maybe_unused]] IGraphicsDevice *device)
	{
		this->pipeline.reset();
	}

	void SamplerResource::CreateResource(const SamplerDescriptor &samplerDescriptor, IGraphicsDevice *device)
	{
		this->descriptor = samplerDescriptor;
		this->sampler = device->CreateSampler(descriptor);
	}

	void SamplerResource::DestroyResource([[maybe_unused]] const SamplerDescriptor &samplerDescriptor,
										  [[maybe_unused]] IGraphicsDevice *device)
	{
		this->sampler.reset();
	}
} // namespace Hush::Graphics
