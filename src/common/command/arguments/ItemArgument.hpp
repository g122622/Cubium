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

#include "ArgumentType.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class ItemStack;

namespace command {

/**
 * @brief 物品输入包装器
 *
 * 包装物品ID和可选的NBT数据，用于命令中指定物品
 */
class ItemInput {
public:
    ItemInput() = default;
    explicit ItemInput(ItemId itemId)
        : m_itemId(itemId)
    {}

    [[nodiscard]] ItemId itemId() const noexcept { return m_itemId; }
    [[nodiscard]] bool isValid() const noexcept { return m_itemId != 0; }

    /**
     * @brief 获取物品
     * @return 物品指针，如果无效返回 nullptr
     */
    [[nodiscard]] const Item* getItem() const
    {
        if (!isValid()) return nullptr;
        return ItemRegistry::instance().getItem(m_itemId);
    }

    /**
     * @brief 创建物品堆
     * @param count 数量
     * @return 物品堆
     */
    [[nodiscard]] std::unique_ptr<ItemStack> createStack(i32 count) const;

private:
    ItemId m_itemId = 0;
};

/**
 * @brief 物品参数类型
 *
 * 解析物品ID：
 * - minecraft:stone
 * - stone
 * - minecraft:diamond_sword
 *
 * 参考 MC 的 ItemArgument 类
 */
class ItemArgumentType : public ArgumentType<ItemInput> {
public:
    [[nodiscard]] ItemInput parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();

        // 读取物品名称
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

        return ItemInput(item->itemId());
    }

    [[nodiscard]] std::string getTypeName() const override { return "item"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:stone", "stone", "minecraft:diamond_sword", "diamond_sword"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<ItemArgumentType> item() { return std::make_shared<ItemArgumentType>(); }

    // ========== 静态获取方法 ==========

    template <typename S>
    static ItemInput getItem(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<ItemInput>(name);
    }
};

/**
 * @brief 物品谓词参数类型
 *
 * 用于检查物品是否匹配特定条件
 */
class ItemPredicateArgumentType : public ArgumentType<ItemInput> {
public:
    [[nodiscard]] ItemInput parse(StringReader& reader) override
    {
        // 与 ItemArgumentType 相同，但允许通配符等
        return ItemArgumentType().parse(reader);
    }

    [[nodiscard]] std::string getTypeName() const override { return "item_predicate"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:stone", "stone", "#minecraft:logs"};
    }

    static std::shared_ptr<ItemPredicateArgumentType> itemPredicate()
    {
        return std::make_shared<ItemPredicateArgumentType>();
    }
};

} // namespace command
} // namespace mc
