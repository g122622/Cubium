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

#include "FontTextureAtlas.hpp"
#include "Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {}

namespace mc::client {

/**
 * @brief 字形提供者接口
 *
 * 定义字形加载的统一接口，支持不同的字体来源
 * （位图字体、TrueType字体等）
 */
class IGlyphProvider {
public:
    virtual ~IGlyphProvider() = default;

    /**
     * @brief 获取字形像素数据
     * @param codepoint Unicode码点
     * @param outPixels 输出像素数据（调用者负责释放）
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     * @param outAdvance 输出前进宽度
     * @param outBearingX 输出水平偏移
     * @param outBearingY 输出垂直偏移
     * @return 是否找到字形
     */
    [[nodiscard]] virtual bool getGlyphData(u32 codepoint,
        std::vector<u8>& outPixels,
        u32& outWidth,
        u32& outHeight,
        f32& outAdvance,
        f32& outBearingX,
        f32& outBearingY) const = 0;

    /**
     * @brief 获取提供的码点集合
     */
    [[nodiscard]] virtual const std::vector<u32>& getCodepoints() const = 0;

    /**
     * @brief 获取字体高度
     */
    [[nodiscard]] virtual u32 getFontHeight() const = 0;

    /**
     * @brief 获取基线到字形顶部的距离
     */
    [[nodiscard]] virtual u32 getAscent() const = 0;
};

/**
 * @brief 位图字形提供者
 *
 * 加载Minecraft格式的位图字体（ascii.png等）
 * JSON格式:
 * {
 *   "type": "bitmap",
 *   "file": "minecraft:font/ascii.png",
 *   "height": 8,
 *   "ascent": 7,
 *   "chars": ["ABCDEFGHIJ...", "abcdefghij..."]
 * }
 */
class BitmapGlyphProvider : public IGlyphProvider {
public:
    /**
     * @brief 构造位图字形提供者
     */
    BitmapGlyphProvider() = default;

    /**
     * @brief 从资源包加载位图字体
     * @param pack 资源包
     * @param texturePath 纹理文件路径（如 "font/ascii.png"）
     * @param height 字符高度（像素）
     * @param ascent 基线到字符顶部距离
     * @param charRows 字符行定义
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> load(IResourcePack& pack,
        const std::string& texturePath,
        u32 height,
        u32 ascent,
        const std::vector<std::string>& charRows);

    [[nodiscard]] bool getGlyphData(u32 codepoint,
        std::vector<u8>& outPixels,
        u32& outWidth,
        u32& outHeight,
        f32& outAdvance,
        f32& outBearingX,
        f32& outBearingY) const override;

    [[nodiscard]] const std::vector<u32>& getCodepoints() const override { return m_codepoints; }

    [[nodiscard]] u32 getFontHeight() const override { return m_height; }
    [[nodiscard]] u32 getAscent() const override { return m_ascent; }

private:
    /**
     * @brief 从像素数据计算字符的实际宽度
     * @param charX 字符在纹理中的X位置
     * @param charY 字符在纹理中的Y位置
     * @param charWidth 字符单元格宽度
     * @param charHeight 字符单元格高度
     * @return 实际宽度
     */
    [[nodiscard]] u32 _calculateCharWidth(u32 charX, u32 charY, u32 charWidth, u32 charHeight) const;

    std::vector<u8> m_pixels;                        // 纹理像素数据 (RGBA)
    u32 m_textureWidth = 0;                          // 纹理宽度
    u32 m_textureHeight = 0;                         // 纹理高度
    u32 m_height = 8;                                // 字符高度
    u32 m_ascent = 7;                                // 基线到顶部距离
    u32 m_charWidth = 8;                             // 单元格宽度
    std::vector<u32> m_codepoints;                   // 支持的码点列表
    std::unordered_map<u32, u32> m_codepointToIndex; // 码点到索引的映射
};

/**
 * @brief 字体类
 *
 * 管理字形纹理图集和字形缓存。
 */
class Font {
public:
    Font();
    ~Font();

