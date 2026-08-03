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
#include "common/resource/ResourceLocation.hpp"
#include <array>
#include <cstddef>
#include <string>

namespace mc::skin {

/**
 * @brief 纹理属性签名状态
 *
 * 与 MC Java 版 authlib 的 SignatureState 对应。
 * 用于指示 textures 属性签名的验证结果。
 *
 * - Unsigned: 属性没有签名（离线模式或会话服务未提供签名）
 * - Invalid:  有签名但验证失败（签名被篡改或使用了未知公钥）
 * - Signed:   有签名且验证通过（属性来源可信）
 */
enum class SignatureState : u8 {
    Unsigned = 0, // 无签名
    Invalid = 1,  // 签名验证失败
    Signed = 2    // 签名验证通过
};

/**
 * @brief 皮肤模型类型
 *
 * 决定玩家模型的手臂宽度：
 * - Default (Steve): 4像素宽手臂
 * - Slim (Alex): 3像素宽手臂
 */
enum class SkinType : u8 {
    Default = 0, // 宽手臂 (4px)，Steve 模型
    Slim = 1     // 窄手臂 (3px)，Alex 模型
};

/**
 * @brief 默认皮肤变体
 *
 * MC 1.21.1 有 18 种默认皮肤（9 slim + 9 wide），通过 UUID 哈希选择。
 * 变体名称：alex, ari, efe, kai, makena, noor, steve, sunny, zuri
 * 每个名称有 slim 和 wide 两种手臂类型。
 *
 * 数组索引与 MC 源码 DefaultPlayerSkin.DEFAULT_SKINS 一致：
 *   0-8:  slim (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
 *   9-17: wide (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
 */
struct DefaultSkinVariant {
    /// 皮肤名称（如 "alex", "steve"）
    const char* name;
    /// 皮肤模型类型
    SkinType skinType;
    /// 在 DEFAULT_SKINS 数组中的索引 (0-17)
    u8 index;

    /**
     * @brief 获取此变体的 ResourceLocation
     * @return 纹理路径，如 "minecraft:textures/entity/player/slim/steve.png"
     */
    [[nodiscard]] ResourceLocation textureLocation() const
    {
        const char* typeDir = (skinType == SkinType::Slim) ? "slim" : "wide";
        return ResourceLocation(std::string("minecraft:textures/entity/player/") + typeDir + "/" + name + ".png");
    }
};

/// 默认皮肤变体总数
constexpr size_t DEFAULT_SKIN_COUNT = 18;

/**
 * @brief 获取所有默认皮肤变体（按 MC 源码顺序）
 *
 * 索引 0-8: slim 变体
 * 索引 9-17: wide 变体
 *
 * @return 包含 18 个默认皮肤变体的数组
 */
[[nodiscard]] const std::array<DefaultSkinVariant, DEFAULT_SKIN_COUNT>& getDefaultSkinVariants();

/**
 * @brief 根据 UUID 获取默认皮肤变体
 *
 * 使用与 MC Java 版 DefaultPlayerSkin.get(UUID) 相同的算法：
 * index = Math.floorMod(uuid.hashCode(), 18)
 *
 * @param uuid 玩家UUID（16字节，big-endian）
 * @return 默认皮肤变体
 */
[[nodiscard]] const DefaultSkinVariant& getDefaultSkinVariantForUUID(const std::array<u8, 16>& uuid);

/**
 * @brief 获取规范默认皮肤（无 UUID 上下文时的回退）
 *
 * 返回 slim/steve（索引 6），与 MC 源码 DefaultPlayerSkin.getDefaultSkin() 一致。
 *
 * @return 默认皮肤变体
 */
[[nodiscard]] const DefaultSkinVariant& getCanonicalDefaultSkin();

/**
 * @brief 将字符串转换为皮肤类型
 *
 * @param typeStr 类型字符串 ("default" 或 "slim")
 * @return 皮肤类型，无法识别时返回 Default
 */
[[nodiscard]] SkinType parseSkinType(const std::string& typeStr);

/**
 * @brief 将皮肤类型转换为字符串
 *
 * @param type 皮肤类型
 * @return "default" 或 "slim"
 */
[[nodiscard]] std::string skinTypeToString(SkinType type);

/**
 * @brief 根据UUID确定默认皮肤类型
 *
 * 基于 UUID 哈希值从 18 种默认皮肤中选择，返回所选变体的皮肤类型。
 * 内部委托给 getDefaultSkinVariantForUUID()，确保与 18 皮肤选择算法一致。
 *
 * @param uuid 玩家UUID（16字节，big-endian）
 * @return 默认皮肤类型
 */
[[nodiscard]] SkinType getDefaultSkinTypeForUUID(const std::array<u8, 16>& uuid);

/**
 * @brief 计算UUID的哈希码
 *
 * 使用与 Java UUID.hashCode() 相同的算法：
 * int hashCode = (int)(mostSigBits >> 32) ^ (int)mostSigBits ^
 *                (int)(leastSigBits >> 32) ^ (int)leastSigBits;
 *
 * @param uuid UUID 字节数组
 * @return 32位哈希码
 */
[[nodiscard]] i32 calculateUUIDHashCode(const std::array<u8, 16>& uuid);

} // namespace mc::skin
