/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "client/resource/atlas/AtlasManager.hpp"

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include "client/renderer/trident/core/texture/AnimatedSprite.hpp"
#include "client/renderer/trident/core/texture/TextureAtlasTicker.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"
#include "client/resource/atlas/AtlasConfigLoader.hpp"
#include "client/resource/atlas/AtlasHandle.hpp"
#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/MissingNo.hpp"
#include "client/resource/atlas/SpriteContents.hpp"
#include "client/resource/atlas/SpriteLoader.hpp"
#include "client/resource/atlas/TexturePathVariant.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::resource::atlas {

// 静态空包列表兜底（m_packs 未设置时）
const std::vector<ResourcePackPtr> AtlasManager::m_emptyPacks{};

AtlasManager::~AtlasManager()
{
    destroy();
}

AtlasEntry::~AtlasEntry() = default;

void AtlasManager::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    renderer::api::IStagingBufferPool* stagingPool)
{
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_stagingPool = stagingPool;
}

void AtlasManager::destroy()
{
    for (auto& [id, entry] : m_atlases) {
        if (entry) {
            entry->handle.destroy();
            entry->ticker.reset();
        }
    }
    m_atlases.clear();
    m_missingRegion.reset();
    // 不重置 Vulkan 句柄（由所有者释放，便于 destroy 后再次 initialize）
}

void AtlasManager::setResourcePacks(const std::vector<ResourcePackPtr>* packs)
{
    m_packs = packs;
}

const std::vector<ResourcePackPtr>& AtlasManager::_packs() const
{
    return (m_packs != nullptr) ? *m_packs : m_emptyPacks;
}

VkFilter AtlasManager::_filterForAtlas(const ResourceLocation& atlasId)
{
    // per-atlas 采样配置：particles/gui 用 LINEAR，其余用 NEAREST
    const std::string& path = atlasId.path();
    if (path == "particles" || path == "gui" || path == "effects") {
        return VK_FILTER_LINEAR;
    }
    return VK_FILTER_NEAREST;
}

void AtlasManager::_destroyAtlas(const ResourceLocation& atlasId)
{
    auto it = m_atlases.find(atlasId);
    if (it != m_atlases.end()) {
        if (it->second) {
            it->second->handle.destroy();
        }
        m_atlases.erase(it);
    }
}

