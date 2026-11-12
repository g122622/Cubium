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

#include "common/core/Types.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace mc::text {

/**
 * @brief 文本组件工厂
 *
 * 统一构造面向玩家展示的文本组件，避免各处手写 `std::make_unique<...>` 时出现
 * 「本该用翻译组件却用了纯文本」的退化（那会让客户端无法按自己的语言本地化）。
 * 参数一律显式传递，不设默认值。
 */

/**
 * @brief 构造纯文本组件
 * @param text 文本内容
 */
[[nodiscard]] std::unique_ptr<ITextComponent> literal(std::string text);

/**
 * @brief 构造无参数翻译组件
 * @param key 翻译键
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatable(std::string key);

/**
 * @brief 构造带组件参数的翻译组件
 * @param key 翻译键
 * @param params 参数组件列表
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatable(
    std::string key, std::vector<std::unique_ptr<ITextComponent>> params);

/**
 * @brief 构造带若干字符串参数的翻译组件
 * @param key 翻译键
 * @param params 字符串参数列表（每项按纯文本组件传递）
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatable(std::string key, std::initializer_list<std::string> params);

/**
 * @brief 构造带单个组件参数的翻译组件
 * @param key 翻译键
 * @param param 参数组件（所有权转移）
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatable(std::string key, std::unique_ptr<ITextComponent> param);

/**
 * @brief 构造带单个字符串参数的翻译组件
 * @param key 翻译键
 * @param param 字符串参数（按纯文本组件传递）
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatable(std::string key, std::string param);

/**
 * @brief 构造带单个整数参数的翻译组件
 * @param key 翻译键
 * @param param 整数参数（十进制文本）
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatable(std::string key, i32 param);

/**
 * @brief 构造带转义字符串参数的翻译组件
 *
 * 对应原版 `Component.translatableEscape`：把参数里的 `%` 转义成 `%%`，
 * 避免玩家名等外部文本被当成占位符（如计分板目标名含 `%`）。
 *
 * @param key 翻译键
 * @param param 字符串参数（其中的 `%` 会被转义）
 */
[[nodiscard]] std::unique_ptr<ITextComponent> translatableEscape(std::string key, std::string param);

} // namespace mc::text
