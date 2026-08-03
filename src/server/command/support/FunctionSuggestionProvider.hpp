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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/command/suggestions/Suggestions.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <future>

namespace mc {
namespace command {

/**
 * @brief 函数参数的 Tab 补全建议提供器
 *
 * 在建议阶段查询 FunctionManager，提供已注册的函数名和标签名（带 # 前缀）。
 * 参考 MC Java 的 SUGGEST_FUNCTION 建议。
 */
class FunctionSuggestionProvider : public ISuggestionProvider<ServerCommandSource> {
public:
    std::future<Suggestions> getSuggestions(
        CommandContext<ServerCommandSource>& context, SuggestionsBuilder& builder) override;
};

} // namespace command
} // namespace mc
