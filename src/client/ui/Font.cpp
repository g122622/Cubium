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

#include "Font.hpp"
#include "client/ui/Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>
#include <cstring>

// STB image for loading PNG textures (已在TextureAtlasBuilder.cpp中定义)
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stb_image.h>

namespace mc::client {

// ============================================================================
// Font Implementation
// ============================================================================

Font::Font() = default;

Font::~Font()
{
    destroy();
}

Font::Font(Font&& other) noexcept
    : m_atlas(std::move(other.m_atlas))
    , m_providers(std::move(other.m_providers))
    , m_fontHeight(other.m_fontHeight)
    , m_glyphsByWidth(std::move(other.m_glyphsByWidth))
    , m_widthIndexBuilt(other.m_widthIndexBuilt)
{}

Font& Font::operator=(Font&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_atlas = std::move(other.m_atlas);
        m_providers = std::move(other.m_providers);
        m_fontHeight = other.m_fontHeight;
        m_glyphsByWidth = std::move(other.m_glyphsByWidth);
        m_widthIndexBuilt = other.m_widthIndexBuilt;
    }
    return *this;
}

Result<void> Font::initialize(u32 textureSize)
{
    return m_atlas.create(textureSize);
}

void Font::destroy()
{
    m_providers.clear();
    m_atlas.destroy();
    m_fontHeight = 9;
    m_glyphsByWidth.clear();
    m_widthIndexBuilt = false;
}

void Font::addProvider(std::unique_ptr<IGlyphProvider> provider)
{
    // 更新字体高度
    u32 providerHeight = provider->getFontHeight();
    if (providerHeight > m_fontHeight) {
        m_fontHeight = providerHeight;
    }
    m_providers.push_back(std::move(provider));

    // 新增字形提供者后，宽度索引需要重新构建
    m_widthIndexBuilt = false;
    m_glyphsByWidth.clear();
}

const Glyph* Font::getGlyph(u32 codepoint)
{
    // 空格特殊处理
    if (codepoint == ' ') {
        return &m_emptyGlyph;
    }

    // 先从图集缓存查找
    const Glyph* cached = m_atlas.getGlyph(codepoint);
    if (cached != nullptr) {
        return cached;
    }

    // 从提供者加载
    for (auto& provider : m_providers) {
        std::vector<u8> pixels;
        u32 width, height;
        f32 advance, bearingX, bearingY;

        if (provider->getGlyphData(codepoint, pixels, width, height, advance, bearingX, bearingY)) {
            // 添加到图集
            auto result = m_atlas.addGlyph(codepoint, pixels.data(), width, height, advance, bearingX, bearingY);
            if (result.success()) {
                return m_atlas.getGlyph(codepoint);
            }
            // 如果图集满了，继续尝试下一个提供者
        }
    }

    return nullptr;
}

f32 Font::getStringWidth(const std::string& text)
{
    f32 width = 0.0f;
    for (char c : text) {
        const Glyph* glyph = getGlyph(static_cast<u32>(static_cast<u8>(c)));
        if (glyph != nullptr) {
            width += glyph->advance;
        } else {
            // 未知字符使用默认宽度
            width += 4.0f;
        }
    }
    return width;
}

