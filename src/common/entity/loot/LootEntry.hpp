#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "LootContext.hpp"
#include "RandomRanges.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {
namespace loot {

// Forward declarations
class LootPool;
class LootContext;
class LootCondition;

/**
 * @brief 掉落条目类型
 *
 * 参考: net.minecraft.loot.LootPoolEntryType
 */
enum class LootEntryType : u8 {
    Empty,      // 空条目
    Item,       // 物品条目
    Tag,        // 标签条目
    Table,      // 掉落表引用
    Dynamic,    // 动态条目
    Alternatives, // 替代条目
    Sequence,   // 序列条目
    Group       // 组条目
};

/**
 * @brief 掉落条目基类
 *
 * 定义掉落表中单个条目的基础结构。
 * 参考: net.minecraft.loot.LootEntry
 */
class LootEntry {
public:
    virtual ~LootEntry();

    /**
     * @brief 获取条目类型
     */
    [[nodiscard]] virtual LootEntryType getType() const = 0;

    /**
     * @brief 创建副本
     */
    [[nodiscard]] virtual std::unique_ptr<LootEntry> clone() const = 0;

    /**
     * @brief 获取权重
     */
    [[nodiscard]] virtual i32 getWeight() const { return m_weight; }

    /**
     * @brief 获取有效权重（考虑幸运值）
     */
    [[nodiscard]] virtual i32 getEffectiveWeight(f32 luck) const {
        return m_weight + static_cast<i32>(luck * m_quality);
    }

    /**
     * @brief 获取质量（幸运值加成系数）
     */
    [[nodiscard]] i32 getQuality() const { return m_quality; }

    /**
     * @brief 设置权重
     */
    void setWeight(i32 weight) { m_weight = weight; }

    /**
     * @brief 设置质量
     */
    void setQuality(i32 quality) { m_quality = quality; }

    // ========== 条件管理 ==========

    /**
     * @brief 添加掉落条件
     *
     * 条件用于控制条目是否生效。所有条件都必须满足才能生成物品。
     *
     * @param condition 条件
     */
    void addCondition(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 获取所有条件
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const {
        return m_conditions;
    }

    /**
     * @brief 检查所有条件是否满足
     *
     * @param context 掉落上下文
     * @return 如果所有条件都满足返回true
     */
    [[nodiscard]] bool testConditions(LootContext& context) const;

    /**
     * @brief 扩展条目（生成候选列表）
     *
     * 将条目添加到候选列表中，用于加权随机选择。
     *
     * @param context 掉落上下文
     * @param consumer 接收候选条目的回调
     */
    virtual void expand(LootContext& context,
                       std::function<void(LootEntry&)> consumer) const = 0;

    /**
     * @brief 生成物品
     *
     * 执行条目的生成逻辑，将生成的物品传递给消费者。
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     * @return 是否成功生成（用于条件判断）
     */
    virtual bool generate(std::function<void(const ItemStack&)> consumer,
                         LootContext& context) const = 0;

protected:
    LootEntry() = default;
    explicit LootEntry(i32 weight, i32 quality = 0)
        : m_weight(weight), m_quality(quality) {}

    i32 m_weight = 1;
    i32 m_quality = 0;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

/**
 * @brief 空掉落条目
 *
 * 不生成任何物品。
 * 参考: net.minecraft.loot.EmptyLootEntry
 */
class EmptyLootEntry : public LootEntry {
public:
    EmptyLootEntry() = default;
    explicit EmptyLootEntry(i32 weight, i32 quality = 0)
        : LootEntry(weight, quality) {}

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Empty; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    void expand(LootContext& context,
               std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer,
                 LootContext& context) const override;
};

/**
 * @brief 物品掉落条目
 *
 * 生成特定物品。
 * 参考: net.minecraft.loot.ItemLootEntry
 */
class ItemLootEntry : public LootEntry {
public:
    /**
     * @brief 构造物品条目
     * @param itemId 物品ID
     * @param count 数量范围
     * @param weight 权重
     * @param quality 质量
     */
    ItemLootEntry(const String& itemId,
                  const RandomValueRange& count = RandomValueRange(1.0f, 1.0f),
                  i32 weight = 1, i32 quality = 0);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Item; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    [[nodiscard]] const String& getItemId() const { return m_itemId; }
    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }

    void expand(LootContext& context,
               std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer,
                 LootContext& context) const override;

    /**
     * @brief 设置数量范围
     */
    void setCount(const RandomValueRange& count) { m_count = count; }

private:
    String m_itemId;
    RandomValueRange m_count;
};

/**
 * @brief 掉落表引用条目
 *
 * 引用另一个掉落表。
 * 参考: net.minecraft.loot.TableLootEntry
 */
class TableLootEntry : public LootEntry {
public:
    explicit TableLootEntry(const String& tableId, i32 weight = 1, i32 quality = 0);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Table; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    [[nodiscard]] const String& getTableId() const { return m_tableId; }

