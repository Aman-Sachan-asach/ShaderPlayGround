#pragma once
#include <global.h>
#include <Vulkan/Utilities/vPipelineUtil.h>
#include <Vulkan/Utilities/vBufferUtil.h>
#include <Vulkan/Utilities/vDescriptorUtil.h>
#include <Vulkan/Utilities/vShaderUtil.h>
#include <Vulkan/Utilities/vRenderUtil.h>
#include <Vulkan/RendererBackend/vAccelerationStructure.h>
#include <Vulkan/Utilities/vAccelerationStructureUtil.h>
#include <Utilities/generalUtility.h>
#include <Vulkan/vulkanManager.h>

#include "Camera.h"
#include "Scene.h"
#include "SceneElements/texture.h"


struct DescriptorSetLayouts
{
	std::vector<VkDescriptorSetLayout> computeDSL;
	std::vector<VkDescriptorSetLayout> rasterDSL;
	std::vector<VkDescriptorSetLayout> raytraceDSL;
};

struct ComputePipelineLayouts
{
	VkPipelineLayout sky;
	VkPipelineLayout clouds;
	VkPipelineLayout grass;	
	VkPipelineLayout water;
};


// This class will manage the pipelines created and used for rendering
// It should help abstract away that detail from the renderer class and prevent the renderpass stuff from being coupled with other things
class VulkanRendererBackend
{
public:
	VulkanRendererBackend() = delete;
	VulkanRendererBackend(std::shared_ptr<VulkanManager> vulkanManager,
		RendererOptions& rendererOptions, int numSwapChainImages, VkExtent2D windowExtents);
	~VulkanRendererBackend();
	void cleanup();

	void createPipelines(DescriptorSetLayouts& pipelineDescriptorSetLayouts);
	void createRenderPassesAndFrameResources();
	void createAllPostProcessEffects(std::shared_ptr<Scene> scene);
	
	// Update Descriptors and Resources
	void update(uint32_t currentImageIndex);

	// Descriptor Sets
	void expandDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets();

	// Synchronization Objects
	void createSyncObjects();

	// Command Buffers
	void recreateCommandBuffers();
	void submitCommandBuffers();
	void recordAllCommandBuffers(std::shared_ptr<Camera> m_camera, std::shared_ptr<Scene> m_scene);
	

	// Getters
	VkDescriptorSet getDescriptorSet(DSL_TYPE type, int frameIndex, int postProcessIndex = 0);
	VkDescriptorSetLayout getDescriptorSetLayout(DSL_TYPE type, int postProcessIndex = 0);
	const VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
	VkSemaphore getpostProcessFinishedVkSemaphore(uint32_t index) const { return m_postProcessFinishedSemaphores[index]; }
	const VkCommandPool getComputeCommandPool() const { return m_computeCmdPool; }
	const VkCommandPool getGraphicsCommandPool() const { return m_graphicsCmdPool; }

	// Setters
	void setWindowExtents(VkExtent2D windowExtent) { m_windowExtents = windowExtent; }

private:
	void cleanupPipelines();
	void cleanupRenderPassesAndFrameResources();
	void cleanupPostProcess();

	// Render Passes
	void createRenderPasses(const VkImageLayout& beforeRenderPassExecuted, const VkImageLayout& afterRenderPassExecuted);
	// Frame Buffer Attachments -- Used in conjunction with RenderPasses but not needed for their creation
	void createDepthResources();
	void createFrameBuffers(
		const VkImageLayout& layoutBeforeImageCreation,
		const VkImageLayout& layoutToTransitionImageToAfterCreation,
		const VkImageLayout& layoutAfterRenderPassExecuted );
	
