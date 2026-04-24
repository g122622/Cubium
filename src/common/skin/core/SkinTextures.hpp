#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "SkinTypes.hpp"
#include <optional>
#include <string>

namespace mc::skin {

/**
 * @brief 皮肤纹理类型
 *
 * 参考 MC 1.16.5 MinecraftProfileTexture.Type
 */
enum class TextureType : u8 {
    Skin,    // 皮肤
    Cape,    // 披风
    Elytra   // 鞘翅
};

/**
 * @brief 单个纹理的元数据
 *
 * 从 textures 属性的 Base64 JSON 中解析。
 */
struct TextureMetadata {
    String url;                          // 下载URL
    std::optional<String> hash;          // 纹理哈希（用于缓存文件名）
    std::optional<SkinType> skinType;    // 仅皮肤：模型类型（default/slim）

    TextureMetadata() = default;

    /**
     * @brief 是否有效（必须有URL）
     */
    [[nodiscard]] bool isValid() const { return !url.empty(); }
};

/**
 * @brief 皮肤纹理集合
 *
 * 存储玩家的所有纹理资源定位信息。
 * 从 GameProfileProperty 的 Base64 JSON 解析而来。
 *
 * 参考 MC 1.16.5 PlayerTextures
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

    // ========== 纹理获取 ==========

    /**
     * @brief 获取皮肤 ResourceLocation
     * @return 皮肤位置，不存在返回空optional
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getSkin() const { return m_skin; }

    /**
     * @brief 获取披风 ResourceLocation
     * @return 披风位置，不存在返回空optional
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getCape() const { return m_cape; }

    /**
     * @brief 获取鞘翅 ResourceLocation
     * @return 鞘翅位置，不存在返回空optional
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getElytra() const { return m_elytra; }

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] SkinType skinType() const { return m_skinType; }

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
    void setSkinType(SkinType type) { m_skinType = type; }

    // ========== URL 获取（用于下载） ==========

    [[nodiscard]] const std::optional<String>& skinUrl() const { return m_skinUrl; }
    [[nodiscard]] const std::optional<String>& capeUrl() const { return m_capeUrl; }
    [[nodiscard]] const std::optional<String>& elytraUrl() const { return m_elytraUrl; }

    void setSkinUrl(const String& url) { m_skinUrl = url; }
    void setCapeUrl(const String& url) { m_capeUrl = url; }
    void setElytraUrl(const String& url) { m_elytraUrl = url; }

    // ========== 哈希获取（用于缓存） ==========

    [[nodiscard]] const std::optional<String>& skinHash() const { return m_skinHash; }
    [[nodiscard]] const std::optional<String>& capeHash() const { return m_capeHash; }
    [[nodiscard]] const std::optional<String>& elytraHash() const { return m_elytraHash; }

    void setSkinHash(const String& hash) { m_skinHash = hash; }
    void setCapeHash(const String& hash) { m_capeHash = hash; }
    void setElytraHash(const String& hash) { m_elytraHash = hash; }

    // ========== 状态检查 ==========

    [[nodiscard]] bool hasSkin() const { return m_skin.has_value(); }
    [[nodiscard]] bool hasCape() const { return m_cape.has_value(); }
    [[nodiscard]] bool hasElytra() const { return m_elytra.has_value(); }

    /**
     * @brief 是否有任何纹理
     */
    [[nodiscard]] bool hasAnyTexture() const {
        return hasSkin() || hasCape() || hasElytra();
    }

    // ========== 缓存键生成 ==========

    /**
     * @brief 获取皮肤缓存键
     *
     * 格式：skins/<hash前2字符>/<完整hash>
     * 用于 ResourceLocation 和本地缓存路径。
     *
     * @return 缓存键字符串，如果没有皮肤hash返回空
     */
    [[nodiscard]] String getSkinCacheKey() const;

    /**
     * @brief 获取披风缓存键
     */
    [[nodiscard]] String getCapeCacheKey() const;

    /**
     * @brief 获取鞘翅缓存键
     */
    [[nodiscard]] String getElytraCacheKey() const;

    // ========== 从URL提取哈希 ==========

    /**
     * @brief 从皮肤URL提取哈希
     *
     * Mojang 皮肤URL格式：http://textures.minecraft.net/texture/<hash>
     *
     * @return 哈希字符串，提取失败返回空
     */
    [[nodiscard]] static String extractHashFromUrl(const String& url);

private:
    std::optional<ResourceLocation> m_skin;
    std::optional<ResourceLocation> m_cape;
    std::optional<ResourceLocation> m_elytra;

    std::optional<String> m_skinUrl;
    std::optional<String> m_capeUrl;
    std::optional<String> m_elytraUrl;

    std::optional<String> m_skinHash;
    std::optional<String> m_capeHash;
    std::optional<String> m_elytraHash;

    SkinType m_skinType = SkinType::Default;
};

} // namespace mc::skin
