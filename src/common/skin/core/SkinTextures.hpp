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

#pragma once

#include "SkinTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>

namespace mc::skin {

/**
 * @brief 皮肤纹理类型
 */
enum class TextureType : u8 {
    Skin,  // 皮肤
    Cape,  // 披风
    Elytra // 鞘翅
};

/**
 * @brief 单个纹理的元数据
 *
 * 从 textures 属性的 Base64 JSON 中解析。
 */
struct TextureMetadata {
    std::string url;                  // 下载URL
    std::optional<std::string> hash;  // 纹理哈希（用于缓存文件名）
    std::optional<SkinType> skinType; // 仅皮肤：模型类型（default/slim）

    TextureMetadata() = default;
    TextureMetadata(TextureMetadata&& other) noexcept = default;
    TextureMetadata& operator=(TextureMetadata&& other) noexcept = default;
    TextureMetadata(const TextureMetadata& other) = default;
    TextureMetadata& operator=(const TextureMetadata& other) = default;

    /**
     * @brief 是否有效（必须有URL）
     */
    [[nodiscard]] bool isValid() const noexcept { return !url.empty(); }
};

/**
 * @brief 皮肤纹理集合
 *
 * 存储玩家的所有纹理资源定位信息。
 * 从 GameProfileProperty 的 Base64 JSON 解析而来。
 *
 * 示例 JSON 格式（Base64 解码后）：
 * @code
 * {
 *   "textures": {
 *     "SKIN": {
 *       "url": "http://textures.minecraft.net/texture/...",
 *       "metadata": { "model": "slim" }
 *     },
 *     "CAPE": {
 *       "url": "http://textures.minecraft.net/texture/..."
 *     }
 *   }
 * }
 * @endcode
 */
class SkinTextures {
public:
    SkinTextures() = default;
    SkinTextures(SkinTextures&& other) noexcept = default;
    SkinTextures& operator=(SkinTextures&& other) noexcept = default;
    SkinTextures(const SkinTextures& other) = default;
    SkinTextures& operator=(const SkinTextures& other) = default;

    // ========== 纹理获取 ==========

    /**
     * @brief 获取皮肤 ResourceLocation
     * @return 皮肤位置，不存在返回空optional
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getSkin() const noexcept { return m_skin; }

    /**
     * @brief 获取披风 ResourceLocation
     * @return 披风位置，不存在返回空optional
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getCape() const noexcept { return m_cape; }

    /**
     * @brief 获取鞘翅 ResourceLocation
     * @return 鞘翅位置，不存在返回空optional
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getElytra() const noexcept { return m_elytra; }

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] SkinType skinType() const noexcept { return m_skinType; }

    // ========== 纹理设置 ==========

    /**
     * @brief 设置皮肤位置
     */
    void setSkin(const ResourceLocation& location) { m_skin = location; }

    /**
     * @brief 设置披风位置
     */
    void setCape(const ResourceLocation& location) { m_cape = location; }

    /**
     * @brief 设置鞘翅位置
     */
    void setElytra(const ResourceLocation& location) { m_elytra = location; }

    /**
     * @brief 设置皮肤类型
     */
    void setSkinType(SkinType type) noexcept { m_skinType = type; }

    // ========== URL 获取（用于下载） ==========

    /**
     * @brief 获取皮肤下载URL
     */
    [[nodiscard]] const std::optional<std::string>& skinUrl() const noexcept { return m_skinUrl; }

    /**
     * @brief 获取披风下载URL
     */
    [[nodiscard]] const std::optional<std::string>& capeUrl() const noexcept { return m_capeUrl; }

    /**
     * @brief 获取鞘翅下载URL
     */
    [[nodiscard]] const std::optional<std::string>& elytraUrl() const noexcept { return m_elytraUrl; }

    /**
     * @brief 设置皮肤下载URL
     */
    void setSkinUrl(const std::string& url) { m_skinUrl = url; }

    /**
     * @brief 设置披风下载URL
     */
    void setCapeUrl(const std::string& url) { m_capeUrl = url; }

    /**
     * @brief 设置鞘翅下载URL
     */
    void setElytraUrl(const std::string& url) { m_elytraUrl = url; }

    // ========== 哈希获取（用于缓存） ==========

    /**
     * @brief 获取皮肤哈希
     */
    [[nodiscard]] const std::optional<std::string>& skinHash() const noexcept { return m_skinHash; }

    /**
     * @brief 获取披风哈希
     */
    [[nodiscard]] const std::optional<std::string>& capeHash() const noexcept { return m_capeHash; }

    /**
     * @brief 获取鞘翅哈希
     */
    [[nodiscard]] const std::optional<std::string>& elytraHash() const noexcept { return m_elytraHash; }

    /**
     * @brief 设置皮肤哈希
     */
    void setSkinHash(const std::string& hash) { m_skinHash = hash; }

    /**
     * @brief 设置披风哈希
     */
    void setCapeHash(const std::string& hash) { m_capeHash = hash; }

    /**
     * @brief 设置鞘翅哈希
     */
    void setElytraHash(const std::string& hash) { m_elytraHash = hash; }

    // ========== 状态检查 ==========

    /**
     * @brief 是否有皮肤
     */
    [[nodiscard]] bool hasSkin() const noexcept { return m_skin.has_value(); }

    /**
     * @brief 是否有披风
     */
    [[nodiscard]] bool hasCape() const noexcept { return m_cape.has_value(); }

    /**
     * @brief 是否有鞘翅
     */
    [[nodiscard]] bool hasElytra() const noexcept { return m_elytra.has_value(); }

    /**
     * @brief 是否有任何纹理
     */
    [[nodiscard]] bool hasAnyTexture() const noexcept { return hasSkin() || hasCape() || hasElytra(); }

    // ========== 缓存键生成 ==========

    /**
     * @brief 获取皮肤缓存键
     *
     * 格式：skins/<hash前2字符>/<完整hash>
     * 用于 ResourceLocation 和本地缓存路径。
     *
     * @return 缓存键字符串，如果没有皮肤hash返回空
     */
    [[nodiscard]] std::string getSkinCacheKey() const;

    /**
     * @brief 获取披风缓存键
     */
    [[nodiscard]] std::string getCapeCacheKey() const;

    /**
     * @brief 获取鞘翅缓存键
     */
    [[nodiscard]] std::string getElytraCacheKey() const;

    // ========== 从URL提取哈希 ==========

    /**
     * @brief 从皮肤URL提取哈希
     * @return 哈希字符串，提取失败返回空
     */
    [[nodiscard]] static std::string extractHashFromUrl(const std::string& url);

private:
    std::optional<ResourceLocation> m_skin;
    std::optional<ResourceLocation> m_cape;
    std::optional<ResourceLocation> m_elytra;

    std::optional<std::string> m_skinUrl;
    std::optional<std::string> m_capeUrl;
    std::optional<std::string> m_elytraUrl;

    std::optional<std::string> m_skinHash;
    std::optional<std::string> m_capeHash;
    std::optional<std::string> m_elytraHash;

    SkinType m_skinType = SkinType::Default;
};

} // namespace mc::skin
