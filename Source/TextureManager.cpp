// Copyright (c) 2020-2021 Sultim Tsyrendashiev
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "TextureManager.h"

#include <numeric>

#include "Const.h"
#include "Utils.h"
#include "TextureOverrides.h"

using namespace RTGL1;

#define DEFAULT_TEXTURES_PATH               ""
#define DEFAULT_ALBEDO_ALPHA_POSTFIX        ""
#define DEFAULT_NORMAL_METALLIC_POSTFIX     "_n"
#define DEFAULT_EMISSION_ROUGHNESS_POSTFIX  "_e"

constexpr VkFormat      IMAGE_FORMAT_SRGB = VK_FORMAT_R8G8B8A8_SRGB;
constexpr VkFormat      IMAGE_FORMAT_UNORM = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkDeviceSize  IMAGE_BYTES_PER_PIXEL = 4;

constexpr MaterialTextures EmptyMaterialTextures = { EMPTY_TEXTURE_INDEX, EMPTY_TEXTURE_INDEX,EMPTY_TEXTURE_INDEX };

TextureManager::TextureManager(
    VkDevice _device,
    std::shared_ptr<MemoryAllocator> _memAllocator,
    std::shared_ptr<SamplerManager> _samplerMgr,
    const std::shared_ptr<CommandBufferManager> &_cmdManager,
    const char *_defaultTexturesPath,
    const char *_albedoAlphaPostfix,
    const char *_normalMetallicPostfix,
    const char *_emissionRoughnessPostfix)
:
    device(_device),
    samplerMgr(_samplerMgr)
{
    this->defaultTexturesPath = _defaultTexturesPath != nullptr ? _defaultTexturesPath : DEFAULT_TEXTURES_PATH;
    this->albedoAlphaPostfix = _albedoAlphaPostfix != nullptr ? _albedoAlphaPostfix : DEFAULT_ALBEDO_ALPHA_POSTFIX;
    this->normalMetallicPostfix = _normalMetallicPostfix != nullptr ? _normalMetallicPostfix : DEFAULT_NORMAL_METALLIC_POSTFIX;
    this->emissionRoughnessPostfix = _emissionRoughnessPostfix != nullptr ? _emissionRoughnessPostfix : DEFAULT_EMISSION_ROUGHNESS_POSTFIX;

    imageLoader = std::make_shared<ImageLoader>();
    textureDesc = std::make_shared<TextureDescriptors>(device);
    textureUploader = std::make_shared<TextureUploader>(device, std::move(_memAllocator));

    textures.resize(MAX_TEXTURE_COUNT);

    // submit cmd to create empty texture
    VkCommandBuffer cmd = _cmdManager->StartGraphicsCmd();
    CreateEmptyTexture(cmd, 0);
    _cmdManager->Submit(cmd);
    _cmdManager->WaitGraphicsIdle();
}

void TextureManager::CreateEmptyTexture(VkCommandBuffer cmd, uint32_t frameIndex)
{
    assert(textures[0].image == VK_NULL_HANDLE && textures[0].view == VK_NULL_HANDLE);

    uint32_t data[] = { 0xFFFFFFFF };
    RgExtent2D size = { 1,1 };
    VkSampler sampler = samplerMgr->GetSampler(RG_SAMPLER_FILTER_NEAREST, RG_SAMPLER_ADDRESS_MODE_REPEAT, RG_SAMPLER_ADDRESS_MODE_REPEAT);

    uint32_t textureIndex = PrepareStaticTexture(cmd, frameIndex, data, size, sampler, false, "Empty texture");

    // must have specific index
    assert(textureIndex == EMPTY_TEXTURE_INDEX);

    VkImage emptyImage = textures[textureIndex].image;
    VkImageView emptyView = textures[textureIndex].view;

    assert(emptyImage != VK_NULL_HANDLE && emptyView != VK_NULL_HANDLE);

    // if texture will be reset, it will use empty texture's info
    textureDesc->SetEmptyTextureInfo(emptyView, sampler);
}

TextureManager::~TextureManager()
{
    for (auto &texture : textures)
    {
        assert((texture.image == VK_NULL_HANDLE && texture.view == VK_NULL_HANDLE) ||
               (texture.image != VK_NULL_HANDLE && texture.view != VK_NULL_HANDLE));

        if (texture.image != VK_NULL_HANDLE)
        {
            DestroyTexture(texture);
        }
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        for (auto &texture : texturesToDestroy[i])
        {
            DestroyTexture(texture);
        }
    }
}

void TextureManager::PrepareForFrame(uint32_t frameIndex)
{
    // destroy delayed textures
    for (auto &texture : texturesToDestroy[frameIndex])
    {
        DestroyTexture(texture);
    }
    texturesToDestroy[frameIndex].clear();

    // clear staging buffer that are not in use
    textureUploader->ClearStaging(frameIndex);
}

