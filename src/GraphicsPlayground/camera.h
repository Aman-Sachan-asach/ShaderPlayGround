#pragma once

#include <global.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "VulkanDevices.h"
#include <Utilities/bufferUtility.h>

#define PI 3.14159

struct CameraUBO {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 eyePos; //pad vec3s with extra float to make them vec4s so vulkan can do offsets correctly
	glm::vec2 tanFovBy2; //vec2 and vec4 are acceptable for offseting; 
	//stored as .x = horizontalFovBy2 and .y = verticalFovBy2
};

class Camera 
{
public:
	Camera() = delete;	// https://stackoverflow.com/questions/5513881/meaning-of-delete-after-function-declaration
	Camera(VulkanDevices* devices, unsigned int numSwapChainImages,
		glm::vec3 eyePos, glm::vec3 lookAtPoint, int width, int height,
		float foV_vertical, float aspectRatio, float nearClip, float farClip);
	~Camera();

	VkBuffer getUniformBuffer(unsigned int bufferIndex) const;
	uint32_t getUniformBufferSize() const;
	void updateUniformBuffer(unsigned int bufferIndex);
	void updateUniformBuffer(Camera* cam, unsigned int dstCamBufferIndex, unsigned int srcCamBufferIndex);
	void copyToGPUMemory(unsigned int bufferIndex);

	glm::mat4 getView() const;
	glm::mat4 getProj() const;
	glm::mat4 getViewProj() const;
	void recomputeAttributes();

	void rotateAboutUp(float deg);
	void rotateAboutRight(float deg);

	void translateAlongLook(float amt);
	void translateAlongRight(float amt);
	void translateAlongUp(float amt);

private:
	VulkanDevices* m_devices; //member variable because it is needed for the destructor
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	unsigned int m_numSwapChainImages;

	std::vector<CameraUBO> m_cameraUBOs;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBufferMemories;
	VkDeviceSize m_uniformBufferSize;
	std::vector<void*> m_mappedDataUniformBuffers;

	glm::vec3 m_eyePos;
	glm::vec3 m_ref;      //The point in world space towards which the camera is pointing
			  
	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_up;
	glm::vec3 m_worldUp;

	int m_width, m_height;

	float m_fovy;
	float m_aspect;
	float m_near_clip;  // Near clip plane distance
	float m_far_clip;  // Far clip plane distance
};