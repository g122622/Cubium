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

#include "FontTextureAtlas.hpp"
#include "client/ui/Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstring>
#include <utility>

namespace mc::client {

FontTextureAtlas::FontTextureAtlas() = default;

FontTextureAtlas::~FontTextureAtlas()
{
    destroy();
}

FontTextureAtlas::FontTextureAtlas(FontTextureAtlas&& other) noexcept
    : m_textureSize(other.m_textureSize)
    , m_pixels(std::move(other.m_pixels))
    , m_glyphs(std::move(other.m_glyphs))
    , m_root(other.m_root)
    , m_padding(other.m_padding)
{
    other.m_root = nullptr;
    other.m_textureSize = 0;
}

FontTextureAtlas& FontTextureAtlas::operator=(FontTextureAtlas&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_textureSize = other.m_textureSize;
        m_pixels = std::move(other.m_pixels);
        m_glyphs = std::move(other.m_glyphs);
        m_root = other.m_root;
        m_padding = other.m_padding;
        other.m_root = nullptr;
        other.m_textureSize = 0;
    }
    return *this;
}

Result<void> FontTextureAtlas::create(u32 textureSize)
{
    if (textureSize == 0) {
        return Error(ErrorCode::InvalidArgument, "Texture size must be positive");
    }

    m_textureSize = textureSize;
    m_pixels.resize(static_cast<size_t>(textureSize) * textureSize, 0);

    // 创建根节点（整个纹理区域）
    m_root = new Node(0, 0, textureSize, textureSize);

    return {};
}

void FontTextureAtlas::destroy()
{
    delete m_root;
    m_root = nullptr;
    m_pixels.clear();
    m_glyphs.clear();
    m_textureSize = 0;
}

FontTextureAtlas::Node* FontTextureAtlas::_findNode(Node* node, u32 width, u32 height)
{
    if (node == nullptr) {
        return nullptr;
    }

    // 如果节点已被使用，递归查找子节点
    if (node->used) {
        Node* found = _findNode(node->left, width, height);
        if (found != nullptr) {
            return found;
        }
        return _findNode(node->right, width, height);
    }

    // 检查节点大小是否合适
    if (width <= node->width && height <= node->height) {
        return node;
    }

    return nullptr;
}

FontTextureAtlas::Node* FontTextureAtlas::_splitNode(Node* node, u32 width, u32 height)
{
    // 标记节点为已使用
    node->used = true;

    // 计算剩余空间
    u32 remainingWidth = node->width - width;
    u32 remainingHeight = node->height - height;

    // 创建两个子节点存储剩余空间（Shelf NFD算法）
    // 右边：同行剩余宽度区域（高度与分配区域相同）
    // 下边：下方剩余高度区域（宽度为节点原始宽度）

    if (remainingWidth > 0) {
        node->left = new Node(node->x + width, node->y, remainingWidth, height);
    }

    if (remainingHeight > 0) {
        node->right = new Node(node->x, node->y + height, node->width, remainingHeight);
    }

    // 返回原节点，其 (x, y) 就是分配区域的起点
    return node;
}

void FontTextureAtlas::_copyPixels(u32 x, u32 y, u32 width, u32 height, const u8* pixels)
{
    for (u32 row = 0; row < height; ++row) {
        u32 dstOffset = (y + row) * m_textureSize + x;
        u32 srcOffset = row * width;
        std::memcpy(m_pixels.data() + dstOffset, pixels + srcOffset, width);
    }
}

Result<Glyph> FontTextureAtlas::addGlyph(
    u32 codepoint, const u8* pixels, u32 width, u32 height, f32 advance, f32 bearingX, f32 bearingY)
{
    if (!isValid()) {
        return Error(ErrorCode::NotInitialized, "FontTextureAtlas not initialized");
    }

    if (pixels == nullptr || width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid glyph data");
    }

    // 检查是否已存在
    if (hasGlyph(codepoint)) {
        return Error(ErrorCode::AlreadyExists, "Glyph already exists");
    }

    // 找到合适的空间（包含padding的空间）
    u32 paddedWidth = width + m_padding;
    u32 paddedHeight = height + m_padding;

    Node* node = _findNode(m_root, paddedWidth, paddedHeight);
    if (node == nullptr) {
        return Error(ErrorCode::CapacityExceeded, "Font texture atlas is full");
    }

    // 分割节点（使用带padding的大小）
    node = _splitNode(node, paddedWidth, paddedHeight);

    // 绘制位置就是节点位置（不需要额外偏移padding）
    u32 drawX = node->x;
    u32 drawY = node->y;

    // 复制像素数据到纹理
    _copyPixels(drawX, drawY, width, height, pixels);

    // 计算UV坐标
    f32 texSize = static_cast<f32>(m_textureSize);
    f32 u0 = static_cast<f32>(drawX) / texSize;
    f32 v0 = static_cast<f32>(drawY) / texSize;
    f32 u1 = static_cast<f32>(drawX + width) / texSize;
    f32 v1 = static_cast<f32>(drawY + height) / texSize;

    // 创建字形
    Glyph glyph(
        codepoint, u0, v0, u1, v1, advance, bearingX, bearingY, static_cast<f32>(width), static_cast<f32>(height));

    // 存储字形
    m_glyphs[codepoint] = glyph;

    return glyph;
}

const Glyph* FontTextureAtlas::getGlyph(u32 codepoint) const
{
    auto it = m_glyphs.find(codepoint);
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    return nullptr;
}

bool FontTextureAtlas::hasGlyph(u32 codepoint) const
{
    return m_glyphs.find(codepoint) != m_glyphs.end();
}

} // namespace mc::client