f32 Font::getStringWidthUTF8(const std::string& text)
{
    f32 width = 0.0f;
    size_t i = 0;

    while (i < text.size()) {
        u32 codepoint = 0;
        u8 byte = static_cast<u8>(text[i]);

        // UTF-8解码
        if ((byte & 0x80) == 0) {
            // 单字节字符
            codepoint = byte;
            i += 1;
        } else if ((byte & 0xE0) == 0xC0) {
            // 双字节字符
            if (i + 1 >= text.size()) break;
            codepoint = ((byte & 0x1F) << 6) | (static_cast<u8>(text[i + 1]) & 0x3F);
            i += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            // 三字节字符
            if (i + 2 >= text.size()) break;
            codepoint = ((byte & 0x0F) << 12) | ((static_cast<u8>(text[i + 1]) & 0x3F) << 6) |
                (static_cast<u8>(text[i + 2]) & 0x3F);
            i += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            // 四字节字符
            if (i + 3 >= text.size()) break;
            codepoint = ((byte & 0x07) << 18) | ((static_cast<u8>(text[i + 1]) & 0x3F) << 12) |
                ((static_cast<u8>(text[i + 2]) & 0x3F) << 6) | (static_cast<u8>(text[i + 3]) & 0x3F);
            i += 4;
        } else {
            // 无效的UTF-8序列
            i += 1;
            continue;
        }

        const Glyph* glyph = getGlyph(codepoint);
        if (glyph != nullptr) {
            width += glyph->advance;
        } else {
            width += 4.0f; // 默认宽度
        }
    }

    return width;
}

u32 Font::getFontHeight() const
{
    return m_fontHeight;
}

const Glyph* Font::getRandomGlyph(math::Random& random, i32 advanceWidth)
{
    // 确保宽度索引已构建
    if (!m_widthIndexBuilt) {
        buildWidthIndex();
    }

    auto it = m_glyphsByWidth.find(advanceWidth);
    if (it == m_glyphsByWidth.end() || it->second.empty()) {
        // 没有匹配宽度的字形，尝试邻近宽度
        // 查找 ±1 宽度范围内的字形
        for (i32 delta = 1; delta <= 2; ++delta) {
            auto itPlus = m_glyphsByWidth.find(advanceWidth + delta);
            if (itPlus != m_glyphsByWidth.end() && !itPlus->second.empty()) {
                i32 idx = random.nextInt(static_cast<i32>(itPlus->second.size()));
                return getGlyph(itPlus->second[static_cast<size_t>(idx)]);
            }
            auto itMinus = m_glyphsByWidth.find(advanceWidth - delta);
            if (itMinus != m_glyphsByWidth.end() && !itMinus->second.empty()) {
                i32 idx = random.nextInt(static_cast<i32>(itMinus->second.size()));
                return getGlyph(itMinus->second[static_cast<size_t>(idx)]);
            }
        }
        return nullptr;
    }

    // 从匹配宽度的码点列表中随机选择
    i32 idx = random.nextInt(static_cast<i32>(it->second.size()));
    return getGlyph(it->second[static_cast<size_t>(idx)]);
}

void Font::buildWidthIndex()
{
    m_glyphsByWidth.clear();

    for (const auto& provider : m_providers) {
        for (u32 codepoint : provider->getCodepoints()) {
            // 跳过空格
            if (codepoint == ' ') {
                continue;
            }

            // 获取字形以计算前进宽度
            const Glyph* glyph = getGlyph(codepoint);
            if (glyph == nullptr) {
                continue;
            }

            // 计算非粗体前进宽度的向上取整
            i32 width = static_cast<i32>(std::ceil(glyph->advance));

            // 过滤异常宽度的字形（advance ≤ 0 或 > 32）
            if (width <= 0 || width > 32) {
                continue;
            }

            m_glyphsByWidth[width].push_back(codepoint);
        }
    }

    m_widthIndexBuilt = true;
}

// ============================================================================
// BitmapGlyphProvider Implementation
// ============================================================================