	// Pipelines
	void createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &pathToShader);
	void createRayTracePipeline(std::vector<VkDescriptorSetLayout>& rayTraceDSL);
	void createRasterizationRenderPipeline(std::vector<VkDescriptorSetLayout>& rasterizationDSL);

	// Command Buffers
	void createCommandPoolsAndBuffers();
	void recordCommandBuffer_rayTracingCmds(
		unsigned int frameIndex, VkCommandBuffer& rayTracingCmdBuffer, std::shared_ptr<Camera> m_camera, std::shared_ptr<Scene> m_scene);
	void recordCommandBuffer_ComputeCmds(
		unsigned int frameIndex, VkCommandBuffer& ComputeCmdBuffer, std::shared_ptr<Scene> scene);
	void recordCommandBuffer_GraphicsCmds(
		unsigned int frameIndex, VkCommandBuffer& graphicsCmdBuffer, std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera,
		VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* clearValues);
	void recordCommandBuffer_PostProcessCmds(
		unsigned int frameIndex, VkCommandBuffer& postProcessCmdBuffer, std::shared_ptr<Scene> scene,
		VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* clearValues);
	void recordCommandBuffer_FinalCmds(unsigned int frameIndex, VkCommandBuffer& cmdBuffer);


	// Ray Tracing
public:
	void getRayTracingFunctionPointers();
	void cleanupRayTracing();
	void destroyRayTracing();

	void createDescriptors_rayTracing(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets_rayTracing(std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene);

	void createStorageImages();
	void createAndBuildAccelerationStructures(std::shared_ptr<Scene> scene);

	void createAllBottomLevelAccelerationStructures(std::shared_ptr<Scene> scene);
	void createGeometryInstancesForTLAS(std::shared_ptr<Scene> scene);
	void createTopLevelAccelerationStructure(bool allowUpdate);
	void buildAccelerationStructures(std::shared_ptr<Scene> scene, std::vector<GeometryInstance>& geometryInstances);
	
	void createShaderBindingTable();
	void mapShaderBindingTable();

	// Helpers
	VkDeviceSize getScratchBufferSize(std::shared_ptr<Scene> scene);
	VkDeviceSize copyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex);
	
private:

	// Post Process
	void expandDescriptorPool_PostProcess(std::vector<VkDescriptorPoolSize>& poolSizes);
	void createDescriptors_PostProcess_Common(VkDescriptorPool descriptorPool);
	void createDescriptors_PostProcess_Specific(VkDescriptorPool descriptorPool);
	void writeToAndUpdateDescriptorSets_PostProcess_Common();
	void writeToAndUpdateDescriptorSets_PostProcess_Specific();

	void prePostProcess();
	void addPostProcessPass(std::string effectName, std::vector<VkDescriptorSetLayout>& effectDSL, 
		POST_PROCESS_TYPE postType,	PostProcessRPI& postRPI);

	// A image that is rendered to in one pass will be read from in the next pass. For this reason we treat the images as storage images,
	// which helps us avoid constantly transitioning the images from a color attachment optimal state to a read only optimal state.
	// Load and store operations on storage images can only be done on images in VK_IMAGE_LAYOUT_GENERAL layout.
	void addRenderPass_PostProcess(VkRenderPass& l_renderPass, const VkFormat colorFormat, const VkFormat depthFormat,
		const VkImageLayout initialLayout, const VkImageLayout finalLayout);
	void addFrameBuffers_PostProcess(PostProcessRPI& passRPI, VkFormat colorFormat, POST_PROCESS_TYPE postType,
		std::vector<FrameBufferAttachment>& fbAttachments, const VkImageLayout afterRenderPassExecuted);
	void addPipeline_PostProcess(const std::string &shaderName, std::vector<VkDescriptorSetLayout>& l_postProcessDSL,
		VkRenderPass& l_renderPass, const uint32_t subpass = 0, VkExtent2D extents = { 0,0 });

	inline DSL_TYPE chooseHighResInput();
	inline DSL_TYPE chooseLowResInput();

private:
	RendererOptions m_rendererOptions;
	std::shared_ptr<VulkanManager> m_vulkanManager;
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	uint32_t m_numSwapChainImages;
	VkExtent2D m_windowExtents;

	VkFormat m_highResolutionRenderFormat;
	VkFormat m_lowResolutionRenderFormat;
	VkFormat m_depthFormat;

	VkDescriptorPool m_descriptorPool;
