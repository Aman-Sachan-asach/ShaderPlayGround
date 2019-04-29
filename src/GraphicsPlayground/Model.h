#pragma once

#include <unordered_map>
#include <global.h>
#include <Utilities/bufferUtility.h>
#include <Utilities/imageUtility.h>

#include "vulkanDevices.h"

struct ModelUBO
{
	glm::mat4 modelMat;
};

class Model
{
public:
	Model() = delete;
	Model(VulkanDevices* devices, VkCommandPool& commandPool, unsigned int numSwapChainImages, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);
	Model(VulkanDevices* devices, VkCommandPool& commandPool, unsigned int numSwapChainImages, const std::string model_path, const std::string texture_path);
	~Model();

	void updateUniformBuffer(uint32_t currentImageIndex);

	//Getters
	const std::vector<Vertex>& getVertices() const;
	VkBuffer getVertexBuffer();
	uint32_t getVertexBufferSize() const;

	const std::vector<uint32_t>& getIndices() const;
	const uint32_t Model::getNumIndices() const;
	VkBuffer getIndexBuffer();
	uint32_t getIndexBufferSize() const;

private:
	VulkanDevices* m_devices;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	unsigned int m_numSwapChainImages;
	
	std::vector<Vertex> m_vertices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkDeviceSize m_vertexBufferSize;
	void* m_mappedDataVertexBuffer;

	std::vector<uint32_t> m_indices;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	VkDeviceSize m_indexBufferSize;
	void* m_mappedDataIndexBuffer;

	// Multiple buffers for UBO because multiple frames may be in flight at the same time and this is data that could potentially be updated every frame
	// This is also why it wouldnt make sense to use the staging buffer, the overhead of that may lead to worse performance
	std::vector<ModelUBO> m_modelUBOs;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBufferMemories;
	VkDeviceSize m_uniformBufferSize;
	std::vector<void*> m_mappedDataUniformBuffers;
};