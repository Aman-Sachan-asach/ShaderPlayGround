#pragma once

#include <Vulkan\vulkanManager.h>
#include <Vulkan\Utilities\vBufferUtil.h>
#include <Vulkan\Utilities\vImageUtil.h>

class Texture
{
public:
	Texture() = delete;
	Texture(std::shared_ptr<VulkanManager> vulkanObj, VkQueue& queue, VkCommandPool& cmdPool, VkFormat format);
	~Texture();

	void create2DTexture(
		std::string texturePath, 
		bool isMipMapped = false, 
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, 
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	void createEmpty2DTexture(
		uint32_t width, uint32_t height, uint32_t depth,
		bool isMipMapped = false, 
		VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	void create3DTexture(
		std::string texturePath, 
		bool isMipMapped = false, 
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, 
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	void create3DTextureFromMany2DTextures(
		VkDevice logicalDevice, VkCommandPool commandPool, 
		int num2DImages, int numChannels,
		const std::string folder_path, const std::string textureBaseName, const std::string fileExtension );
	
	uint32_t getWidth() const;
	uint32_t getHeight() const;
	uint32_t getDepth() const;
	VkFormat getFormat() const;
	VkImageLayout getImageLayout() const;

	VkImage& getImage();
	VkDeviceMemory& getImageMemory();
	VkImageView& getImageView();
	VkSampler& getSampler();

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;

	// We keep handles to the graphicsQueue and commandPool because we create and submit commands outside of the constructor
	// These commands are for things like image transitions, copying buffers into VkImage's, etc
	VkQueue m_graphicsQueue;
	VkCommandPool m_cmdPool;

	uint32_t m_width, m_height, m_depth;
	VkFormat m_format;
	VkImageLayout m_imageLayout;
	uint32_t m_mipLevels; // number of levels in the mip chain

	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
};