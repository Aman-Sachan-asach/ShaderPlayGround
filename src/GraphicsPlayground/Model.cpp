#include "model.h"

/*
#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/tiny_obj_loader.h"

Model::Model(VulkanDevice* device, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
	: device(device), vertices(vertices), indices(indices)
{
	if (this->vertices.size() > 0)
	{
		// Create Vertex Buffer 
		VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
		BufferUtils::CreateBufferFromData(device, commandPool, this->vertices.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
	}

	if (this->indices.size() > 0)
	{
		// Create Index Buffer
		VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
		BufferUtils::CreateBufferFromData(device, commandPool, this->indices.data(), indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
	}

	modelBufferObject.modelMatrix = glm::mat4(1.0f);
	BufferUtils::CreateBuffer(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ModelBufferObject),
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		modelBuffer, modelBufferMemory);

	vkMapMemory(device->GetVkDevice(), modelBufferMemory, 0, sizeof(ModelBufferObject), 0, &mappedData);
	memcpy(mappedData, &modelBufferObject, sizeof(ModelBufferObject));
}

Model::Model(VulkanDevice* device, VkCommandPool commandPool, const std::string model_path, const std::string texture_path)
	: device(device)
{
	LoadModel(model_path);

	if (vertices.size() > 0)
	{
		// Create Vertex Buffer 
		VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
		BufferUtils::CreateBufferFromData(device, commandPool, this->vertices.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
	}

	if (indices.size() > 0)
	{
		// Create Index Buffer
		VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
		BufferUtils::CreateBufferFromData(device, commandPool, this->indices.data(), indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
	}

	// Create Model Buffer
	modelBufferObject.modelMatrix = glm::mat4(1.0f);
	BufferUtils::CreateBuffer(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ModelBufferObject),
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							modelBuffer, modelBufferMemory);

	vkMapMemory(device->GetVkDevice(), modelBufferMemory, 0, sizeof(ModelBufferObject), 0, &mappedData);
	memcpy(mappedData, &modelBufferObject, sizeof(ModelBufferObject));

	SetTexture(device, commandPool, texture_path);
}

Model::~Model()
{
	if (indices.size() > 0) 
	{
		vkDestroyBuffer(device->GetVkDevice(), indexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), indexBufferMemory, nullptr);
	}

	if (vertices.size() > 0) 
	{
		vkDestroyBuffer(device->GetVkDevice(), vertexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), vertexBufferMemory, nullptr);
	}

	vkUnmapMemory(device->GetVkDevice(), modelBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), modelBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), modelBufferMemory, nullptr);

	if (texture != VK_NULL_HANDLE) {
		vkDestroyImage(device->GetVkDevice(), texture, nullptr);
	}

	if (textureMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device->GetVkDevice(), textureMemory, nullptr);
	}

	if (textureView != VK_NULL_HANDLE) {
		vkDestroyImageView(device->GetVkDevice(), textureView, nullptr);
	}

	if (textureSampler != VK_NULL_HANDLE) {
		vkDestroySampler(device->GetVkDevice(), textureSampler, nullptr);
	}
}

void Model::SetTexture(VulkanDevice* device, VkCommandPool commandPool, const std::string texture_path)
{
	ImageLoadingUtility::loadImageFromFile(device, commandPool, texture_path.c_str(), texture, textureMemory,
											VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
											VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Image::createImageView(device, textureView, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	Image::createSampler(device, textureSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f);
}

void Model::LoadModel(const std::string model_path)
{
	// The attrib container holds all of the positions, normals and texture coordinates 
	// in its attrib.vertices, attrib.normals and attrib.texcoords vectors.
	tinyobj::attrib_t attrib;
	
	// The shapes container contains all of the separate objects and their faces.
	std::vector<tinyobj::shape_t> shapes;
	
	// OBJ models can define a material and texture per face --> ignored for now
	std::vector<tinyobj::material_t> materials;
	
	std::string err;

	//Faces in OBJ files can actually contain an arbitrary number of vertices, whereas our application can only render triangles. 
	//Luckily the LoadObj has an optional parameter to automatically triangulate such faces, which is enabled by default.
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model_path.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	for (const auto& shape : shapes) 
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.position = glm::vec4(attrib.vertices[3 * index.vertex_index + 0],
										attrib.vertices[3 * index.vertex_index + 1],
										attrib.vertices[3 * index.vertex_index + 2], 
										1.0f);
			
			vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

			//The origin of texture coordinates in Vulkan is the top-left corner, whereas 
			//the OBJ format assumes the bottom-left corner. 
			//Solve this by flipping the vertical component of the texture coordinates
			vertex.texCoord = glm::vec2(attrib.texcoords[2 * index.texcoord_index + 0],
										1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);

			//prevents vertex duplication
			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

const std::vector<Vertex>& Model::getVertices() const
{
	return vertices;
}
const std::vector<uint32_t>& Model::getIndices() const
{
	return indices;
}

VkBuffer Model::getVertexBuffer() const
{
	return vertexBuffer;
}
VkBuffer Model::getIndexBuffer() const
{
	return indexBuffer;
}

uint32_t Model::getVertexBufferSize() const
{
	return static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
}
uint32_t Model::getIndexBufferSize() const
{
	return static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
}

const ModelBufferObject& Model::getModelBufferObject() const
{
	return modelBufferObject;
}

glm::mat4 Model::GetModelMatrix() const
{
	return modelBufferObject.modelMatrix;
}
VkBuffer Model::GetModelBuffer() const
{
	return modelBuffer;
}
void Model::SetModelBuffer(glm::mat4 &model)
{
	modelBufferObject.modelMatrix = model;
	memcpy(mappedData, &modelBufferObject, sizeof(ModelBufferObject));
}

VkImage Model::GetTexture() const
{
	return texture;
}
VkDeviceMemory Model::GetTextureMemory() const
{
	return textureMemory;
}
VkImageView Model::GetTextureView() const
{
	return textureView;
}
VkSampler Model::GetTextureSampler() const
{
	return textureSampler;
}
*/