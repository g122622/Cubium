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
#include "common/skin/core/SkinTextures.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// Base64 解码
#include <stdexcept>

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
    int bits = 0;

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
            continue;
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

} // anonymous namespace

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

        // 解析 SKIN
        if (texturesObj.contains("SKIN") && texturesObj["SKIN"].is_object()) {
            const auto& skinObj = texturesObj["SKIN"];

            // URL
            if (skinObj.contains("url") && skinObj["url"].is_string()) {
                std::string url = skinObj["url"].get<std::string>();
                textures.setSkinUrl(url);

                // 提取哈希
                std::string hash = SkinTextures::extractHashFromUrl(url);
                if (!hash.empty()) {
                    textures.setSkinHash(hash);
                }
            }

            // Metadata - model
            if (skinObj.contains("metadata") && skinObj["metadata"].is_object()) {
                const auto& metadata = skinObj["metadata"];
                if (metadata.contains("model") && metadata["model"].is_string()) {
                    std::string model = metadata["model"].get<std::string>();
                    textures.setSkinType(parseSkinType(model));
                }
            }
        }

        // 解析 CAPE
        if (texturesObj.contains("CAPE") && texturesObj["CAPE"].is_object()) {
            const auto& capeObj = texturesObj["CAPE"];

            if (capeObj.contains("url") && capeObj["url"].is_string()) {
                std::string url = capeObj["url"].get<std::string>();
                textures.setCapeUrl(url);

                std::string hash = SkinTextures::extractHashFromUrl(url);
                if (!hash.empty()) {
                    textures.setCapeHash(hash);
                }
            }
        }

        // 解析 ELYTRA
        if (texturesObj.contains("ELYTRA") && texturesObj["ELYTRA"].is_object()) {
            const auto& elytraObj = texturesObj["ELYTRA"];

            if (elytraObj.contains("url") && elytraObj["url"].is_string()) {
                std::string url = elytraObj["url"].get<std::string>();
                textures.setElytraUrl(url);

                std::string hash = SkinTextures::extractHashFromUrl(url);
                if (!hash.empty()) {
                    textures.setElytraHash(hash);
                }
            }
        }

        spdlog::debug("SkinMetadataParser: Parsed textures - skin:{}, cape:{}, elytra:{}",
            textures.hasSkin(),
            textures.hasCape(),
            textures.hasElytra());
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse textures JSON: ") + e.what());
    }

    return textures;
}

bool SkinMetadataParser::verifySignature(const GameProfileProperty& property)
{
    // TODO: 实现签名验证
    // 需要使用 Mojang 的公钥验证签名
    // 签名数据是 property.signature
    // 签名内容是 property.value 的 Base64 解码结果

    if (!property.hasSignature()) {
        // 没有签名，在离线模式下可以接受
        spdlog::debug("SkinMetadataParser: No signature for textures property");
        return true; // 允许无签名
    }

    // 生产环境应该验证签名
    // 当前简化实现：跳过验证
    spdlog::debug("SkinMetadataParser: Signature verification not implemented, skipping");
    return true;
}

void SkinMetadataParser::parseTexture(const void* textureObj, const std::string& type, SkinTextures& textures)
{
    // 此方法在 parseJson 中已经实现，这里保留用于未来扩展
}

} // namespace mc::skin
