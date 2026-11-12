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

#include "ComponentFactory.hpp"
#include "StringTextComponent.hpp"
#include <string>
#include <utility>

namespace mc::text {

std::unique_ptr<ITextComponent> literal(std::string text)
{
    return std::make_unique<StringTextComponent>(std::move(text));
}

std::unique_ptr<ITextComponent> translatable(std::string key)
{
    return std::make_unique<TranslationTextComponent>(std::move(key));
}

std::unique_ptr<ITextComponent> translatable(std::string key, std::vector<std::unique_ptr<ITextComponent>> params)
{
    return std::make_unique<TranslationTextComponent>(std::move(key), std::move(params));
}

std::unique_ptr<ITextComponent> translatable(std::string key, std::initializer_list<std::string> params)
{
    auto component = std::make_unique<TranslationTextComponent>(std::move(key));
    for (const auto& param : params) {
        component->addParam(literal(param));
    }
    return component;
}

std::unique_ptr<ITextComponent> translatable(std::string key, std::unique_ptr<ITextComponent> param)
{
    auto component = std::make_unique<TranslationTextComponent>(std::move(key));
    component->addParam(std::move(param));
    return component;
}

std::unique_ptr<ITextComponent> translatable(std::string key, std::string param)
{
    auto component = std::make_unique<TranslationTextComponent>(std::move(key));
    component->addParam(literal(std::move(param)));
    return component;
}

std::unique_ptr<ITextComponent> translatable(std::string key, i32 param)
{
    auto component = std::make_unique<TranslationTextComponent>(std::move(key));
    component->addParam(literal(std::to_string(param)));
    return component;
}

std::unique_ptr<ITextComponent> translatableEscape(std::string key, std::string param)
{
    // 原版在服务端把参数里的 `%` 转义为 `%%`，客户端替换占位符时还原为 `%`。
    std::string escaped;
    escaped.reserve(param.size());
    for (const char ch : param) {
        if (ch == '%') {
            escaped += "%%";
        } else {
            escaped += ch;
        }
    }

    auto component = std::make_unique<TranslationTextComponent>(std::move(key));
    component->addParam(literal(std::move(escaped)));
    return component;
}

} // namespace mc::text
