#pragma once

#include "LootContext.hpp"
#include "LootPool.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 掉落表
 *
 * 定义实体死亡、方块破坏等情况下生成的物品。
 * 参考: net.minecraft.loot.LootTable
 *
 * 结构：
 * - 掉落表包含多个池（LootPool）
 * - 每个池定义掷骰次数和多个条目（LootEntry）
 * - 条目按权重随机选择
 * - 条目可以是物品、另一个掉落表引用等
 *
 * 示例用法：
 * @code
 * LootTable table;
 * auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
 * pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 2.0f), 10, 1));
 * table.addPool(std::move(pool));
 *
 * auto context = LootContextBuilder(world).build();
 * auto drops = table.generate(*context);
 * @endcode
 */
class LootTable {
public:
    /**
     * @brief 空掉落表常量
     */
    static const LootTable EMPTY;

    LootTable() = default;
    ~LootTable() = default;

    // 禁止拷贝
    LootTable(const LootTable&) = delete;
    LootTable& operator=(const LootTable&) = delete;

    // 允许移动
    LootTable(LootTable&&) = default;
    LootTable& operator=(LootTable&&) = default;

    // ========== 池管理 ==========

    /**
     * @brief 添加掉落池
     */
    void addPool(std::unique_ptr<LootPool> pool);

    /**
     * @brief 获取所有池
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootPool>>& getPools() const { return m_pools; }

    /**
     * @brief 根据名称获取池
     */
    [[nodiscard]] LootPool* getPool(const std::string& name);

    /**
     * @brief 移除池
     */
    std::unique_ptr<LootPool> removePool(const std::string& name);

    /**
     * @brief 获取池数量
     */
    [[nodiscard]] size_t poolCount() const { return m_pools.size(); }

    // ========== 参数集 ==========

    /**
     * @brief 获取参数集
     */
    [[nodiscard]] const LootParameterSet& getParameterSet() const { return m_paramSet; }

    /**
     * @brief 设置参数集
     */
    void setParameterSet(const LootParameterSet& paramSet) { m_paramSet = paramSet; }

    // ========== 标识符 ==========

    /**
     * @brief 获取掉落表ID
     */
    [[nodiscard]] const std::string& getId() const { return m_id; }

    /**
     * @brief 设置掉落表ID
     */
    void setId(const std::string& id) { m_id = id; }

    // ========== 生成 ==========

    /**
     * @brief 生成掉落物
     *
     * @param context 掉落上下文
     * @return 生成的物品列表
     */
    [[nodiscard]] std::vector<ItemStack> generate(LootContext& context) const;

    /**
     * @brief 生成掉落物到消费者
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     */
    void generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const;

    /**
     * @brief 递归生成（处理嵌套掉落表）
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     */
    void recursiveGenerate(std::function<void(const ItemStack&)> consumer, LootContext& context) const;

    // ========== 序列化 ==========

    /**
     * @brief 从JSON加载掉落表
     */
    [[nodiscard]] static Result<std::unique_ptr<LootTable>> fromJson(const std::string& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] std::string toJson() const;

private:
    std::string m_id;
    LootParameterSet m_paramSet;
    std::vector<std::unique_ptr<LootPool>> m_pools;
};

/**
 * @brief 掉落表构建器
 *
 * 参考: net.minecraft.loot.LootTable.Builder
 */
class LootTableBuilder {
public:
    LootTableBuilder() = default;

    /**
     * @brief 设置掉落表ID
     */
    LootTableBuilder& id(const std::string& id)
    {
        m_id = id;
        return *this;
    }

    /**
     * @brief 设置参数集
     */
    LootTableBuilder& paramSet(const LootParameterSet& paramSet)
    {
        m_paramSet = paramSet;
        return *this;
    }

    /**
     * @brief 添加池
     */
    LootTableBuilder& pool(std::unique_ptr<LootPool> pool)
    {
        m_pools.push_back(std::move(pool));
        return *this;
    }

    /**
     * @brief 构建掉落表
     */
    [[nodiscard]] std::unique_ptr<LootTable> build() const;

private:
    std::string m_id;
    LootParameterSet m_paramSet;
    std::vector<std::unique_ptr<LootPool>> m_pools;
};

/**
 * @brief 掉落表管理器
 *
 * 管理所有注册的掉落表。
 * 参考: net.minecraft.loot.LootTableManager
 */
class LootTableManager {
public:
    LootTableManager() = default;
    ~LootTableManager() = default;

    // 禁止拷贝
    LootTableManager(const LootTableManager&) = delete;
    LootTableManager& operator=(const LootTableManager&) = delete;

    // ========== 表管理 ==========

    /**
     * @brief 注册掉落表
     * @param id 掉落表ID（如 "minecraft:entities/pig"）
     * @param table 掉落表
     */
    void registerTable(const std::string& id, std::unique_ptr<LootTable> table);

    /**
     * @brief 获取掉落表
     * @param id 掉落表ID
     * @return 掉落表指针，不存在返回nullptr
     */
    [[nodiscard]] const LootTable* getTable(const std::string& id) const;

    /**
     * @brief 检查是否有掉落表
     */
    [[nodiscard]] bool hasTable(const std::string& id) const;

    /**
     * @brief 获取所有已注册的掉落表ID
     */
    [[nodiscard]] std::vector<std::string> getAllTableIds() const;

    // ========== 默认表 ==========

    /**
     * @brief 获取空掉落表
     */
    [[nodiscard]] static const LootTable& getEmptyTable();

    // ========== 内置掉落表 ==========

    /**
     * @brief 初始化内置掉落表
     */
    void initializeDefaultTables();

private:
    std::unordered_map<std::string, std::unique_ptr<LootTable>> m_tables;
};

} // namespace loot
} // namespace mc