void TextureManager::SubmitDescriptors(uint32_t frameIndex)
{
    // update desc set with current values
    UpdateDescSet(frameIndex);
}

void TextureManager::UpdateDescSet(uint32_t frameIndex)
{
    for (uint32_t i = 0; i < textures.size(); i++)
    {
        if (textures[i].image != VK_NULL_HANDLE)
        {
            textureDesc->UpdateTextureDesc(frameIndex, i, textures[i].view, textures[i].sampler);
        }
        else
        {
            // reset descriptor to empty texture
            textureDesc->ResetTextureDesc(frameIndex, i);
        }
    }

    textureDesc->FlushDescWrites();
}

uint32_t TextureManager::CreateStaticMaterial(VkCommandBuffer cmd, uint32_t frameIndex, const RgStaticMaterialCreateInfo &createInfo)
{
    MaterialTextures textures = {};
    VkSampler sampler = samplerMgr->GetSampler(createInfo.filter, createInfo.addressModeU, createInfo.addressModeV);

    if (!createInfo.disableOverride)
    {
        ParseInfo parseInfo = {};
        parseInfo.texturesPath = defaultTexturesPath.c_str();
        parseInfo.albedoAlphaPostfix = albedoAlphaPostfix.c_str();
        parseInfo.normalMetallicPostfix = normalMetallicPostfix.c_str();
        parseInfo.emissionRoughnessPostfix = emissionRoughnessPostfix.c_str();

        // load additional textures, they'll be freed after leaving the scope
        TextureOverrides ovrd(createInfo, parseInfo, imageLoader);

        textures.albedoAlpha        = PrepareStaticTexture(cmd, frameIndex, ovrd.aa, ovrd.aaSize, sampler, true, createInfo.useMipmaps, ovrd.debugName);
        textures.normalMetallic     = PrepareStaticTexture(cmd, frameIndex, ovrd.nm, ovrd.nmSize, sampler, true, createInfo.useMipmaps, ovrd.debugName);
        textures.emissionRoughness  = PrepareStaticTexture(cmd, frameIndex, ovrd.er, ovrd.erSize, sampler, true, createInfo.useMipmaps, ovrd.debugName);
    }
    else
    {
        textures.albedoAlpha        = PrepareStaticTexture(cmd, frameIndex, createInfo.data, createInfo.size, sampler, createInfo.isSRGB, createInfo.relativePath);
        textures.normalMetallic     = EMPTY_TEXTURE_INDEX;
        textures.emissionRoughness  = EMPTY_TEXTURE_INDEX;
    }

    return InsertMaterial(textures, false);
}

uint32_t TextureManager::PrepareStaticTexture(
    VkCommandBuffer cmd, uint32_t frameIndex, const void *data, const RgExtent2D &size,
    VkSampler sampler, bool isSRGB, bool generateMipmaps, const char *debugName)
{
    return PrepareTexture(false, cmd, frameIndex, data, size, sampler,  generateMipmaps, debugName);
}

uint32_t TextureManager::CreateDynamicMaterial(VkCommandBuffer cmd, uint32_t frameIndex, const RgDynamicMaterialCreateInfo &createInfo)
{
    VkSampler sampler = samplerMgr->GetSampler(createInfo.filter, createInfo.addressModeU, createInfo.addressModeV);

    MaterialTextures textures = {};
    textures.albedoAlpha        = PrepareDynamicTexture(cmd, frameIndex, createInfo.data, createInfo.size, sampler, createInfo.isSRGB, createInfo.useMipmaps);
    textures.normalMetallic     = EMPTY_TEXTURE_INDEX;
    textures.emissionRoughness  = EMPTY_TEXTURE_INDEX;

    return InsertMaterial(textures, true);
}

bool TextureManager::UpdateDynamicMaterial(VkCommandBuffer cmd, const RgDynamicMaterialUpdateInfo &updateInfo)
{
    const auto it = materials.find(updateInfo.dynamicMaterial);

    if (it != materials.end())
    {
        if (it->second.isDynamic)
        {
            // dynamic textures have only albedo/alpha
            uint32_t textureIndex = it->second.textures.albedoAlpha;

            if (textureIndex == EMPTY_TEXTURE_INDEX)
            {
                return false;
            }

            VkImage img = textures[textureIndex].image;

            if (img == VK_NULL_HANDLE)
            {
                return false;
            }

            textureUploader->UpdateDynamicImage(cmd, img, updateInfo.data);
            return true;
        }
    }

    return false;
}

