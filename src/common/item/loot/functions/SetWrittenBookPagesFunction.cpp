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

#include "SetWrittenBookPagesFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <memory>

namespace mc {
namespace loot {

namespace {
/// 成书页数上限（vanilla WrittenBookContent.PAGES_CODEC）
constexpr std::size_t MAX_WRITTEN_PAGES = 100;
} // namespace

SetWrittenBookPagesFunction::SetWrittenBookPagesFunction(nlohmann::json pages, ListOperationKind mode)
    : m_pages(std::move(pages))
    , m_mode(mode)
{}

ItemStack SetWrittenBookPagesFunction::run(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);
    if (stack.isEmpty() || !m_pages.is_array()) {
        return stack;
    }
    nlohmann::json pages = nlohmann::json::array();
    const nlohmann::json* tag = stack.getTag();
    if (tag != nullptr && tag->contains("written_book_content") && (*tag)["written_book_content"].contains("pages")) {
        pages = (*tag)["written_book_content"]["pages"];
    }
    if (!pages.is_array()) {
        pages = nlohmann::json::array();
    }
    switch (m_mode) {
        case ListOperationKind::ReplaceAll:
            pages = m_pages;
            break;
        case ListOperationKind::Append:
            for (const auto& page : m_pages) {
                pages.push_back(page);
            }
            break;
        case ListOperationKind::Insert:
            // 本项目不区分 insert 的偏移（数据包未使用），按追加处理
            for (const auto& page : m_pages) {
                pages.push_back(page);
            }
            break;
    }
    if (pages.size() > MAX_WRITTEN_PAGES) {
        while (pages.size() > MAX_WRITTEN_PAGES) {
            pages.erase(pages.begin() + static_cast<std::ptrdiff_t>(pages.size() - 1));
        }
    }
    stack.getOrCreateTag()["written_book_content"]["pages"] = std::move(pages);
    return stack;
}

std::unique_ptr<LootFunction> SetWrittenBookPagesFunction::clone() const noexcept
{
    auto func = std::make_unique<SetWrittenBookPagesFunction>(m_pages, m_mode);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
