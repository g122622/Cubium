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

#include "Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::client {

/**
 * @brief 字形纹理图集
 *
 * 将字形动态打包到纹理中，使用简化的矩形打包算法（二叉树分割）。
 */
class FontTextureAtlas {
public:
    FontTextureAtlas();
    ~FontTextureAtlas();

    // 禁止拷贝
    FontTextureAtlas(const FontTextureAtlas&) = delete;
    FontTextureAtlas& operator=(const FontTextureAtlas&) = delete;

    // 允许移动
    FontTextureAtlas(FontTextureAtlas&& other) noexcept;
    FontTextureAtlas& operator=(FontTextureAtlas&& other) noexcept;

    /**
     * @brief 创建字形纹理图集
     * @param textureSize 纹理尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(u32 textureSize);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 添加字形到图集
     * @param codepoint Unicode码点
     * @param pixels 字形像素数据 (单通道灰度，8位)
     * @param width 字形宽度
     * @param height 字形高度
     * @param advance 前进宽度
     * @param bearingX 水平偏移
     * @param bearingY 垂直偏移（从基线到字形顶部）
     * @return 字形信息，如果图集已满则返回错误
     */
    [[nodiscard]] Result<Glyph> addGlyph(
        u32 codepoint, const u8* pixels, u32 width, u32 height, f32 advance, f32 bearingX, f32 bearingY);

    /**
     * @brief 获取字形
     * @param codepoint Unicode码点
     * @return 字形指针，如果不存在返回nullptr
     */
    [[nodiscard]] const Glyph* getGlyph(u32 codepoint) const;

    /**
     * @brief 检查字形是否存在
     */
    [[nodiscard]] bool hasGlyph(u32 codepoint) const;

    /**
     * @brief 获取纹理像素数据 (单通道灰度)
     * @return 像素数据指针
     */
    [[nodiscard]] const u8* pixelData() const { return m_pixels.data(); }

    /**
     * @brief 获取纹理数据 (可修改，用于上传到GPU)
     */
    [[nodiscard]] u8* pixelData() { return m_pixels.data(); }

    /**
     * @brief 获取纹理尺寸
     */
    [[nodiscard]] u32 textureSize() const { return m_textureSize; }

    /**
     * @brief 检查图集是否有效
     */
    [[nodiscard]] bool isValid() const { return !m_pixels.empty(); }

    /**
     * @brief 获取已使用的字形数量
     */
    [[nodiscard]] size_t glyphCount() const { return m_glyphs.size(); }

private:
    /**
     * @brief 矩形节点 (用于二叉树打包)
     */
    struct Node {
        u32 x, y;          // 位置
        u32 width, height; // 尺寸
        Node* left = nullptr;
        Node* right = nullptr;
        bool used = false;

        Node(u32 x_, u32 y_, u32 w, u32 h)
            : x(x_)
            , y(y_)
            , width(w)
            , height(h)
        {}

        ~Node()
        {
            delete left;
            delete right;
        }
    };

    /**
     * @brief 在节点树中找到合适的空间
     * @param node 当前节点
     * @param width 需要的宽度
     * @param height 需要的高度
     * @return 找到的节点，如果找不到返回nullptr
     */
    [[nodiscard]] Node* _findNode(Node* node, u32 width, u32 height);

    /**
     * @brief 分割节点
     * @param node 要分割的节点
     * @param width 使用的宽度
     * @param height 使用的高度
     * @return 使用后的节点
     */
    [[nodiscard]] Node* _splitNode(Node* node, u32 width, u32 height);

    /**
     * @brief 将像素数据复制到纹理
     * @param x 目标X位置
     * @param y 目标Y位置
     * @param width 源宽度
     * @param height 源高度
     * @param pixels 源像素数据
     */
    void _copyPixels(u32 x, u32 y, u32 width, u32 height, const u8* pixels);

    u32 m_textureSize = 0;
    std::vector<u8> m_pixels;                // 灰度纹理数据
    std::unordered_map<u32, Glyph> m_glyphs; // 字形映射表
    Node* m_root = nullptr;                  // 打包树根节点
    u32 m_padding = 1;                       // 字形间距（防止纹理 bleeding）
};

} // namespace mc::client