public:
	// --- Descriptor Sets ---
	VkDescriptorSetLayout m_DSL_rayTrace;
	std::vector<VkDescriptorSet> m_DS_rayTrace;
private:
	// --- Render Passes --- 
	// RPI stands for Render Pass Info
	// RenderPasses render to their own framebuffers unless otherwise specified
	RenderPassInfo m_rasterRPI; // Typical Forward render pass

	// --- Pipelines ---
	// Pipelines -- P
	VkPipeline m_rayTrace_P;
	VkPipeline m_rasterization_P;
	VkPipeline m_compute_P;
	// Pipeline Layouts -- PLs
	VkPipelineLayout m_rayTrace_PL;
	VkPipelineLayout m_rasterization_PL;
	VkPipelineLayout m_compute_PL;	
		
	// --- Frame Buffer Attachments --- 
	// Depth is going to be common to the scene across render passes as well
	FrameBufferAttachment m_depth;
	std::array<std::vector<FrameBufferAttachment>, 2> m_fbaHighRes;
	std::array<std::vector<FrameBufferAttachment>, 2> m_fbaLowRes;
	unsigned int m_fbaHighResIndexInUse;
	unsigned int m_fbaLowResIndexInUse;

	// --- PostProcess ---
	// This set can then be referenced by the UI pass easily.
	std::vector<VkDescriptorImageInfo> m_prePostProcessInput; // result of render passes that occur before post process work.
	PostProcessPushConstants shaderConstants;

	unsigned int m_numPostEffects;
	VkSampler m_postProcessSampler;
	std::vector<std::string> m_postEffectNames;
	std::vector<VkPipeline> m_postProcess_Ps;
	std::vector<VkPipelineLayout> m_postProcess_PLs;
	std::vector<PostProcessRPI> m_postProcessRPIs;
	std::vector<PostProcessDescriptors> m_postProcessDescriptorsSpecific;
	std::vector<PostProcessDescriptors> m_postProcessDescriptorsCommon;
	

	// --- Command Buffers and Memory Pools --- 
	// Need a command pool for every type of queue you use	
	VkCommandPool m_computeCmdPool;
	std::vector<VkCommandBuffer> m_computeCommandBuffers;
	VkCommandPool m_graphicsCmdPool;
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	std::vector<VkCommandBuffer> m_rayTracingCommandBuffers;	
	std::vector<VkCommandBuffer> m_postProcessCommandBuffers;

	// Synchronization
	std::vector<VkSemaphore> m_renderOperationsFinishedSemaphores;
	std::vector<VkSemaphore> m_computeOperationsFinishedSemaphores;
	std::vector<VkSemaphore> m_postProcessFinishedSemaphores;

	// --- Queues --- 
	VkQueue m_graphicsQueue;
	VkQueue m_computeQueue;

	// --- Ray Tracing ---
public:
	PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
	PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
	PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
	PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
private:
	PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
	PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;

	VkPhysicalDeviceRayTracingPropertiesNV m_rayTracingProperties{};
	std::vector<std::shared_ptr<Texture2D>> m_rayTracedImages;
	
	vTLAS m_topLevelAS;
	uint32_t m_sbtSize;
	mageVKBuffer m_shaderBindingTable;
};

#pragma once
#include <Vulkan/RendererBackend/vRendererBackend_Commands.inl>
#include <Vulkan/RendererBackend/vRendererBackend_Pipelines.inl>
#include <Vulkan/RendererBackend/vRendererBackend_RenderPasses.inl>
#include <Vulkan/RendererBackend/vRendererBackend_PostProcess.inl>
#include <Vulkan/RendererBackend/vRendererBackend_Rasterization.inl>
#include <Vulkan/RendererBackend/vRendererBackend_RayTracing.inl>