Result<void> AtlasManager::loadAtlas(const ResourceLocation& atlasId)
{
    if (m_device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotInitialized, "AtlasManager not initialized");
    }

    const auto& packs = _packs();

    // 1. 读 atlas JSON，多包 sources 拼接
    auto sourcesResult = AtlasConfigLoader::load(packs, atlasId);
    if (sourcesResult.failed()) {
        return sourcesResult.error();
    }
    auto& sources = sourcesResult.value();
    if (sources.empty()) {
        spdlog::warn("AtlasManager: atlas {} has no sources", atlasId.toString());
    }

    // 2. 依次 run sources，累积到 SpriteSourceOutput
    SpriteSourceOutput output;
    for (const auto& source : sources) {
        // sources 来自多包拼接；run 时用最后一个非空包（与 directory 等需要实际枚举的 source 语义一致）
        // 注：SpriteLoader.resolve 会按 pack 优先级查找，此处 run 主要为 directory/list 类 source 提供 pack
        for (auto packIt = packs.rbegin(); packIt != packs.rend(); ++packIt) {
            if (*packIt == nullptr) {
                continue;
            }
            auto runResult = source->run(**packIt, output);
            if (runResult.failed()) {
                spdlog::warn("AtlasManager: source '{}' run failed for atlas {}: {}",
                    source->describe(),
                    atlasId.toString(),
                    runResult.error().message());
            }
            break; // 只需一个 pack 提供枚举上下文
        }
    }

    // 3. build 出唯一 sprite 列表，末尾追加 missingno 兜底
    auto sprites = output.build();

    const ResourceLocation& missingLoc = MissingNo::spriteLocation();
    SpriteContents missingContents;
    missingContents.width = 16;
    missingContents.height = 16;
    missingContents.pixels = MissingNo::generatePixels();
    sprites.emplace_back(missingLoc, SpriteLoader::fromPredecoded(std::move(missingContents)));

    if (sprites.empty()) {
        spdlog::warn("AtlasManager: atlas {} produced no sprites", atlasId.toString());
    }

    // 4. 解码每个 sprite 像素，喂 TextureAtlasBuilder 打包
    TextureAtlasBuilder builder;
    builder.setMaxSize(4096, 4096);

    for (const auto& [spriteName, loader] : sprites) {
        auto resolveResult = loader.resolve(packs);
        if (resolveResult.failed()) {
            spdlog::warn("AtlasManager: failed to resolve sprite {} for atlas {}: {}",
                spriteName.toString(),
                atlasId.toString(),
                resolveResult.error().message());
            continue;
        }
        const auto& contents = resolveResult.value();

        u32 frameWidth = contents.width;
        u32 frameHeight = contents.height;
        if (contents.animation && contents.animation->width > 0 && contents.animation->height > 0) {
            frameWidth = static_cast<u32>(contents.animation->width);
            frameHeight = static_cast<u32>(contents.animation->height);
            builder.addTextureFrame(spriteName,
                contents.pixels,
                contents.width,
                contents.height,
                frameWidth,
                frameHeight,
                *contents.animation);
        } else {
            builder.addTextureFrame(
                spriteName, contents.pixels, contents.width, contents.height, frameWidth, frameHeight);
        }
    }

    // 5. 打包
    auto buildResult = builder.build();
    if (buildResult.failed()) {
        return buildResult.error();
    }
    auto& atlas = buildResult.value();
    if (atlas.pixels.empty()) {
        spdlog::warn("AtlasManager: atlas {} built empty", atlasId.toString());
    }

    // 6. 销毁旧图集（若存在）并创建新 AtlasEntry
    _destroyAtlas(atlasId);

    auto entry = std::make_unique<AtlasEntry>();
    entry->atlasId = atlasId;
    entry->filter = _filterForAtlas(atlasId);
    entry->width = atlas.width;
    entry->height = atlas.height;
    entry->regions = atlas.regions;

    // 创建 Vulkan 资源并上传
    auto createResult = entry->handle.create(m_device,
        m_physicalDevice,
        m_commandPool,
        m_graphicsQueue,
        m_stagingPool,
        atlas.width,
        atlas.height,
        entry->filter);
    if (createResult.failed()) {
        return createResult;
    }

    const u64 pixelSize = static_cast<u64>(atlas.width) * atlas.height * 4;
    auto uploadResult = entry->handle.upload(atlas.pixels.data(), pixelSize, atlas.width, atlas.height);
    if (uploadResult.failed()) {
        return uploadResult;
    }

    // 7. 注册动画（AnimationDescriptor→AnimatedSprite 转换，由 tickAnimations/uploadPendingAnimationFrames 驱动）
    if (!atlas.animations.empty()) {
        entry->ticker = std::make_unique<renderer::trident::TextureAtlasTicker>();
        for (const auto& anim : atlas.animations) {
            std::vector<renderer::trident::AnimatedSprite::FrameData> frames;
            frames.reserve(anim.framePixels.size());
            for (const auto& frameData : anim.framePixels) {
                renderer::trident::AnimatedSprite::FrameData frame;
                frame.pixels = frameData;
                frame.width = anim.frameWidth;
                frame.height = anim.frameHeight;
                frames.push_back(std::move(frame));
            }
            auto sprite = std::make_shared<renderer::trident::AnimatedSprite>(
                anim.metadata, std::move(frames), anim.atlasX, anim.atlasY);
            sprite->setLocation(anim.location);
            entry->ticker->registerAnimatedSprite(std::move(sprite));
        }
    }

    // 设置 missingno 兜底区域（用首个加载图集的 missingno 区域）
    if (!m_missingRegion) {
        auto missIt = atlas.regions.find(missingLoc);
        if (missIt != atlas.regions.end()) {
            m_missingRegion = std::make_unique<TextureRegion>(missIt->second);
        }
    }

    const ResourceLocation key = atlasId;
    m_atlases.emplace(key, std::move(entry));

    spdlog::info("AtlasManager: loaded atlas {} ({}x{}, {} sprites, {} animations)",
        atlasId.toString(),
        atlas.width,
        atlas.height,
        atlas.regions.size(),
        atlas.animations.size());

    return {};
}

