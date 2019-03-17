#pragma once
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <set>
#include <bitset>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

enum QueueFlags
{
	Graphics,
	Compute,
	Transfer,
	Present,
};

enum QueueFlagBit
{
	GraphicsBit = 1 << 0,
	ComputeBit = 1 << 1,
	TransferBit = 1 << 2, 	// TransferBit tells Vulkan that we can transfer data between CPU and GPU
	PresentBit = 1 << 3,
};

using QueueFlagBits = std::bitset<sizeof(QueueFlags)>;
using QueueFamilyIndices = std::array<int, sizeof(QueueFlags)>;
using Queues = std::array<VkQueue, sizeof(QueueFlags)>;