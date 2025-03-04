#pragma once

#include <cstdint>
#include <VkBootstrap.h>
#include <vector>

namespace Hush
{
	class VulkanRenderer;

	class VulkanSwapchain
	{

	public:
		void Recreate(uint32_t width, uint32_t height, VulkanRenderer *renderer);

		void Resize(uint32_t width, uint32_t height);

		void Destroy();

		void Present(VkCommandBuffer cmd, uint32_t *swapchainImageIndex, bool *shouldResize);

		uint32_t AcquireNextImage(VkSemaphore swapchainSemaphore, bool *shouldResize);

		[[nodiscard]]
		const VkFormat &GetImageFormat() const noexcept;

		[[nodiscard]]
		const VkExtent2D &GetExtent() const noexcept;

		[[nodiscard]]
		const std::vector<VkImage> &GetImages() const noexcept;

		[[nodiscard]]
		const std::vector<VkImageView> &GetImageViews() const noexcept;

		VkSwapchainKHR GetRawSwapchain() noexcept;

	private:
		vkb::Swapchain BuildVkbSwapchain(uint32_t width, uint32_t height, VulkanRenderer *renderer);

		VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;

		VkExtent2D m_swapChainExtent{};

		VkSwapchainKHR m_swapchain = nullptr;

		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;

		// Weak ptr to the renderer that "owns" this swap chain
		VulkanRenderer *m_renderer;
	};

} // namespace Hush
