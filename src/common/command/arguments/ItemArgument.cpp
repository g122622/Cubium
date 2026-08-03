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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ItemArgument.hpp"

#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTag.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace command {

// ============================================================================
// ItemInput
// ============================================================================

std::unique_ptr<ItemStack> ItemInput::createStack(i32 count) const
{
    const Item* item = getItem();
    if (item == nullptr) {
        return nullptr;
    }
    return std::make_unique<ItemStack>(*item, count);
}

// ============================================================================
// ItemPredicateInput
// ============================================================================

bool ItemPredicateInput::test(const ItemStack& stack) const
{
    switch (m_mode) {
        case Mode::Any:
            // 通配符 * 匹配任意非空物品堆
            return !stack.isEmpty();

        case Mode::Item: {
            // 匹配特定物品ID
            const Item* expectedItem = ItemRegistry::instance().getItem(m_itemId);
            if (expectedItem == nullptr) {
                return false;
            }
            return stack.getItem() == expectedItem;
        }

        case Mode::Tag: {
            // 匹配物品标签
            item::tag::ItemTag* tag = item::tag::ItemTags::getTag(m_tagId);
            if (tag == nullptr) {
                // 未知标签不匹配任何物品
                return false;
            }
            return tag->contains(stack);
        }

        default:
            return false;
    }
}

std::string ItemPredicateInput::displayName() const
{
    switch (m_mode) {
        case Mode::Any:
            return "*";
        case Mode::Item: {
            const Item* item = getItem();
            if (item != nullptr) {
                return item->itemLocation().toString();
            }
            return "unknown_item";
        }
        case Mode::Tag:
            return "#" + m_tagId.toString();
        default:
            return "?";
    }
}

// ============================================================================
// ItemPredicateArgumentType
// ============================================================================

/**
 * @brief 判断字符是否为资源位置标识符中的合法字符
 *
 * 与 FunctionArgument 中的 readIdentifierGreedy() 使用相同的字符集，
 * 与 MC Java 的 Identifier.read() 一致。
 */
static bool isAllowedInIdentifier(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':' || c == '/';
}

/**
 * @brief 从 StringReader 中贪婪读取资源位置标识符
 */
static std::string readIdentifierGreedy(StringReader& reader)
{
    i32 start = reader.getCursor();
    while (reader.canRead() && isAllowedInIdentifier(reader.peek())) {
        reader.skip();
    }
    const auto startIdx = static_cast<size_t>(start);
    const auto endIdx = static_cast<size_t>(reader.getCursor());
    return std::string(reader.getString().substr(startIdx, endIdx - startIdx));
}

ItemPredicateInput ItemPredicateArgumentType::parse(StringReader& reader)
{
    i32 start = reader.getCursor();

    // 1. 通配符 *：匹配任意物品
    if (reader.canRead() && reader.peek() == '*') {
        reader.skip();
        return ItemPredicateInput(); // Mode::Any
    }

    // 2. 标签引用 #namespace:path：匹配标签中的所有物品
    if (reader.canRead() && reader.peek() == '#') {
        reader.skip(); // 跳过 '#'
        std::string str = readIdentifierGreedy(reader);
        if (str.empty()) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::StringExpected, "Expected item tag name after '#'", start);
        }
        ResourceLocation id = ResourceLocation::parse(str);
        return ItemPredicateInput(std::move(id)); // Mode::Tag
    }

    // 3. 特定物品 minecraft:stone 或 stone：匹配特定物品
    std::string str = reader.readString();

    // 解析命名空间
    std::string namespace_;
    std::string path;

    size_t colonPos = str.find(':');
    if (colonPos != std::string::npos) {
        namespace_ = str.substr(0, colonPos);
        path = str.substr(colonPos + 1);
    } else {
        namespace_ = "minecraft";
        path = str;
    }

    // 查找物品
    ResourceLocation location(namespace_, path);
    const Item* item = ItemRegistry::instance().getItem(location);

    if (item == nullptr) {
        reader.setCursor(start);
        throw CommandException(CommandErrorType::Unknown, "Unknown item: " + str, start);
    }

    return ItemPredicateInput(item->itemId()); // Mode::Item
}

} // namespace command
} // namespace mc
