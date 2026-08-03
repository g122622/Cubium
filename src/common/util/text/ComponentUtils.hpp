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

#include "TranslationTextComponent.hpp"
#include "common/util/text/ITextComponent.hpp"
#include <memory>
#include <utility>

namespace mc::text {

/**
 * @brief 文本组件工具函数
 *
 * 提供对文本组件的通用操作，参考 MC 1.16.5: net.minecraft.util.text.ComponentUtils
 */
namespace ComponentUtils {

/**
 * @brief 将文本组件包裹在方括号中
 *
 * 使用翻译键 "chat.square_brackets"（即 "[%s]"）将传入的组件包裹在方括号中。
 * 这确保了方括号格式可以通过语言文件自定义（例如不同语言的括号样式）。
 *
 * 参考 MC: net.minecraft.util.text.ComponentUtils.wrapInSquareBrackets
 *
 * @param component 要包裹的文本组件
 * @return 包裹在方括号中的新文本组件
 */
inline std::unique_ptr<ITextComponent> wrapInSquareBrackets(std::unique_ptr<ITextComponent> component)
{
    auto result = std::make_unique<TranslationTextComponent>("chat.square_brackets");
    result->addParam(std::move(component));
    return result;
}

} // namespace ComponentUtils

} // namespace mc::text
