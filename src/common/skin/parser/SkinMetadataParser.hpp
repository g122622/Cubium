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
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <string>
#include <vector>

namespace mc::skin {

/**
 * @brief 皮肤元数据解析器
 *
 * 解析 GameProfileProperty 中的 textures 属性。
 * textures 属性值是 Base64 编码的 JSON，格式如下：
 *
 * @code
 * {
 *   "textures": {
 *     "SKIN": {
 *       "url": "http://textures.minecraft.net/texture/...",
 *       "metadata": {
 *         "model": "slim"
 *       }
 *     },
 *     "CAPE": {
 *       "url": "http://textures.minecraft.net/texture/..."
 *     },
 *     "ELYTRA": {
 *       "url": "http://textures.minecraft.net/texture/..."
 *     }
 *   }
 * }
 * @endcode
 *
 * 签名验证遵循 MC Java 版 authlib YggdrasilServicesKeyInfo 的规范：
 * - 算法: SHA1withRSA (4096-bit RSA)
 * - 验证数据: property.value 的原始 ASCII 字节（不是 Base64 解码后的内容）
 * - 签名: property.signature 的 Base64 解码字节
 * - 公钥: 从 https://api.minecraftservices.com/publickeys 获取的 X509 编码 RSA 公钥
 */
class SkinMetadataParser {
public:
    /**
     * @brief 解析 textures 属性
     *
     * @param property textures 属性
     * @return 解析出的皮肤纹理集合
     */
    [[nodiscard]] static Result<SkinTextures> parse(const GameProfileProperty& property);

    /**
     * @brief 解析 Base64 编码的 JSON
     *
     * @param base64Data Base64 编码的 JSON 数据
     * @return 解析出的皮肤纹理集合
     */
    [[nodiscard]] static Result<SkinTextures> parseBase64(const std::string& base64Data);

    /**
     * @brief 解析 JSON 字符串
     *
     * @param jsonData JSON 字符串
     * @return 解析出的皮肤纹理集合
     */
    [[nodiscard]] static Result<SkinTextures> parseJson(const std::string& jsonData);

    /**
     * @brief 验证 textures 属性签名并返回签名状态
     *
     * 遵循 MC Java 版 authlib 的签名验证流程：
     * 1. 无签名 → UNSIGNED
     * 2. 有签名但无法验证（缺少加密库支持或公钥未加载）→ UNSIGNED（降级处理）
     * 3. 有签名且验证失败 → INVALID
     * 4. 有签名且验证通过 → SIGNED
     *
     * @param property textures 属性
     * @return 签名状态
     */
    [[nodiscard]] static SignatureState getSignatureState(const GameProfileProperty& property);

    /**
     * @brief 验证签名
     *
     * 便捷方法：调用 getSignatureState 并返回签名是否有效。
     * UNSIGNED 和 SIGNED 状态下返回 true，INVALID 返回 false。
     *
     * @param property textures 属性
     * @return 签名是否有效（UNSIGNED 视为有效）
     */
    [[nodiscard]] static bool verifySignature(const GameProfileProperty& property);

private:
    /**
     * @brief 解析单个纹理信息
     *
     * 从 JSON 纹理对象中提取 URL、哈希和可选元数据（如模型类型），
     * 并设置到 SkinTextures 中。
     *
     * @param textureObj JSON 纹理对象（nlohmann::json 对象）
     * @param type 纹理类型（"SKIN", "CAPE", "ELYTRA"）
     * @param textures 输出的皮肤纹理集合
     */
    static void _parseTexture(const void* textureObj, const std::string& type, SkinTextures& textures);
};

} // namespace mc::skin
