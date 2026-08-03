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
 * copies of substantial portions of the Software.
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

#include "FunctionSuggestionProvider.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "server/application/IServer.hpp"
#include "server/function/FunctionManager.hpp"
#include <future>

namespace mc {
namespace command {

std::future<Suggestions> FunctionSuggestionProvider::getSuggestions(
    CommandContext<ServerCommandSource>& context, SuggestionsBuilder& builder)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        return builder.buildFuture();
    }

    auto& functionManager = server->functionManager();

    // 建议函数名（直接引用格式：namespace:path）
    auto functionIds = functionManager.getAllFunctionIds();
    for (const auto& id : functionIds) {
        builder.suggest(id.toString());
    }

    // 建议标签名（标签引用格式：#namespace:path）
    auto tagIds = functionManager.getAllTagIds();
    for (const auto& id : tagIds) {
        builder.suggest("#" + id.toString());
    }

    return builder.buildFuture();
}

} // namespace command
} // namespace mc