    // 禁止拷贝
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // 允许移动
    Font(Font&& other) noexcept;
    Font& operator=(Font&& other) noexcept;

    /**
     * @brief 初始化字体
     * @param textureSize 字形纹理图集大小
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(u32 textureSize);

    /**
     * @brief 销毁字体
     */
    void destroy();

    /**
     * @brief 添加字形提供者
     * @param provider 字形提供者
     */
    void addProvider(std::unique_ptr<IGlyphProvider> provider);

    /**
     * @brief 获取字形（从缓存或加载到图集）
     * @param codepoint Unicode码点
     * @return 字形指针，如果不存在返回nullptr
     */
    [[nodiscard]] const Glyph* getGlyph(u32 codepoint);

    /**
     * @brief 获取随机等宽字形（用于混淆文字 §k）
     *
     * 从与指定前进宽度相同的字形集合中随机选择一个字形。
     * 如果找不到匹配宽度的字形，返回 nullptr。
     * 与 MC Java 的 FontSet.getRandomGlyph() 对应。
     *
     * @param random 随机数生成器
     * @param advanceWidth 目标前进宽度（非粗体，取 ceil）
     * @return 随机字形指针，如果没有匹配宽度的字形则返回 nullptr
     */
    [[nodiscard]] const Glyph* getRandomGlyph(math::Random& random, i32 advanceWidth);

    /**
     * @brief 构建宽度索引（按前进宽度索引字形码点）
     *
     * 遍历所有字形提供者，为每个码点计算其前进宽度并分类存储。
     * 用于混淆文字（§k）的随机等宽字形选择。
     * 必须在所有 addProvider() 调用之后调用。
     */
    void buildWidthIndex();

    /**
     * @brief 获取字符串宽度
     * @param text 文本
     * @return 宽度（像素）
     */
    [[nodiscard]] f32 getStringWidth(const std::string& text);

    /**
     * @brief 获取字符串宽度（UTF-8）
     * @param text UTF-8文本
     * @return 宽度（像素）
     */
    [[nodiscard]] f32 getStringWidthUTF8(const std::string& text);

    /**
     * @brief 获取字体高度
     * @return 高度（像素）
     */
    [[nodiscard]] u32 getFontHeight() const;

    /**
     * @brief 获取字形纹理图集
     */
    [[nodiscard]] FontTextureAtlas& atlas() { return m_atlas; }
    [[nodiscard]] const FontTextureAtlas& atlas() const { return m_atlas; }

    /**
     * @brief 获取图集像素数据（用于上传到GPU）
     */
    [[nodiscard]] const u8* atlasPixels() const { return m_atlas.pixelData(); }
    [[nodiscard]] u32 atlasSize() const { return m_atlas.textureSize(); }

    /**
     * @brief 检查字体是否有效
     */
    [[nodiscard]] bool isValid() const { return m_atlas.isValid(); }

private:
    FontTextureAtlas m_atlas;                                 // 字形纹理图集
    std::vector<std::unique_ptr<IGlyphProvider>> m_providers; // 字形提供者列表
    u32 m_fontHeight = 9;                                     // 默认字体高度
    EmptyGlyph m_emptyGlyph;                                  // 空字形缓存

    /**
     * @brief 按前进宽度索引的字形码点集合
     *
     * key = ceil(advance)（非粗体前进宽度的向上取整）
     * value = 具有该宽度的所有码点列表
     *
     * 用于混淆文字（§k）渲染时选择等宽随机替换字符。
     * 用于混淆文字（§k）渲染时选择等宽随机替换字符。
     */
    std::unordered_map<i32, std::vector<u32>> m_glyphsByWidth;
    bool m_widthIndexBuilt = false; // 宽度索引是否已构建
};

} // namespace mc::client