Result<void> AtlasManager::loadAll(const std::vector<ResourceLocation>& atlasIds)
{
    for (const auto& atlasId : atlasIds) {
        auto result = loadAtlas(atlasId);
        if (result.failed()) {
            spdlog::warn("AtlasManager: failed to load atlas {}: {}", atlasId.toString(), result.error().message());
        }
    }
    return {};
}

Result<void> AtlasManager::reload(const std::vector<ResourceLocation>& atlasIds)
{
    for (auto& [id, entry] : m_atlases) {
        if (entry) {
            entry->handle.destroy();
            entry->ticker.reset();
        }
    }
    m_atlases.clear();
    m_missingRegion.reset();
    return loadAll(atlasIds);
}

const AtlasEntry* AtlasManager::findAtlas(const ResourceLocation& atlasId) const
{
    auto it = m_atlases.find(atlasId);
    return (it != m_atlases.end()) ? it->second.get() : nullptr;
}

const TextureRegion* AtlasManager::findSprite(const ResourceLocation& spriteName) const
{
    for (const auto& [id, entry] : m_atlases) {
        if (!entry) {
            continue;
        }
        auto it = entry->regions.find(spriteName);
        if (it != entry->regions.end()) {
            return &it->second;
        }
    }
    return nullptr;
}

const TextureRegion* AtlasManager::findSpriteWithVariant(const ResourceLocation& spriteName) const
{
    if (const TextureRegion* region = findSprite(spriteName); region != nullptr) {
        return region;
    }
    // 路径变体回退：textures/block/stone ↔ textures/blocks/stone 等
    const std::string altPath = TexturePathVariant::getAltTexturePath(spriteName.path());
    if (!altPath.empty()) {
        const ResourceLocation altLoc(spriteName.namespace_(), altPath);
        if (const TextureRegion* region = findSprite(altLoc); region != nullptr) {
            return region;
        }
    }
    return nullptr;
}

const TextureRegion* AtlasManager::findSpriteByTexturePath(const ResourceLocation& textureLocation) const
{
    // 剥掉 "textures/" 前缀：textures/block/stone -> block/stone
    const std::string& path = textureLocation.path();
    constexpr std::string_view texturesPrefix = "textures/";
    std::string spritePath;
    if (path.size() > texturesPrefix.size() && path.compare(0, texturesPrefix.size(), texturesPrefix) == 0) {
        spritePath = path.substr(texturesPrefix.size());
    } else {
        spritePath = path;
    }
    const ResourceLocation spriteName(textureLocation.namespace_(), std::move(spritePath));
    return findSpriteWithVariant(spriteName);
}

const TextureRegion* AtlasManager::findSpriteInAtlas(
    const ResourceLocation& atlasId, const ResourceLocation& spriteName) const
{
    const AtlasEntry* entry = findAtlas(atlasId);
    if (entry == nullptr) {
        return nullptr;
    }
    auto it = entry->regions.find(spriteName);
    return (it != entry->regions.end()) ? &it->second : nullptr;
}