    void expand(LootContext& context,
               std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer,
                 LootContext& context) const override;

private:
    String m_tableId;
};

/**
 * @brief 替代条目
 *
 * 尝试多个条目，直到一个成功。
 * 参考: net.minecraft.loot.AlternativesLootEntry
 */
class AlternativesLootEntry : public LootEntry {
public:
    AlternativesLootEntry() = default;
    explicit AlternativesLootEntry(std::vector<std::unique_ptr<LootEntry>> children);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Alternatives; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    void addChild(std::unique_ptr<LootEntry> child);

    void expand(LootContext& context,
               std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer,
                 LootContext& context) const override;

private:
    std::vector<std::unique_ptr<LootEntry>> m_children;
};

/**
 * @brief 序列条目
 *
 * 按顺序尝试所有子条目，直到一个失败。
 * 参考: net.minecraft.loot.SequenceLootEntry
 */
class SequenceLootEntry : public LootEntry {
public:
    SequenceLootEntry() = default;
    explicit SequenceLootEntry(std::vector<std::unique_ptr<LootEntry>> children);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Sequence; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    void addChild(std::unique_ptr<LootEntry> child);

    void expand(LootContext& context,
               std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer,
                 LootContext& context) const override;

private:
    std::vector<std::unique_ptr<LootEntry>> m_children;
};

/**
 * @brief 组条目
 *
 * 执行所有子条目。
 * 参考: net.minecraft.loot.GroupLootEntry
 */
class GroupLootEntry : public LootEntry {
public:
    GroupLootEntry() = default;
    explicit GroupLootEntry(std::vector<std::unique_ptr<LootEntry>> children);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Group; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    void addChild(std::unique_ptr<LootEntry> child);

    void expand(LootContext& context,
               std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer,
                 LootContext& context) const override;

private:
    std::vector<std::unique_ptr<LootEntry>> m_children;
};

/**
 * @brief 掉落条目构建器
 *
 * 参考: net.minecraft.loot.LootEntry.Builder
 */
class LootEntryBuilder {
public:
    LootEntryBuilder() = default;

    /**
     * @brief 设置权重
     */
    LootEntryBuilder& weight(i32 w) {
        m_weight = w;
        return *this;
    }

    /**
     * @brief 设置质量
     */
    LootEntryBuilder& quality(i32 q) {
        m_quality = q;
        return *this;
    }

    /**
     * @brief 构建物品条目
     */
    static LootEntryBuilder item(const String& itemId);

    /**
     * @brief 构建空条目
     */
    static LootEntryBuilder empty();

    /**
     * @brief 构建掉落表引用
     */
    static LootEntryBuilder table(const String& tableId);

    /**
     * @brief 设置数量
     */
    LootEntryBuilder& count(f32 min, f32 max);

    /**
     * @brief 设置固定数量
     */
    LootEntryBuilder& count(i32 value);

    /**
     * @brief 构建条目
     */
    [[nodiscard]] std::unique_ptr<LootEntry> build() const;

private:
    String m_itemId;
    String m_tableId;
    LootEntryType m_type = LootEntryType::Empty;
    RandomValueRange m_count{1.0f, 1.0f};
    i32 m_weight = 1;
    i32 m_quality = 0;
};

} // namespace loot
} // namespace mc
