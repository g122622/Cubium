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

#include "SkinMetadataParser.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::skin {

namespace {

/**
 * @brief Base64 解码
 */
std::vector<u8> base64Decode(const std::string& encoded)
{
    static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<u8> decoded;

    if (encoded.empty()) {
        return decoded;
    }

    // 移除空白字符
    std::string cleanEncoded;
    for (char c : encoded) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            cleanEncoded.push_back(c);
        }
    }

    if (cleanEncoded.length() % 4 != 0) {
        spdlog::warn("SkinMetadataParser: Invalid base64 length: {}", cleanEncoded.length());
        return decoded;
    }

    size_t padding = 0;
    if (cleanEncoded.length() >= 1 && cleanEncoded.back() == '=') {
        padding++;
        if (cleanEncoded.length() >= 2 && cleanEncoded[cleanEncoded.length() - 2] == '=') {
            padding++;
        }
    }

    decoded.reserve((cleanEncoded.length() / 4) * 3 - padding);

    u32 buffer = 0;
    i32 bits = 0;

    for (size_t i = 0; i < cleanEncoded.length(); ++i) {
        char c = cleanEncoded[i];

        if (c == '=') {
            buffer <<= 6;
            bits += 6;
            continue;
        }

        size_t pos = base64Chars.find(c);
        if (pos == std::string::npos) {
            spdlog::warn("SkinMetadataParser: Invalid base64 character: {}", c);
            // 遇到非法字符时整体解码失败（返回空），与"Invalid base64 length"路径
            // 的失败语义保持一致。此前实现仅 continue 跳过非法字符，会导致
            // 形如 "!!!invalid-base64!!!" 的恶意/损坏签名被部分解码成非空字节，
            // 进而使 getSignatureState 误判为可验证签名（Unsigned）而非 INVALID。
            decoded.clear();
            return decoded;
        }

        buffer = (buffer << 6) | static_cast<u32>(pos);
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            decoded.push_back(static_cast<u8>((buffer >> bits) & 0xFF));
        }
    }

    return decoded;
}

/**
 * @brief 从纹理 JSON 对象中提取 URL 和哈希
 *
 * 对应 MC Java 版 MinecraftTexturesPayload 中各纹理类型的解析。
 * 纹理对象格式：
 * @code
 * {
 *   "url": "http://textures.minecraft.net/texture/<hash>",
 *   "metadata": { "model": "slim" }  // 仅 SKIN 类型有
 * }
 * @endcode
 *
 * @param textureObj JSON 纹理对象
 * @param[out] outUrl 输出的 URL
 * @param[out] outHash 输出的哈希
 * @return 是否成功提取到有效 URL
 */
bool extractTextureUrlAndHash(const nlohmann::json& textureObj, std::string& outUrl, std::string& outHash)
{
    if (!textureObj.contains("url") || !textureObj["url"].is_string()) {
        return false;
    }

    outUrl = textureObj["url"].get<std::string>();
    outHash = SkinTextures::extractHashFromUrl(outUrl);
    return !outUrl.empty();
}

} // anonymous namespace

// ============================================================================
// SkinMetadataParser - 公共接口
// ============================================================================

Result<SkinTextures> SkinMetadataParser::parse(const GameProfileProperty& property)
{
    if (property.name != "textures") {
        return Error(ErrorCode::InvalidArgument, "Property is not 'textures': " + property.name);
    }

    return parseBase64(property.value);
}

Result<SkinTextures> SkinMetadataParser::parseBase64(const std::string& base64Data)
{
    // Base64 解码
    auto decoded = base64Decode(base64Data);
    if (decoded.empty()) {
        return Error(ErrorCode::InvalidData, "Failed to decode base64 data");
    }

    // 转换为字符串
    std::string jsonData(decoded.begin(), decoded.end());

    return parseJson(jsonData);
}

Result<SkinTextures> SkinMetadataParser::parseJson(const std::string& jsonData)
{
    SkinTextures textures;

    try {
        auto json = nlohmann::json::parse(jsonData);

        // 检查是否有 textures 对象
        if (!json.contains("textures") || !json["textures"].is_object()) {
            return Error(ErrorCode::InvalidData, "Missing or invalid 'textures' object");
        }

        const auto& texturesObj = json["textures"];

        // 解析 SKIN、CAPE、ELYTRA 三种纹理类型
        _parseTexture(&texturesObj, "SKIN", textures);
        _parseTexture(&texturesObj, "CAPE", textures);
        _parseTexture(&texturesObj, "ELYTRA", textures);
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse textures JSON: ") + e.what());
    }

    return textures;
}

