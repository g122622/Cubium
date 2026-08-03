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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::skin {

/**
 * @brief 玩家档案属性
 *
 * 用于存储皮肤、披风等纹理URL信息。
 * 通常包含 "textures" 属性，值为 Base64 编码的 JSON。
 *
 * 示例：
 * - name: "textures"
 * - value: Base64编码的JSON，包含皮肤URL等信息
 * - signature: 可选的签名，用于验证属性真实性
 */
struct GameProfileProperty {
    std::string name;                     // 属性名，如 "textures"
    std::string value;                    // Base64 编码的值
    std::optional<std::string> signature; // 可选的签名（用于验证）

    GameProfileProperty() noexcept = default;

    /**
     * @brief 构造属性
     * @param n 属性名
     * @param v 属性值（Base64编码）
     * @param sig 可选的签名
     */
    GameProfileProperty(
        const std::string& n, const std::string& v, const std::optional<std::string>& sig = std::nullopt)
        : name(n)
        , value(v)
        , signature(sig)
    {}

    /**
     * @brief 检查是否有签名
     */
    [[nodiscard]] bool hasSignature() const noexcept { return signature.has_value(); }
};

/**
 * @brief 玩家游戏档案
 *
 * 存储玩家的唯一标识信息，用于：
 * - 皮肤加载（通过UUID查找皮肤）
 * - 网络同步（玩家列表、实体生成）
 * - 离线/在线模式区分
 *
 * 示例：
 * @code
 * GameProfile profile;
 * profile.setUUID(uuid);
 * profile.setName("PlayerName");
 * profile.addProperty({"textures", base64Data, signature});
 *
 * // 获取皮肤属性
 * if (auto* textures = profile.getTexturesProperty()) {
 *     // 解析 Base64 JSON 获取皮肤 URL
 * }
 * @endcode
 */
class GameProfile {
public:
    GameProfile() noexcept = default;

    /**
     * @brief 构造档案
     * @param uuid 玩家UUID
     * @param name 玩家名称
     */
    GameProfile(const std::array<u8, 16>& uuid, const std::string& name);

    // ========== 基本信息 ==========

    /**
     * @brief 获取UUID
     * @return 16字节UUID数组
     */
    [[nodiscard]] const std::array<u8, 16>& uuid() const noexcept { return m_uuid; }

    /**
     * @brief 设置UUID
     */
    void setUUID(const std::array<u8, 16>& uuid) { m_uuid = uuid; }

    /**
     * @brief 获取玩家名称
     */
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

    /**
     * @brief 设置玩家名称
     */
    void setName(const std::string& name) { m_name = name; }

    // ========== 属性管理 ==========

    /**
     * @brief 获取所有属性
     */
    [[nodiscard]] const std::vector<GameProfileProperty>& properties() const noexcept { return m_properties; }

    /**
     * @brief 添加属性
     */
    void addProperty(const GameProfileProperty& property);

    /**
     * @brief 添加属性（移动语义）
     */
    void addProperty(GameProfileProperty&& property);

    /**
     * @brief 获取指定名称的属性
     * @param name 属性名
     * @return 属性指针，不存在返回 nullptr
     */
    [[nodiscard]] const GameProfileProperty* getProperty(const std::string& name) const;

    /**
     * @brief 检查是否有 textures 属性
     */
    [[nodiscard]] bool hasTextures() const noexcept;

    /**
     * @brief 获取 textures 属性
     * @return textures 属性指针，不存在返回 nullptr
     */
    [[nodiscard]] const GameProfileProperty* getTexturesProperty() const;

    /**
     * @brief 清除所有属性
     */
    void clearProperties() noexcept { m_properties.clear(); }

    // ========== UUID 工具 ==========

    /**
     * @brief 获取 UUID 的字符串表示（带连字符）
     * @return 如 "550e8400-e29b-41d4-a716-446655440000"
     */
    [[nodiscard]] std::string uuidToString() const;

    /**
     * @brief 获取 UUID 的无连字符字符串表示
     * @return 如 "550e8400e29b41d4a716446655440000"
     */
    [[nodiscard]] std::string uuidToStringNoDashes() const;

    /**
     * @brief 从字符串解析 UUID
     * @param str UUID字符串（带或不带连字符）
     * @return UUID数组，解析失败返回全零
     *
     * 支持格式：
     * - "550e8400-e29b-41d4-a716-446655440000" (带连字符)
     * - "550e8400e29b41d4a716446655440000" (无连字符)
     */
    [[nodiscard]] static std::array<u8, 16> parseUUID(const std::string& str);

    /**
     * @brief 计算 UUID 哈希值（用于默认皮肤类型确定）
     * @return 32位哈希值
     */
    [[nodiscard]] i32 uuidHashCode() const noexcept;

    /**
     * @brief 检查 UUID 是否有效（非全零）
     */
    [[nodiscard]] bool hasValidUUID() const noexcept;

    // ========== 比较操作 ==========

    /**
     * @brief 相等比较（基于UUID）
     */
    bool operator==(const GameProfile& other) const noexcept { return m_uuid == other.m_uuid; }
    bool operator!=(const GameProfile& other) const noexcept { return m_uuid != other.m_uuid; }

    // ========== JSON 序列化（用于 ItemStack NBT）==========

    /**
     * @brief 序列化为 JSON 对象（用于 SkullOwner NBT 标签）
     *
     * 输出格式：
     * {
     *   "Name": "PlayerName",
     *   "Id": "550e8400-e29b-41d4-a716-446655440000",
     *   "Properties": {
     *     "textures": [{"Value": "base64...", "Signature": "..."}]
     *   }
     * }
     *
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 对象反序列化
     * @param json JSON 对象
     * @return GameProfile 对象，失败返回错误
     */
    [[nodiscard]] static Result<GameProfile> fromJson(const nlohmann::json& json);

private:
    std::array<u8, 16> m_uuid = {};                // 玩家UUID（big-endian）
    std::string m_name;                            // 玩家名称
    std::vector<GameProfileProperty> m_properties; // 属性列表
};

} // namespace mc::skin