uint32_t TextureManager::PrepareDynamicTexture(
    VkCommandBuffer cmd, uint32_t frameIndex, const void *data, const RgExtent2D &size, 
    VkSampler sampler, bool isSRGB, bool generateMipmaps, const char *debugName)
{
    return PrepareTexture(true, cmd, frameIndex, data, size, sampler, generateMipmaps, debugName);
}

uint32_t TextureManager::PrepareTexture(
    bool isDynamic, VkCommandBuffer cmd, uint32_t frameIndex, const void *data,
    const RgExtent2D &size, VkSampler sampler, bool isSRGB, bool generateMipmaps, const char *debugName)
{
    // only dynamic textures can have null data
    if (!isDynamic && data == nullptr)
    {
        return EMPTY_TEXTURE_INDEX;
    }

    assert(size.width > 0 && size.height > 0);

    TextureUploader::UploadInfo info = {};
    info.cmd = cmd;
    info.frameIndex = frameIndex;
    info.data = data;
    info.size = size;
    info.format = isSRGB ? IMAGE_FORMAT_SRGB : IMAGE_FORMAT_UNORM;
    info.bytesPerPixel = IMAGE_BYTES_PER_PIXEL;
    info.isDynamic = isDynamic;
    info.generateMipmaps = generateMipmaps;
    info.debugName = debugName;

    auto result = textureUploader->UploadImage(info);

    if (!result.wasUploaded)
    {
        return EMPTY_TEXTURE_INDEX;
    }

    return InsertTexture(result.image, result.view, sampler);
}

uint32_t TextureManager::CreateAnimatedMaterial(VkCommandBuffer cmd, uint32_t frameIndex, const RgAnimatedMaterialCreateInfo &createInfo)
{
    if (createInfo.frameCount == 0)
    {
        return RG_NO_MATERIAL;
    }

    std::vector<uint32_t> materialIndices(createInfo.frameCount);

    // animated material is a series of static materials
    for (uint32_t i = 0; i < createInfo.frameCount; i++)
    {
        materialIndices[i] = CreateStaticMaterial(cmd, frameIndex, createInfo.frames[i]);
    }

    return InsertAnimatedMaterial(materialIndices);
}

bool TextureManager::ChangeAnimatedMaterialFrame(uint32_t animMaterial, uint32_t materialFrame)
{
    const auto animIt = animatedMaterials.find(animMaterial);

    if (animIt != animatedMaterials.end())
    {
        AnimatedMaterial &anim = animIt->second;

        materialFrame = std::min(materialFrame, (uint32_t)anim.materialIndices.size());
        anim.currentFrame = materialFrame;

        // notify subscribers
        for (auto &ws : subscribers)
        {
            // if subscriber still exist
            if (auto s = ws.lock())
            {
                uint32_t frameMatIndex = anim.materialIndices[anim.currentFrame];

                // find MaterialTextures
                auto it = materials.find(frameMatIndex);

                if (it != materials.end())
                {
                    s->OnMaterialChange(animMaterial, it->second.textures);
                }
            }
        }

        return true;
    }

    return false;
}

uint32_t TextureManager::GenerateMaterialIndex(const MaterialTextures &materialTextures)
{
    uint32_t matIndex = materialTextures.indices[0] + materialTextures.indices[1] + materialTextures.indices[2];

    while (materials.find(matIndex) != materials.end())
    {
        matIndex++;
    }

    return matIndex;
}

uint32_t TextureManager::GenerateMaterialIndex(const std::vector<uint32_t> &materialIndices)
{
    uint32_t matIndex = std::accumulate(materialIndices.begin(), materialIndices.end(), 0u);

    // all materials share the same pool of indices
    while (materials.find(matIndex) != materials.end())
    {
        matIndex++;
    }

    return matIndex;
}

uint32_t TextureManager::InsertMaterial(const MaterialTextures &materialTextures, bool isDynamic)
{
    bool isEmpty = true;

    for (uint32_t t : materialTextures.indices)
    {
        if (t != EMPTY_TEXTURE_INDEX)
        {
            isEmpty = false;
            break;
        }
    }

    if (isEmpty)
    {
        return RG_NO_MATERIAL;
    }

    uint32_t matIndex = GenerateMaterialIndex(materialTextures);

    Material material = {};
    material.isDynamic = isDynamic;
    material.textures = materialTextures;

    materials[matIndex] = material;
    return matIndex;
}

