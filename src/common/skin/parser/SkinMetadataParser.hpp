#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
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
 *     }
 *   }
 * }
 * @endcode
 *
 * 参考 MC 1.16.5 MinecraftProfileTexture
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
     * @brief 验证签名
     *
     * 验证 textures 属性的签名是否有效。
     * 注意：此方法需要 Mojang 的公钥，简化实现中可能跳过验证。
     *
     * @param property textures 属性
     * @return 签名是否有效
     */
    [[nodiscard]] static bool verifySignature(const GameProfileProperty& property);

private:
    /**
     * @brief 解析单个纹理信息
     *
     * @param textureObj JSON 纹理对象
     * @param type 纹理类型（"SKIN", "CAPE", "ELYTRA"）
     * @param textures 输出的皮肤纹理集合
     */
    static void parseTexture(const void* textureObj, const std::string& type, SkinTextures& textures);
};

} // namespace mc::skin
