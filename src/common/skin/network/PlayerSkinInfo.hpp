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

#include "common/core/Types.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <atomic>
#include <optional>
#include <string>

namespace mc::skin {

/**
 * @brief 皮肤加载状态
 */
enum class SkinLoadState : u8 {
    NotLoaded,   // 未加载
    Loading,     // 加载中
    Loaded,      // 已加载
    Failed,      // 加载失败
    UsingDefault // 使用默认皮肤
};

/**
 * @brief 客户端玩家皮肤信息
 *
 * 存储单个玩家的皮肤相关信息，包括：
 * - 纹理资源位置
 * - 皮肤类型
 * - 加载状态
 * - 皮肤部件可见性
 *
 * 线程安全：加载状态使用 atomic，纹理访问需要外部同步。
 */
class PlayerSkinInfo {
public:
    /**
     * @brief 构造玩家皮肤信息
     * @param profile 玩家档案
     */
    explicit PlayerSkinInfo(const GameProfile& profile);

    // ========== 基本信息 ==========

    /**
     * @brief 获取玩家档案
     */
    [[nodiscard]] const GameProfile& profile() const noexcept { return m_profile; }

    /**
     * @brief 获取玩家UUID
     */
    [[nodiscard]] const std::array<u8, 16>& uuid() const noexcept { return m_profile.uuid(); }

    /**
     * @brief 获取玩家名称
     */
    [[nodiscard]] const std::string& name() const noexcept { return m_profile.name(); }

    // ========== 皮肤纹理 ==========

    /**
     * @brief 获取皮肤 ResourceLocation
     *
     * 如果皮肤已加载，返回实际皮肤位置；
     * 否则返回默认皮肤。
     */
    [[nodiscard]] ResourceLocation getSkinLocation() const;

    /**
     * @brief 获取披风 ResourceLocation
     * @return 披风位置，无披风返回空
     */
    [[nodiscard]] std::optional<ResourceLocation> getCapeLocation() const;

    /**
     * @brief 获取鞘翅 ResourceLocation
     * @return 鞘翅位置，无鞘翅返回空
     */
    [[nodiscard]] std::optional<ResourceLocation> getElytraLocation() const;

    /**
     * @brief 获取皮肤类型
     *
     * 优先使用已加载皮肤的类型；
     * 未加载时根据 UUID 确定默认类型。
     */
    [[nodiscard]] SkinType getSkinType() const;

    /**
     * @brief 获取皮肤纹理集合
     */
    [[nodiscard]] const SkinTextures& textures() const noexcept { return m_textures; }

    // ========== 加载状态 ==========

    [[nodiscard]] SkinLoadState loadState() const noexcept { return m_loadState.load(); }
    void setLoadState(SkinLoadState state) noexcept { m_loadState.store(state); }

    [[nodiscard]] bool isLoaded() const noexcept { return m_loadState == SkinLoadState::Loaded; }
    [[nodiscard]] bool isLoading() const noexcept { return m_loadState == SkinLoadState::Loading; }
    [[nodiscard]] bool isFailed() const noexcept { return m_loadState == SkinLoadState::Failed; }
    [[nodiscard]] bool isUsingDefault() const noexcept { return m_loadState == SkinLoadState::UsingDefault; }

    // ========== 纹理设置（由 SkinManager 调用） ==========

    /**
     * @brief 设置皮肤纹理集合
     */
    void setSkinTextures(const SkinTextures& textures);

    /**
     * @brief 设置皮肤位置
     */
    void setSkinLocation(const ResourceLocation& location);

    /**
     * @brief 设置皮肤类型
     */
    void setSkinType(SkinType type) noexcept { m_textures.setSkinType(type); }

    /**
     * @brief 设置披风位置
     */
    void setCapeLocation(const ResourceLocation& location);

    /**
     * @brief 设置鞘翅位置
     */
    void setElytraLocation(const ResourceLocation& location);

    // ========== 部件可见性 ==========

    /**
     * @brief 获取皮肤部件可见性掩码
     *
     * 位掩码，参考 PlayerModelPart 枚举：
     * - Bit 0: Cape (披风)
     * - Bit 1: Jacket (外套)
     * - Bit 2: Left Sleeve (左袖)
     * - Bit 3: Right Sleeve (右袖)
     * - Bit 4: Left Pants Leg (左裤腿)
     * - Bit 5: Right Pants Leg (右裤腿)
     * - Bit 6: Hat (帽子)
     */
    [[nodiscard]] u8 modelParts() const noexcept { return m_modelParts; }

    /**
     * @brief 设置皮肤部件可见性掩码
     */
    void setModelParts(u8 parts) noexcept { m_modelParts = parts; }

    /**
     * @brief 检查是否穿着指定部件
     * @param part 部件类型
     */
    [[nodiscard]] bool isWearing(PlayerModelPart part) const noexcept;

    /**
     * @brief 设置指定部件的可见性
     * @param part 部件类型
     * @param enabled 是否可见
     */
    void setModelPartEnabled(PlayerModelPart part, bool enabled) noexcept;

    // ========== 默认皮肤 ==========

    /**
     * @brief 获取默认皮肤位置
     *
     * 根据 UUID 哈希从 18 种默认皮肤中选择，
     * 与 MC Java 版 DefaultPlayerSkin.get(UUID) 一致。
     */
    [[nodiscard]] ResourceLocation getDefaultSkinLocation() const;

    // ========== 签名安全 ==========

    /**
     * @brief 皮肤是否安全（签名验证通过）
     *
     * 对应 MC Java 版 PlayerSkin.secure 字段。
     * 当 textures 属性签名状态为 SIGNED 时为 true。
     * UNSIGNED（离线模式）视为不安全但允许使用。
     * INVALID（签名验证失败）视为不安全。
     *
     * 非本地玩家且 secure=false 时，MC 客户端会回退到默认皮肤。
     */
    [[nodiscard]] bool isSecure() const noexcept { return m_secure; }

    /**
     * @brief 设置皮肤安全状态
     * @param secure 是否安全
     */
    void setSecure(bool secure) noexcept { m_secure = secure; }

    /**
     * @brief 获取签名状态
     * @return 签名状态
     */
    [[nodiscard]] SignatureState signatureState() const noexcept { return m_signatureState; }

    /**
     * @brief 设置签名状态
     * @param state 签名状态
     */
    void setSignatureState(SignatureState state) noexcept
    {
        m_signatureState = state;
        m_secure = (state == SignatureState::Signed);
    }

private:
    GameProfile m_profile;                                            // 玩家档案
    SkinTextures m_textures;                                          // 皮肤纹理集合
    std::atomic<SkinLoadState> m_loadState{SkinLoadState::NotLoaded}; // 加载状态
    u8 m_modelParts = 0x7F;                                           // 默认显示所有部件（除披风外，Bit 0 = 0）
    bool m_secure = false;                                            // 皮肤是否安全（签名验证通过）
    SignatureState m_signatureState{SignatureState::Unsigned};        // 签名状态
};

} // namespace mc::skin
