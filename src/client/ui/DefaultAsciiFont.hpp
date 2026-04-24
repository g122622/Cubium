#pragma once

#include "Font.hpp"
#include "common/core/Result.hpp"
#include <array>

namespace mc::client {

/**
 * @brief 默认ASCII位图字体生成器
 *
 * 生成基本的ASCII字符位图，用于调试和基础显示。
 * 字体数据内置在代码中，不依赖外部资源。
 *
 * 参考：
 * - MC的默认字体是8x8像素
 * - 包含ASCII可打印字符（32-126）
 */
class DefaultAsciiFont {
public:
    /**
     * @brief 创建默认ASCII字体
     * @param font 目标字体对象
     * @return 成功或错误
     */
    [[nodiscard]] static Result<void> create(Font& font);

    /**
     * @brief 获取字体高度
     */
    [[nodiscard]] static constexpr u32 fontHeight() { return 8; }

    /**
     * @brief 获取字体宽度
     */
    [[nodiscard]] static constexpr u32 fontWidth() { return 8; }

private:
    /**
     * @brief 生成单个字符的位图数据
     * @param c 字符
     * @param outPixels 输出像素数据（8x8）
     * @return 字符宽度
     */
    [[nodiscard]] static u32 generateCharBitmap(char c, std::array<u8, 64>& outPixels);

    /**
     * @brief 获取5x7点阵字体数据
     */
    [[nodiscard]] static const u8* getFontData(char c);
};

} // namespace mc::client