uint32_t TextureManager::InsertAnimatedMaterial(std::vector<uint32_t> &materialIndices)
{
    bool isEmpty = true;

    for (uint32_t m : materialIndices)
    {
        if (m != RG_NO_MATERIAL)
        {
            isEmpty = false;
            break;
        }    
    }

    if (isEmpty)
    {
        return RG_NO_MATERIAL;
    }

    uint32_t animMatIndex = GenerateMaterialIndex(materialIndices);

    animatedMaterials[animMatIndex] = {};

    AnimatedMaterial &animMat = animatedMaterials[animMatIndex];
    animMat.currentFrame = 0;
    animMat.materialIndices = std::move(materialIndices);

    return animMatIndex;
}

void TextureManager::DestroyMaterialTextures(uint32_t frameIndex, uint32_t materialIndex)
{
    auto it = materials.find(materialIndex);

    if (it != materials.end())
    {
        DestroyMaterialTextures(frameIndex, it->second);
    }
}

void TextureManager::DestroyMaterialTextures(uint32_t frameIndex, const Material &material)
{
    for (auto t : material.textures.indices)
    {
        if (t != EMPTY_TEXTURE_INDEX)
        {
            Texture &texture = textures[t];

            AddToBeDestroyed(frameIndex, texture);

            // null data
            texture.image = VK_NULL_HANDLE;
            texture.view = VK_NULL_HANDLE;
            texture.sampler = VK_NULL_HANDLE;
        }
    }
}

void TextureManager::DestroyMaterial(uint32_t currentFrameIndex, uint32_t materialIndex)
{
    const auto animIt = animatedMaterials.find(materialIndex);

    // if it's an animated material
    if (animIt != animatedMaterials.end())
    {
        AnimatedMaterial &anim = animIt->second;

        // destroy each material
        for (auto &mat : anim.materialIndices)
        {
            DestroyMaterialTextures(currentFrameIndex, mat);
        }

        animatedMaterials.erase(animIt);
    }
    else
    {
        auto it = materials.find(materialIndex);

        if (it != materials.end())
        {
            DestroyMaterialTextures(currentFrameIndex, it->second);
            materials.erase(it);
        }
    }

    // notify subscribers
    for (auto &ws : subscribers)
    {
        if (auto s = ws.lock())
        {
            // send them empty texture indices as material is destroyed
            s->OnMaterialChange(materialIndex, EmptyMaterialTextures);
        }
    }
}

uint32_t TextureManager::InsertTexture(VkImage image, VkImageView view, VkSampler sampler)
{
    auto texture = std::find_if(textures.begin(), textures.end(), [] (const Texture &t)
    {
        return t.image == VK_NULL_HANDLE && t.view == VK_NULL_HANDLE;
    });

    // if coudn't find empty space, use empty texture
    if (texture == textures.end())
    {
        // clean created data
        textureUploader->DestroyImage(image, view);

        return EMPTY_TEXTURE_INDEX;
    }

    texture->image = image;
    texture->view = view;
    texture->sampler = sampler;

    return (uint32_t)std::distance(textures.begin(), texture);
}

void TextureManager::DestroyTexture(const Texture &texture)
{
    assert(texture.image != VK_NULL_HANDLE && texture.view != VK_NULL_HANDLE);
    textureUploader->DestroyImage(texture.image, texture.view);
}

void TextureManager::AddToBeDestroyed(uint32_t frameIndex, const Texture &texture)
{
    assert(texture.image != VK_NULL_HANDLE && texture.view != VK_NULL_HANDLE);

    texturesToDestroy[frameIndex].push_back(texture);
}

MaterialTextures TextureManager::GetMaterialTextures(uint32_t materialIndex) const
{
    if (materialIndex == RG_NO_MATERIAL)
    {
        return EmptyMaterialTextures;
    }

    const auto animIt = animatedMaterials.find(materialIndex);

    if (animIt != animatedMaterials.end())
    {
        const AnimatedMaterial &anim = animIt->second;

        // return material textures of the current frame
        return GetMaterialTextures(anim.materialIndices[anim.currentFrame]);
    }

    const auto it = materials.find(materialIndex);

    if (it == materials.end())
    {
        return EmptyMaterialTextures;
    }

    return it->second.textures;
}

VkDescriptorSet TextureManager::GetDescSet(uint32_t frameIndex) const
{
    return textureDesc->GetDescSet(frameIndex);
}

VkDescriptorSetLayout TextureManager::GetDescSetLayout() const
{
    return textureDesc->GetDescSetLayout();
}

void TextureManager::Subscribe(std::shared_ptr<IMaterialDependency> subscriber)
{
    subscribers.emplace_back(subscriber);
}

void TextureManager::Unsubscribe(const IMaterialDependency *subscriber)
{
    subscribers.remove_if([subscriber] (const std::weak_ptr<IMaterialDependency> &ws)
    {
        if (const auto s = ws.lock())
        {
            return s.get() == subscriber;
        }

        return true;
    });
}
