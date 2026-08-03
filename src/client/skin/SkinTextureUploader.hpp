/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

#pragma once

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::client::skin {

/**
 * @brief 皮肤纹理上传器
 *
 * 负责将玩家皮肤像素上传到注入的实体纹理图集（渲染器唯一图集，非 ClientSkinManager
 * 自建孤儿图集）。统一通过 EntityTextureAtlas::injectRegion 上传到动态区域：
 * - 默认皮肤：initialize 时一次性上传 18 个变体（占位动态区域，永不移除）
 * - 自定义皮肤：按 UUID 懒上传，玩家离线时 removeRegion
 *
 * 区域命名约定：player_skin:<uuid-hex-no-dashes>，便于按 UUID 查询/移除。
 * 上传失败（动态区域空间耗尽）时返回 nullptr，调用方回退到默认皮肤区域。
 *
 * 线程安全：内部 mutex 保护 m_regionByUuidKey。注入的图集由调用方保证生命周期。
 */
class SkinTextureUploader {
public:
    SkinTextureUploader() = default;
    ~SkinTextureUploader() = default;

    // 禁止拷贝
    SkinTextureUploader(const SkinTextureUploader&) = delete;
    SkinTextureUploader& operator=(const SkinTextureUploader&) = delete;

    /**
     * @brief 注入实体纹理图集（渲染器唯一图集）
     *
     * 必须在 loadDefaultSkins / getOrCreateRegion 之前调用。图集须已 build()。
     * 图集生命周期由调用方（TridentEngine）管理，本类不持有所有权。
     */
    void setTextureAtlas(renderer::entity::pipeline::EntityTextureAtlas* atlas) { m_textureAtlas = atlas; }

    /**
     * @brief 上传 18 种默认皮肤变体到图集动态区域
     *
     * 在 ClientSkinManager::initialize 时一次性调用。每个变体按其规范 ResourceLocation
     *（如 minecraft:textures/entity/player/slim/steve.png）注入，确保与 DefaultSkinProvider
     * 给出的 location 一致——PlayerSkinRegionProvider 查默认皮肤时用同一 location 命中。
     *
     * @param skinDataByIndex 18 个变体的 RGBA 像素数据（索引与 DefaultSkinVariant::index 一致）
     * @param locations 18 个变体的 ResourceLocation（索引对应）
     * @return 成功上传的变体数量（单个失败仅 warn 跳过）
     */
    u32 loadDefaultSkins(const std::array<std::vector<u8>, ::mc::skin::DEFAULT_SKIN_COUNT>& skinDataByIndex,
        const std::array<ResourceLocation, ::mc::skin::DEFAULT_SKIN_COUNT>& locations);

    /**
     * @brief 按名称查询区域（默认皮肤 + 自定义皮肤统一入口）
     * @param location 图集资源位置键
     * @return 区域指针；不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* findRegion(const ResourceLocation& location) const;

    /**
     * @brief 按 UUID 查询默认皮肤区域
     *
     * 根据_uuid 哈希选变体，返回该变体在图集中的区域（loadDefaultSkins 时已注入）。
     */
    [[nodiscard]] const TextureRegion* getDefaultRegion(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 懒上传自定义皮肤
     *
     * 命中缓存返回已有区域；未上传则 injectRegion。动态区域空间耗尽返回 nullptr。
     *
     * @param uuid 玩家 UUID
     * @param rgbaPixels 64x64 RGBA 像素数据
     * @return 皮肤区域指针；失败返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getOrCreateRegion(
        const std::array<u8, 16>& uuid, const std::vector<u8>& rgbaPixels) const;

    /**
     * @brief 按 UUID 移除自定义皮肤区域（玩家离线）
     *
     * 仅移除动态自定义区域，默认皮肤区域不移除。触发图集 contentVersion 自增，
     * 使 AnimatedMeshCache 重做 UV（皮肤区域变更后下次渲染自动回退默认）。
     */
    void removeRegion(const std::array<u8, 16>& uuid);

    /**
     * @brief 清除所有自定义皮肤区域映射（shutdown 时调用）
     *
     * 注意：不调用 removeDynamicRegion（图集即将销毁，无需逐个移除）。
     */
    void clear();

private:
    /**
     * @brief UUID 转 player_skin:<hex> ResourceLocation
     */
    [[nodiscard]] static ResourceLocation _uuidToLocation(const std::array<u8, 16>& uuid);

    renderer::entity::pipeline::EntityTextureAtlas* m_textureAtlas = nullptr;

    // 默认皮肤区域缓存（索引与 DefaultSkinVariant::index 一致），loadDefaultSkins 填充
    std::array<const TextureRegion*, ::mc::skin::DEFAULT_SKIN_COUNT> m_defaultRegions{};

    // 自定义皮肤 UUID-key -> 区域指针（懒上传缓存）
    mutable std::mutex m_customMutex;
    mutable std::unordered_map<std::string, const TextureRegion*> m_customRegions;
};

} // namespace mc::client::skin
