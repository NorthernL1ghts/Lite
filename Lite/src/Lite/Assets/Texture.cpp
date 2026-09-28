#include "Texture.h"

#include "Lite/Core/Logger.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Vulkan/VulkanUtils.h"

#include <cstring>
#include <limits>
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

		const uint64_t byteCount = static_cast<uint64_t>(imageWidth) * imageHeight * 4u;
		if (SUCCEEDED(result) && imageWidth > 0 && imageHeight > 0 && imageWidth <= (UINT_MAX / 4u) && byteCount <= std::numeric_limits<UINT>::max())
		{
			width = imageWidth;
			height = imageHeight;
			pixels.resize(static_cast<size_t>(byteCount));
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
		const uint64_t byteCount = static_cast<uint64_t>(width) * height * 4u;
		if (width == 0 || height == 0 || byteCount != pixels.size())
			return false;

		Lite::AllocatedBuffer staging {};
		if (!Lite::CreateBuffer(
			device,
			physicalDevice,
			pixels.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging,
			"create texture staging buffer"))
			return false;

		void* mapped = nullptr;
		if (!Lite::CheckVk(vkMapMemory(device, staging.Memory, 0, pixels.size(), 0, &mapped), "map texture staging") || !mapped)
		{
			Lite::DestroyBuffer(device, staging);
			return false;
		}

		std::memcpy(mapped, pixels.data(), pixels.size());
		vkUnmapMemory(device, staging.Memory);

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
		bool ready = Lite::CheckVk(vkCreateImage(device, &imageInfo, nullptr, &image), "create texture image");

		if (ready)
		{
			VkMemoryRequirements imageRequirements {};
			vkGetImageMemoryRequirements(device, image, &imageRequirements);
			ready = Lite::AllocateMemory(device, physicalDevice, imageRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory, "allocate texture memory")
				&& Lite::CheckVk(vkBindImageMemory(device, image, memory, 0), "bind texture memory");
		}

		if (ready)
		{
			ready = Lite::SubmitOnce(device, Lite::Renderer::GetGraphicsQueueFamily(), Lite::Renderer::GetGraphicsQueue(), [&](VkCommandBuffer command)
			{
				VkImageMemoryBarrier toTransfer = Lite::ImageBarrier(
					image,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					0,
					VK_ACCESS_TRANSFER_WRITE_BIT);
				vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

				VkBufferImageCopy region {};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.layerCount = 1;
				region.imageExtent = { width, height, 1 };
				vkCmdCopyBufferToImage(command, staging.Buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				VkImageMemoryBarrier toSample = Lite::ImageBarrier(
					image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT);
				vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toSample);
			}, "submit texture upload");
		}

		Lite::DestroyBuffer(device, staging);
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