SignatureState SkinMetadataParser::getSignatureState(const GameProfileProperty& property)
{
    // 无签名 → UNSIGNED
    if (!property.hasSignature()) {
        return SignatureState::Unsigned;
    }

    // 签名内容是 property.value 的原始 ASCII 字节（不是 Base64 解码后的内容）
    // 这与 MC Java 版 authlib Property.isSignatureValid() 的行为一致：
    //   sig.update(value.getBytes(StandardCharsets.US_ASCII))
    const std::string& valueBytes = property.value;

    // 签名本身是 Base64 编码的
    auto signatureBytes = base64Decode(property.signature.value());
    if (signatureBytes.empty()) {
        spdlog::warn("SkinMetadataParser: Failed to decode signature base64");
        return SignatureState::Invalid;
    }

    // TODO: RSA-SHA1 签名验证需要加密库支持（OpenSSL 或类似库）。
    // 当前项目未集成加密库，无法执行实际的 RSA 签名验证。
    //
    // 完整的签名验证流程（参考 MC Java 版 authlib YggdrasilServicesKeyInfo）：
    //   1. 从 https://api.minecraftservices.com/publickeys 获取 Mojang 公钥集
    //      响应格式: { "profilePropertyKeys": [{ "publicKey": "<base64 X509>" }] }
    //   2. 将每个公钥从 X509 编码解析为 RSA PublicKey 对象
    //   3. 使用 SHA1withRSA 算法验证签名：
    //      Signature sig = Signature.getInstance("SHA1withRSA");
    //      sig.initVerify(publicKey);
    //      sig.update(property.value.getBytes(US_ASCII));  // 签名数据是 value 原始字节
    //      boolean valid = sig.verify(Base64.decode(property.signature));
    //   4. 任何一个公钥验证通过即为 SIGNED；所有公钥验证失败则为 INVALID
    //
    // 在集成加密库之前，有签名的属性降级为 UNSIGNED（而非 INVALID），
    // 以避免误判有效签名为无效，导致离线模式玩家无法正常使用皮肤。
    MC_UNUSED(valueBytes);

    spdlog::info("SkinMetadataParser: Signature verification requires crypto library support, "
                 "treating signed property as UNSIGNED");
    return SignatureState::Unsigned;
}

bool SkinMetadataParser::verifySignature(const GameProfileProperty& property)
{
    SignatureState state = getSignatureState(property);
    // UNSIGNED（无签名或无法验证）和 SIGNED 视为有效
    // INVALID（签名验证失败）视为无效
    return state != SignatureState::Invalid;
}

// ============================================================================
// SkinMetadataParser - 私有方法
// ============================================================================

void SkinMetadataParser::_parseTexture(const void* textureObj, const std::string& type, SkinTextures& textures)
{
    const auto* jsonObj = static_cast<const nlohmann::json*>(textureObj);
    if (!jsonObj) return;

    // 检查指定类型的纹理对象是否存在
    if (!jsonObj->contains(type) || !(*jsonObj)[type].is_object()) {
        return;
    }

    const auto& obj = (*jsonObj)[type];

    // 提取 URL 和哈希
    std::string url;
    std::string hash;
    if (!extractTextureUrlAndHash(obj, url, hash)) {
        return;
    }

    // 根据纹理类型设置到 SkinTextures 中
    if (type == "SKIN") {
        textures.setSkinUrl(url);
        if (!hash.empty()) {
            textures.setSkinHash(hash);
        }

        // SKIN 类型可能包含 metadata.model 字段（"slim" 或 "default"）
        if (obj.contains("metadata") && obj["metadata"].is_object()) {
            const auto& metadata = obj["metadata"];
            if (metadata.contains("model") && metadata["model"].is_string()) {
                std::string model = metadata["model"].get<std::string>();
                textures.setSkinType(parseSkinType(model));
            }
        }
    } else if (type == "CAPE") {
        textures.setCapeUrl(url);
        if (!hash.empty()) {
            textures.setCapeHash(hash);
        }
    } else if (type == "ELYTRA") {
        textures.setElytraUrl(url);
        if (!hash.empty()) {
            textures.setElytraHash(hash);
        }
    }
}

} // namespace mc::skin
