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
#include <array>
#include <string>

namespace mc::skin {

/**
 * @brief 皮肤模型类型
 *
 * 决定玩家模型的手臂宽度：
 * - Default (Steve): 4像素宽手臂
 * - Slim (Alex): 3像素宽手臂
 *
 * 参考 MC 1.16.5 PlayerModel 构造函数
 */
enum class SkinType : u8 {
    Default = 0, // 宽手臂 (4px)，Steve 模型
    Slim = 1     // 窄手臂 (3px)，Alex 模型
};

/**
 * @brief 将字符串转换为皮肤类型
 *
 * @param typeStr 类型字符串 ("default" 或 "slim")
 * @return 皮肤类型，无法识别时返回 Default
 *
 * @note MC 1.16.5 中皮肤元数据 JSON 的 "model" 字段使用此格式
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
 * 使用 MC 1.16.5 的算法：
 * - 计算 UUID 的 hashCode
 * - 如果 (hashCode & 1) == 1，则为 Slim (Alex)
 * - 否则为 Default (Steve)
 *
 * @param uuid 玩家UUID（16字节，big-endian）
 * @return 默认皮肤类型
 *
 * @note 这确保了即使没有自定义皮肤，
 *       玩家也会有正确的 Steve/Alex 皮肤
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
