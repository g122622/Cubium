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

#include "ITextComponent.hpp"
#include "StringTextComponent.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/TextStyle.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace mc::text {

/**
 * @brief 文本解析器
 *
 * 解析 § 代码格式的文本为 ITextComponent。
 * 参考: net.minecraft.util.text.TextComponentUtils
 *
 * ## 支持的格式
 *
 * - 颜色代码: §0-§9, §a-§f
 * - 样式代码: §k (混淆), §l (粗体), §m (删除线), §n (下划线), §o (斜体)
 * - 重置代码: §r
 *
 * ## 使用示例
 *
 * ```cpp
 * // 解析带格式的文本
 * auto text = TextParser::parse("§cHello §lWorld!");
 * // 结果: StringTextComponent("Hello ", 红色)
 * //        + StringTextComponent("World!", 红色+粗体)
 *
 * // 转换回 § 格式
 * std::string legacy = TextParser::toLegacyFormat(*text);
 * // 结果: "§cHello §lWorld!"
 * ```
 */
class TextParser {
public:
    /**
     * @brief 解析 § 代码格式的文本
     *
     * 将 "§cHello §lWorld!" 解析为 ITextComponent 树。
     *
     * @param text 带格式的文本
     * @return 文本组件的所有权
     */
    [[nodiscard]] static std::unique_ptr<ITextComponent> parse(std::string_view text);

    /**
     * @brief 将 ITextComponent 转换为 § 代码格式
     *
     * 递归遍历组件树，生成带 § 代码的文本。
     *
     * @param component 文本组件
     * @return 带 § 代码的文本
     */
    [[nodiscard]] static std::string toLegacyFormat(const ITextComponent& component);

private:
    /**
     * @brief 解析状态
     */
    struct ParseState {
        Style currentStyle;                        // 当前样式
        std::string currentText;                   // 当前累积文本
        std::unique_ptr<StringTextComponent> root; // 根组件
        StringTextComponent* currentComponent;     // 当前组件（非拥有指针）

        ParseState()
            : root(std::make_unique<StringTextComponent>())
            , currentComponent(root.get())
        {}

        /**
         * @brief 刷新当前文本到组件
         */
        void flushText()
        {
            if (!currentText.empty()) {
                // 创建新组件存储当前文本和样式
                auto newComponent = std::make_unique<StringTextComponent>(std::move(currentText));
                newComponent->setStyle(currentStyle);
                currentComponent->append(std::move(newComponent));
                currentText.clear();
            }
        }
    };

    /**
     * @brief 处理 § 代码
     *
     * @param state 解析状态
     * @param code § 后的字符
     * @return 是否成功处理
     */
    static bool handleFormattingCode(ParseState& state, char code);
};

} // namespace mc::text