const TextureRegion* AtlasManager::injectRuntimeSprite(const ResourceLocation& atlasId,
    const ResourceLocation& spriteName,
    const std::vector<u8>& pixels,
    u32 width,
    u32 height)
{
    auto it = m_atlases.find(atlasId);
    if (it == m_atlases.end() || it->second == nullptr) {
        return nullptr;
    }
    AtlasEntry& entry = *it->second;

    if (width == 0 || height == 0 || width > entry.width) {
        spdlog::warn("AtlasManager::injectRuntimeSprite: invalid sprite {} size {}x{} for atlas {} ({}x{})",
            spriteName.toString(),
            width,
            height,
            atlasId.toString(),
            entry.width,
            entry.height);
        return nullptr;
    }

    // 右下角追加策略：从左侧起按行堆叠运行时注入的 sprite，
    // nextInjectY 记录下一可用行的 Y。剩余高度不足时回退到图集底部。
    const u32 offsetX = 0;
    u32 offsetY = entry.nextInjectY;
    if (offsetY + height > entry.height) {
        // 空间不足，回退到图集右下角固定位置（覆盖区域，仅皮肤等少量注入可接受）
        offsetY = (entry.height >= height) ? (entry.height - height) : 0;
    } else {
        entry.nextInjectY = offsetY + height;
    }

    const u64 size = static_cast<u64>(width) * height * 4;
    auto uploadResult = entry.handle.uploadRegion(pixels.data(), size, offsetX, offsetY, width, height, width);
    if (uploadResult.failed()) {
        spdlog::warn("AtlasManager::injectRuntimeSprite: upload failed for {}: {}",
            spriteName.toString(),
            uploadResult.error().message());
        return nullptr;
    }

    // 计算 UV 区域
    const f64 u0 = static_cast<f64>(offsetX) / static_cast<f64>(entry.width);
    const f64 v0 = static_cast<f64>(offsetY) / static_cast<f64>(entry.height);
    const f64 u1 = static_cast<f64>(offsetX + width) / static_cast<f64>(entry.width);
    const f64 v1 = static_cast<f64>(offsetY + height) / static_cast<f64>(entry.height);

    auto [regionIt, inserted] = entry.regions.try_emplace(spriteName, u0, v0, u1, v1);
    if (!inserted) {
        // 已存在则覆盖
        regionIt->second = TextureRegion(u0, v0, u1, v1);
    }
    return &regionIt->second;
}

void AtlasManager::tickAnimations()
{
    for (auto& [id, entry] : m_atlases) {
        if (entry && entry->ticker) {
            entry->ticker->tick();
        }
    }
}

void AtlasManager::uploadPendingAnimationFrames(VkCommandBuffer cmd, u32 frameIndex)
{
    const bool asyncPath = (cmd != VK_NULL_HANDLE);
    for (auto& [id, entry] : m_atlases) {
        if (!entry || !entry->ticker || !entry->handle.isValid()) {
            continue;
        }

        // 同一图集内所有待上传 sprite 合并到一次录制/submit：收集区域描述符，
        // 交由 AtlasHandle 在单命令缓冲内录制 N 次 copy + layout 转换。
        // 异步路径随帧命令缓冲 submit、用帧 fence 同步；同步路径独立 submit+wait。
        std::vector<AtlasHandle::RegionUpload> batch;
        std::vector<renderer::trident::AnimatedSprite*> pendingSprites;

        const mc::Size count = entry->ticker->spriteCount();
        for (mc::Size i = 0; i < count; ++i) {
            auto* sprite = entry->ticker->getSprite(i);
            if (sprite == nullptr || !sprite->needsUpload()) {
                continue;
            }
            const auto& framePixels = sprite->currentFramePixels();
            if (framePixels.empty()) {
                sprite->markUploaded();
                continue;
            }
            batch.push_back({framePixels.data(),
                static_cast<u64>(sprite->frameWidth()) * sprite->frameHeight() * 4,
                sprite->atlasX(),
                sprite->atlasY(),
                sprite->frameWidth(),
                sprite->frameHeight(),
                sprite->frameWidth()});
            pendingSprites.push_back(sprite);
        }

        if (batch.empty()) {
            continue;
        }

        Result<void> result = asyncPath ? entry->handle.uploadRegionsBatchAsync(batch, cmd, frameIndex)
                                        : entry->handle.uploadRegionsBatch(batch);
        if (result.success()) {
            // 整批上传成功，统一标记已上传
            for (auto* sprite : pendingSprites) {
                sprite->markUploaded();
            }
        } else {
            // 池容量不足等失败：不标记，待上传的 sprite 下帧重试
            spdlog::warn("AtlasManager::uploadPendingAnimationFrames: batch upload failed for atlas {}: {}",
                id.toString(),
                result.error().message());
        }
    }
}

} // namespace mc::client::resource::atlas
