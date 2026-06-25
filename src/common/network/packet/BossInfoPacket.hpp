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

#include "../../command/ICommandSource.hpp" // for Uuid
#include "../../core/Types.hpp"
#include "Packet.hpp"
#include <memory>
#include <string>

namespace mc {

namespace text {
class ITextComponent;
}

namespace network {

/**
 * @brief Boss 栏操作类型
 *
 * 定义 Boss 栏同步包的操作类型。
 * 参考 MC 1.16.5: net.minecraft.network.play.server.SUpdateBossInfoPacket.Operation
 */
enum class BossInfoAction : u8 {
    Add = 0,             // 添加 Boss 栏
    Remove = 1,          // 移除 Boss 栏
    UpdatePercent = 2,   // 更新百分比
    UpdateName = 3,      // 更新名称
    UpdateStyle = 4,     // 更新样式（颜色和边框）
    UpdateProperties = 5 // 更新属性标志
};

/**
 * @brief Boss 栏同步包
 *
 * 用于服务端向客户端同步 Boss 栏状态，支持：
 * - 添加新的 Boss 栏
 * - 移除 Boss 栏
 * - 更新百分比
 * - 更新名称
 * - 更新样式（颜色和边框）
 * - 更新属性标志（变暗天空、播放音乐、创建迷雾）
 *
 * 参考 MC 1.16.5: net.minecraft.network.play.server.SUpdateBossInfoPacket
 */
class BossInfoPacket : public Packet {
public:
    BossInfoPacket();
    ~BossInfoPacket() override = default;

    // ========================================================================
    // 静态工厂方法
    // ========================================================================

    /**
     * @brief 创建添加 Boss 栏包
     *
     * @param uuid Boss 栏唯一标识符
     * @param name 显示名称
     * @param percent 百分比 (0.0 ~ 1.0)
     * @param color 颜色
     * @param overlay 样式
     * @param darkenSky 是否变暗天空
     * @param playEndBossMusic 是否播放 Boss 音乐
     * @param createFog 是否创建迷雾
     */
    static BossInfoPacket add(const Uuid& uuid,
        std::unique_ptr<text::ITextComponent> name,
        f32 percent,
        u8 color,
        u8 overlay,
        bool darkenSky,
        bool playEndBossMusic,
        bool createFog);

    /**
     * @brief 创建移除 Boss 栏包
     *
     * @param uuid Boss 栏唯一标识符
     */
    static BossInfoPacket remove(const Uuid& uuid);

    /**
     * @brief 创建更新百分比包
     *
     * @param uuid Boss 栏唯一标识符
     * @param percent 新的百分比 (0.0 ~ 1.0)
     */
    static BossInfoPacket updatePercent(const Uuid& uuid, f32 percent);

    /**
     * @brief 创建更新名称包
     *
     * @param uuid Boss 栏唯一标识符
     * @param name 新的显示名称
     */
    static BossInfoPacket updateName(const Uuid& uuid, std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 创建更新样式包
     *
     * @param uuid Boss 栏唯一标识符
     * @param color 新的颜色
     * @param overlay 新的样式
     */
    static BossInfoPacket updateStyle(const Uuid& uuid, u8 color, u8 overlay);

    /**
     * @brief 创建更新属性标志包
     *
     * @param uuid Boss 栏唯一标识符
     * @param darkenSky 是否变暗天空
     * @param playEndBossMusic 是否播放 Boss 音乐
     * @param createFog 是否创建迷雾
     */
    static BossInfoPacket updateProperties(const Uuid& uuid, bool darkenSky, bool playEndBossMusic, bool createFog);

    // ========================================================================
    // 序列化
    // ========================================================================

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========================================================================
    // 访问器
    // ========================================================================

    [[nodiscard]] BossInfoAction action() const noexcept { return m_action; }
    [[nodiscard]] const Uuid& uuid() const noexcept { return m_uuid; }
    [[nodiscard]] f32 percent() const noexcept { return m_percent; }
    [[nodiscard]] u8 color() const noexcept { return m_color; }
    [[nodiscard]] u8 overlay() const noexcept { return m_overlay; }
    [[nodiscard]] bool darkenSky() const noexcept { return m_darkenSky; }
    [[nodiscard]] bool playEndBossMusic() const noexcept { return m_playEndBossMusic; }
    [[nodiscard]] bool createFog() const noexcept { return m_createFog; }

    /**
     * @brief 获取名称（JSON 格式）
     *
     * 仅在 UpdateName 和 Add 操作后有效。
     */
    [[nodiscard]] const std::string& nameJson() const noexcept { return m_nameJson; }

private:
    explicit BossInfoPacket(BossInfoAction action);

    /**
     * @brief 将 ITextComponent 序列化为 JSON 字符串
     */
    static std::string _serializeName(const text::ITextComponent& name);

    BossInfoAction m_action = BossInfoAction::Add;

    // 所有操作共享的字段（128 位 UUID，与 MC 协议一致）
    Uuid m_uuid{};

    // Add/UpdateName 操作使用的字段
    std::string m_nameJson;

    // Add/UpdatePercent 操作使用的字段
    f32 m_percent = 1.0f;

    // Add/UpdateStyle 操作使用的字段
    u8 m_color = 0;   // BossInfoColor
    u8 m_overlay = 0; // BossInfoOverlay

    // Add/UpdateProperties 操作使用的字段
    bool m_darkenSky = false;
    bool m_playEndBossMusic = false;
    bool m_createFog = false;
};

} // namespace network
} // namespace mc