Result<void> BitmapGlyphProvider::load(IResourcePack& pack,
    const std::string& texturePath,
    u32 height,
    u32 ascent,
    const std::vector<std::string>& charRows)
{
    // 构建完整资源路径
    std::string fullPath = "minecraft/textures/";
    fullPath += texturePath;

    // 加载纹理
    auto dataResult = pack.readResource(resource::PackType::ClientResources, fullPath);
    if (!dataResult.success()) {
        return Error(ErrorCode::FileReadFailed, "Failed to load font texture: " + fullPath);
    }

    auto& data = dataResult.value();
    if (data.empty()) {
        return Error(ErrorCode::InvalidData, "Empty font texture");
    }

    // 使用stb_image加载PNG
    int texWidth, texHeight, channels;
    u8* pixels = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &texWidth, &texHeight, &channels, 4);
    if (pixels == nullptr) {
        return Error(ErrorCode::TextureLoadFailed, "Failed to decode font texture");
    }

    // 存储像素数据
    m_textureWidth = static_cast<u32>(texWidth);
    m_textureHeight = static_cast<u32>(texHeight);
    m_pixels.resize(static_cast<size_t>(texWidth) * texHeight * 4);
    std::memcpy(m_pixels.data(), pixels, m_pixels.size());
    stbi_image_free(pixels);

    m_height = height;
    m_ascent = ascent;

    // 计算单元格尺寸
    if (charRows.empty()) {
        return Error(ErrorCode::InvalidData, "No character rows defined");
    }

    u32 cols = static_cast<u32>(charRows[0].length());
    u32 rows = static_cast<u32>(charRows.size());
    m_charWidth = m_textureWidth / cols;

    // 构建码点映射
    m_codepoints.clear();
    m_codepointToIndex.clear();

    for (u32 row = 0; row < rows; ++row) {
        const std::string& chars = charRows[row];
        for (u32 col = 0; col < chars.length(); ++col) {
            u32 codepoint = static_cast<u32>(static_cast<u8>(chars[col]));
            if (codepoint != 0 && codepoint != ' ') {
                m_codepoints.push_back(codepoint);
                m_codepointToIndex[codepoint] = static_cast<u32>(m_codepoints.size() - 1);
            }
        }
    }

    return {};
}

u32 BitmapGlyphProvider::_calculateCharWidth(u32 charX, u32 charY, u32 charWidth, u32 charHeight) const
{
    // 从右向左扫描，找到最右边的非透明像素
    for (i32 x = static_cast<i32>(charWidth) - 1; x >= 0; --x) {
        for (u32 y = 0; y < charHeight; ++y) {
            u32 pixelX = charX + static_cast<u32>(x);
            u32 pixelY = charY + y;
            u32 offset = (pixelY * m_textureWidth + pixelX) * 4;

            // 检查alpha通道
            if (offset + 3 < m_pixels.size() && m_pixels[offset + 3] > 0) {
                return static_cast<u32>(x + 1);
            }
        }
    }
    return 0;
}

bool BitmapGlyphProvider::getGlyphData(u32 codepoint,
    std::vector<u8>& outPixels,
    u32& outWidth,
    u32& outHeight,
    f32& outAdvance,
    f32& outBearingX,
    f32& outBearingY) const
{
    auto it = m_codepointToIndex.find(codepoint);
    if (it == m_codepointToIndex.end()) {
        return false;
    }

    u32 index = it->second;
    u32 cols = m_textureWidth / m_charWidth;
    u32 row = index / cols;
    u32 col = index % cols;

    u32 charX = col * m_charWidth;
    u32 charY = row * m_height;

    // 计算实际字符宽度
    u32 actualWidth = _calculateCharWidth(charX, charY, m_charWidth, m_height);
    if (actualWidth == 0) {
        actualWidth = 1; // 至少1像素宽
    }

    // 提取像素数据（只取alpha通道作为灰度）
    outPixels.resize(actualWidth * m_height);
    for (u32 y = 0; y < m_height; ++y) {
        for (u32 x = 0; x < actualWidth; ++x) {
            u32 srcOffset = ((charY + y) * m_textureWidth + (charX + x)) * 4;
            u32 dstOffset = y * actualWidth + x;
            // 使用alpha通道作为灰度值
            outPixels[dstOffset] = m_pixels[srcOffset + 3];
        }
    }

    outWidth = actualWidth;
    outHeight = m_height;
    outAdvance = static_cast<f32>(actualWidth + 1); // +1像素间距
    outBearingX = 0.0f;
    outBearingY = static_cast<f32>(m_ascent);

    return true;
}

} // namespace mc::client
