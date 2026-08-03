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
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
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
    ItemInput() noexcept = default;
    explicit ItemInput(ItemId itemId) noexcept
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

    [[nodiscard]] std::string getTypeName() const noexcept override { return "item"; }

    [[nodiscard]] std::vector<std::string> getExamples() const noexcept override
    {
        return {"minecraft:stone", "stone", "minecraft:diamond_sword", "diamond_sword"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<ItemArgumentType> item() noexcept { return std::make_shared<ItemArgumentType>(); }

    // ========== 静态获取方法 ==========

    template <typename S>
    static ItemInput getItem(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<ItemInput>(name);
    }
};

/**
 * @brief 物品谓词输入
 *
 * 用于命令中指定物品匹配条件，支持三种匹配模式：
 * - 特定物品：minecraft:stone、stone — 精确匹配物品ID
 * - 物品标签：#minecraft:logs — 匹配标签中的所有物品
 * - 通配符：* — 匹配任意物品
 *
 * 对应 MC Java 的 ItemPredicateArgument.Result（Predicate<ItemStack>）。
 * 与 ItemInput 不同，ItemPredicateInput 是一个谓词/匹配器，而非单一物品引用。
 */
class ItemPredicateInput {
public:
    /**
     * @brief 匹配模式
     */
    enum class Mode : u8 {
        Any,  ///< 通配符 *，匹配任意物品
        Item, ///< 特定物品ID匹配
        Tag,  ///< 物品标签匹配（# 前缀）
    };

    /** @brief 默认构造（Any 模式，匹配任意物品） */
    ItemPredicateInput() noexcept
        : m_mode(Mode::Any)
    {}

    /**
     * @brief 构造特定物品匹配
     * @param itemId 物品ID
     */
    explicit ItemPredicateInput(ItemId itemId) noexcept
        : m_mode(Mode::Item)
        , m_itemId(itemId)
    {}

    /**
     * @brief 构造标签匹配
     * @param tagId 标签资源位置
     */
    explicit ItemPredicateInput(ResourceLocation tagId) noexcept
        : m_mode(Mode::Tag)
        , m_tagId(std::move(tagId))
    {}

    /**
     * @brief 构造指定模式的匹配
     * @param mode 匹配模式
     * @param itemId 物品ID（Item 模式使用）
     * @param tagId 标签资源位置（Tag 模式使用）
     */
    ItemPredicateInput(Mode mode, ItemId itemId, ResourceLocation tagId) noexcept
        : m_mode(mode)
        , m_itemId(itemId)
        , m_tagId(std::move(tagId))
    {}

    /** @brief 获取匹配模式 */
    [[nodiscard]] Mode mode() const noexcept { return m_mode; }

    /** @brief 是否为通配符模式 */
    [[nodiscard]] bool isAny() const noexcept { return m_mode == Mode::Any; }

    /** @brief 是否为物品ID模式 */
    [[nodiscard]] bool isItem() const noexcept { return m_mode == Mode::Item; }

    /** @brief 是否为标签模式 */
    [[nodiscard]] bool isTag() const noexcept { return m_mode == Mode::Tag; }

    /** @brief 获取物品ID（仅 Item 模式有效） */
    [[nodiscard]] ItemId itemId() const noexcept { return m_itemId; }

    /** @brief 获取标签ID（仅 Tag 模式有效） */
    [[nodiscard]] const ResourceLocation& tagId() const noexcept { return m_tagId; }

    /**
     * @brief 获取物品指针（仅 Item 模式有效）
     * @return 物品指针，无效模式或未注册物品返回 nullptr
     */
    [[nodiscard]] const Item* getItem() const
    {
        if (m_mode != Mode::Item) return nullptr;
        return ItemRegistry::instance().getItem(m_itemId);
    }

    /**
     * @brief 测试物品堆是否匹配此谓词
     * @param stack 物品堆
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 获取格式化的显示名称
     *
     * 通配符返回 "*"，标签返回 "#namespace:path"，物品返回 "namespace:path"
     */
    [[nodiscard]] std::string displayName() const;

private:
    Mode m_mode = Mode::Any;
    ItemId m_itemId = 0;
    ResourceLocation m_tagId;
};

/**
 * @brief 物品谓词参数类型
 *
 * 用于命令中指定物品匹配条件，支持三种格式：
 * - "minecraft:stone" 或 "stone" — 匹配特定物品
 * - "#minecraft:logs" — 匹配标签中的所有物品
 * - "*" — 匹配任意物品
 *
 * 对应 MC Java 的 ItemPredicateArgument。
 * TODO: 后续随数据组件系统一起添加组件测试语法支持（stick[custom_data={...}]、
 * stick[!custom_data]、stick[~custom_data={...}]等MC Java 1.21+语法）
 */
class ItemPredicateArgumentType : public ArgumentType<ItemPredicateInput> {
public:
    [[nodiscard]] ItemPredicateInput parse(StringReader& reader) override;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "item_predicate"; }

    [[nodiscard]] std::vector<std::string> getExamples() const noexcept override
    {
        return {"minecraft:stone", "stone", "#minecraft:logs", "*"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<ItemPredicateArgumentType> itemPredicate() noexcept
    {
        return std::make_shared<ItemPredicateArgumentType>();
    }

    // ========== 静态获取方法 ==========

    /**
     * @brief 从命令上下文中获取解析后的物品谓词
     * @tparam S 命令源类型
     * @param context 命令上下文
     * @param name 参数名
     * @return 物品谓词输入
     */
    template <typename S>
    static ItemPredicateInput getItemPredicate(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<ItemPredicateInput>(name);
    }
};

} // namespace command
} // namespace mc
