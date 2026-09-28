#include "Texture.h"

#include "Lite/Core/Logger.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Vulkan/VulkanUtils.h"

#include <cstring>
#include <objbase.h>
#include <vector>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace {

	void Release(IUnknown* object)
	{
		if (object)
			object->Release();
	}

	bool DecodeImage(const std::filesystem::path& path, uint32_t& width, uint32_t& height, std::vector<uint8_t>& pixels)
	{
		HRESULT startup = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		bool uninitialize = SUCCEEDED(startup);

		IWICImagingFactory* factory = nullptr;
		IWICBitmapDecoder* decoder = nullptr;
		IWICBitmapFrameDecode* frame = nullptr;
		IWICFormatConverter* converter = nullptr;
		bool decoded = false;

		HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
		if (SUCCEEDED(result))
			result = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
		if (SUCCEEDED(result))
			result = decoder->GetFrame(0, &frame);
		if (SUCCEEDED(result))
			result = factory->CreateFormatConverter(&converter);
		if (SUCCEEDED(result))
			result = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

		UINT imageWidth = 0;
		UINT imageHeight = 0;
		if (SUCCEEDED(result))
			result = converter->GetSize(&imageWidth, &imageHeight);
		if (SUCCEEDED(result) && imageWidth > 0 && imageHeight > 0)
		{
			width = imageWidth;
			height = imageHeight;
			pixels.resize(static_cast<size_t>(width) * height * 4u);
			result = converter->CopyPixels(nullptr, width * 4u, static_cast<UINT>(pixels.size()), pixels.data());
			decoded = SUCCEEDED(result);
		}

		Release(converter);
		Release(frame);
		Release(decoder);
		Release(factory);
		if (uninitialize)
			CoUninitialize();

		if (!decoded)
			LITE_ERROR("Failed to decode texture {}", path.string());

		return decoded;
	}

	bool UploadImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, const std::vector<uint8_t>& pixels, VkImage& image, VkDeviceMemory& memory)
	{
		VkBuffer staging = VK_NULL_HANDLE;
		VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
		VkDeviceSize size = pixels.size();

		VkBufferCreateInfo bufferInfo {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (!Lite::CheckVk(vkCreateBuffer(device, &bufferInfo, nullptr, &staging), "create texture staging buffer"))
			return false;

		VkMemoryRequirements stagingRequirements {};
		vkGetBufferMemoryRequirements(device, staging, &stagingRequirements);
		uint32_t stagingType = Lite::FindMemoryType(physicalDevice, stagingRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkMemoryAllocateInfo stagingAllocate {};
		stagingAllocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		stagingAllocate.allocationSize = stagingRequirements.size;
		stagingAllocate.memoryTypeIndex = stagingType;
		bool ready = stagingType != UINT32_MAX
			&& Lite::CheckVk(vkAllocateMemory(device, &stagingAllocate, nullptr, &stagingMemory), "allocate texture staging memory")
			&& Lite::CheckVk(vkBindBufferMemory(device, staging, stagingMemory, 0), "bind texture staging memory");

		if (ready)
		{
			void* mapped = nullptr;
			vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
			std::memcpy(mapped, pixels.data(), static_cast<size_t>(size));
			vkUnmapMemory(device, stagingMemory);
		}

		VkImageCreateInfo imageInfo {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.extent = { width, height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ready = ready && Lite::CheckVk(vkCreateImage(device, &imageInfo, nullptr, &image), "create texture image");

		VkMemoryRequirements imageRequirements {};
		if (ready)
			vkGetImageMemoryRequirements(device, image, &imageRequirements);
		uint32_t imageType = ready ? Lite::FindMemoryType(physicalDevice, imageRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) : UINT32_MAX;

		VkMemoryAllocateInfo imageAllocate {};
		imageAllocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		imageAllocate.allocationSize = imageRequirements.size;
		imageAllocate.memoryTypeIndex = imageType;
		ready = ready && imageType != UINT32_MAX
			&& Lite::CheckVk(vkAllocateMemory(device, &imageAllocate, nullptr, &memory), "allocate texture memory")
			&& Lite::CheckVk(vkBindImageMemory(device, image, memory, 0), "bind texture memory");

		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		poolInfo.queueFamilyIndex = Lite::Renderer::GetGraphicsQueueFamily();
		ready = ready && Lite::CheckVk(vkCreateCommandPool(device, &poolInfo, nullptr, &pool), "create texture command pool");

		VkCommandBuffer command = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = pool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		ready = ready && Lite::CheckVk(vkAllocateCommandBuffers(device, &allocateInfo, &command), "allocate texture command");

		if (ready)
		{
			VkCommandBufferBeginInfo begin {};
			begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(command, &begin);

			VkImageMemoryBarrier toTransfer {};
			toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toTransfer.image = image;
			toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toTransfer.subresourceRange.levelCount = 1;
			toTransfer.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

			VkBufferImageCopy region {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = { width, height, 1 };
			vkCmdCopyBufferToImage(command, staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			VkImageMemoryBarrier toSample {};
			toSample.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			toSample.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			toSample.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			toSample.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			toSample.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			toSample.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toSample.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toSample.image = image;
			toSample.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toSample.subresourceRange.levelCount = 1;
			toSample.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toSample);

			vkEndCommandBuffer(command);

			VkSubmitInfo submit {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &command;
			ready = Lite::CheckVk(vkQueueSubmit(Lite::Renderer::GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE), "submit texture upload")
				&& Lite::CheckVk(vkQueueWaitIdle(Lite::Renderer::GetGraphicsQueue()), "wait for texture upload");
		}

		if (pool)
			vkDestroyCommandPool(device, pool, nullptr);
		if (staging)
			vkDestroyBuffer(device, staging, nullptr);
		if (stagingMemory)
			vkFreeMemory(device, stagingMemory, nullptr);

		return ready;
	}

}

namespace Lite {

	Texture::~Texture()
	{
		DestroyGpu();
	}

	bool Texture::LoadFromFile(const std::filesystem::path& path)
	{
		std::vector<uint8_t> pixels;
		if (!DecodeImage(path, m_Width, m_Height, pixels))
			return false;

		m_Device = Renderer::GetDevice();
		if (!m_Device || !UploadImage(m_Device, Renderer::GetPhysicalDevice(), m_Width, m_Height, pixels, m_Image, m_Memory))
		{
			DestroyGpu();
			return false;
		}

		VkImageViewCreateInfo viewInfo {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		if (!CheckVk(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_View), "create texture view"))
		{
			DestroyGpu();
			return false;
		}

		VkSamplerCreateInfo samplerInfo {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxLod = 0.0f;
		if (!CheckVk(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler), "create texture sampler"))
		{
			DestroyGpu();
			return false;
		}

		VkDescriptorSetLayoutBinding binding {};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &binding;
		if (!CheckVk(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_SetLayout), "create texture layout"))
		{
			DestroyGpu();
			return false;
		}

		VkDescriptorPoolSize poolSize {};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		if (!CheckVk(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_Pool), "create texture pool"))
		{
			DestroyGpu();
			return false;
		}

		VkDescriptorSetAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_Pool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_SetLayout;
		if (!CheckVk(vkAllocateDescriptorSets(m_Device, &allocateInfo, &m_Set), "allocate texture set"))
		{
			DestroyGpu();
			return false;
		}

		VkDescriptorImageInfo descriptorImage {};
		descriptorImage.sampler = m_Sampler;
		descriptorImage.imageView = m_View;
		descriptorImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_Set;
		write.dstBinding = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &descriptorImage;
		vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);

		SetIdentity(path.generic_string(), path.filename().string());
		m_Loaded = true;
		LITE_INFO("Texture ready ({} x {})", m_Width, m_Height);
		return true;
	}

	void Texture::DestroyGpu()
	{
		if (!m_Device)
			return;

		vkDeviceWaitIdle(m_Device);

		if (m_Pool)
			vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
		if (m_SetLayout)
			vkDestroyDescriptorSetLayout(m_Device, m_SetLayout, nullptr);
		if (m_Sampler)
			vkDestroySampler(m_Device, m_Sampler, nullptr);
		if (m_View)
			vkDestroyImageView(m_Device, m_View, nullptr);
		if (m_Image)
			vkDestroyImage(m_Device, m_Image, nullptr);
		if (m_Memory)
			vkFreeMemory(m_Device, m_Memory, nullptr);

		m_Pool = VK_NULL_HANDLE;
		m_Set = VK_NULL_HANDLE;
		m_SetLayout = VK_NULL_HANDLE;
		m_Sampler = VK_NULL_HANDLE;
		m_View = VK_NULL_HANDLE;
		m_Image = VK_NULL_HANDLE;
		m_Memory = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
	}

	void Texture::Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &m_Set, 0, nullptr);
	}

	Ref<Asset> TextureHandler::Load(const std::filesystem::path& path)
	{
		auto texture = CreateRef<Texture>();
		if (!texture->LoadFromFile(path))
			return nullptr;

		return texture;
	}

}
