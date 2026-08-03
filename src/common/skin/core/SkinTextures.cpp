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

#include "SkinTextures.hpp"
#include <cstddef>
#include <string>
#include <spdlog/spdlog.h>

namespace mc::skin {

std::string SkinTextures::getSkinCacheKey() const
{
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

std::string SkinTextures::getCapeCacheKey() const
{
    if (!m_capeHash.has_value() || m_capeHash->empty()) {
        return "";
    }

    if (m_capeHash->length() >= 2) {
        return "capes/" + m_capeHash->substr(0, 2) + "/" + *m_capeHash;
    }
    return "capes/" + *m_capeHash;
}

std::string SkinTextures::getElytraCacheKey() const
{
    if (!m_elytraHash.has_value() || m_elytraHash->empty()) {
        return "";
    }

    if (m_elytraHash->length() >= 2) {
        return "elytra/" + m_elytraHash->substr(0, 2) + "/" + *m_elytraHash;
    }
    return "elytra/" + *m_elytraHash;
}

std::string SkinTextures::extractHashFromUrl(const std::string& url)
{
    // 查找最后一个斜杠
    size_t lastSlash = url.rfind('/');
    if (lastSlash == std::string::npos || lastSlash + 1 >= url.length()) {
        spdlog::warn("SkinTextures::extractHashFromUrl: Invalid URL format: {}", url);
        return "";
    }

    // 提取哈希部分
    std::string hash = url.substr(lastSlash + 1);

    // 验证哈希格式（应该是64个十六进制字符）
    if (hash.length() != 64) {
        // 有些URL可能有查询参数，尝试移除
        size_t queryPos = hash.find('?');
        if (queryPos != std::string::npos) {
            hash = hash.substr(0, queryPos);
        }
    }

    return hash;
}

} // namespace mc::skin
