#include "SkinTextures.hpp"
#include <spdlog/spdlog.h>

namespace mc::skin {

String SkinTextures::getSkinCacheKey() const {
    if (!m_skinHash.has_value() || m_skinHash->empty()) {
        return "";
    }

    // 格式: skins/<hash前2字符>/<完整hash>
    // 例如: skins/ab/abcdef1234567890...
    if (m_skinHash->length() >= 2) {
        return "skins/" + m_skinHash->substr(0, 2) + "/" + *m_skinHash;
    }
    return "skins/" + *m_skinHash;
}

String SkinTextures::getCapeCacheKey() const {
    if (!m_capeHash.has_value() || m_capeHash->empty()) {
        return "";
    }

    if (m_capeHash->length() >= 2) {
        return "capes/" + m_capeHash->substr(0, 2) + "/" + *m_capeHash;
    }
    return "capes/" + *m_capeHash;
}

String SkinTextures::getElytraCacheKey() const {
    if (!m_elytraHash.has_value() || m_elytraHash->empty()) {
        return "";
    }

    if (m_elytraHash->length() >= 2) {
        return "elytra/" + m_elytraHash->substr(0, 2) + "/" + *m_elytraHash;
    }
    return "elytra/" + *m_elytraHash;
}

String SkinTextures::extractHashFromUrl(const String& url) {
    // Mojang 皮肤URL格式：http://textures.minecraft.net/texture/<hash>
    // 或：https://textures.minecraft.net/texture/<hash>

    // 查找最后一个斜杠
    size_t lastSlash = url.rfind('/');
    if (lastSlash == String::npos || lastSlash + 1 >= url.length()) {
        spdlog::warn("SkinTextures::extractHashFromUrl: Invalid URL format: {}", url);
        return "";
    }

    // 提取哈希部分
    String hash = url.substr(lastSlash + 1);

    // 验证哈希格式（应该是64个十六进制字符）
    if (hash.length() != 64) {
        // 有些URL可能有查询参数，尝试移除
        size_t queryPos = hash.find('?');
        if (queryPos != String::npos) {
            hash = hash.substr(0, queryPos);
        }

        if (hash.length() != 64) {
            spdlog::debug("SkinTextures::extractHashFromUrl: Unusual hash length: {} (URL: {})",
                         hash.length(), url);
        }
    }

    return hash;
}

} // namespace mc::skin
