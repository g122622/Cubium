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

#include "LootContext.hpp"
#include "StatePropertiesPredicate.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {

// Forward declarations
class ItemStack;
class BlockState;
class BlockPos;

namespace loot {

/**
 * @brief 掉落条件基类
 *
 * 定义掉落表条件接口。条件用于控制掉落条目是否生效。
 * 参考: net.minecraft.loot.conditions.ILootCondition
 *
 * 示例用法:
 * @code
 * auto silkTouch = std::make_unique<SilkTouchCondition>();
 * auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond_ore");
 * entry->addCondition(std::move(silkTouch));
 * @endcode
 */
class LootCondition {
public:
    virtual ~LootCondition() = default;

    /**
     * @brief 测试条件是否满足
     *
     * @param context 掉落上下文
     * @return 如果条件满足返回true
     */
    [[nodiscard]] virtual bool test(LootContext& context) const = 0;

    /**
     * @brief 创建条件副本
     */
    [[nodiscard]] virtual std::unique_ptr<LootCondition> clone() const = 0;

    /**
     * @brief 获取条件类型标识
     */
    [[nodiscard]] virtual std::string getType() const = 0;
};

/**
 * @brief 精准采集条件
 *
 * 当工具具有精准采集附魔时满足条件。
 * 参考: net.minecraft.loot.conditions.SilkTouch
 *
 * 用于控制矿石等方块是否掉落原矿或普通物品:
 * - 有精准采集时: 掉落原矿
 * - 无精准采集时: 掉落普通物品（受时运影响）
 */
class SilkTouchCondition : public LootCondition {
public:
    SilkTouchCondition() = default;

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "silk_touch"; }
};

/**
 * @brief 时运条件
 *
 * 用于检测工具上的时运附魔等级。
 * 参考: net.minecraft.loot.conditions.TableBonus
 *
 * 时运附魔增加矿石掉落数量的概率。
 * 等级1-3，每个等级增加额外掉落的概率。
 */
class FortuneCondition : public LootCondition {
public:
    /**
     * @brief 构造时运条件
     * @param minLevel 最小时运等级（默认0，表示无时运也可满足）
     */
    explicit FortuneCondition(i32 minLevel = 0);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "fortune"; }

    /**
     * @brief 获取时运等级
     *
     * 从掉落上下文中获取时运附魔等级。
     *
     * @param context 掉落上下文
     * @return 时运等级（0-3）
     */
    [[nodiscard]] static i32 getFortuneLevel(LootContext& context);

    /**
     * @brief 计算时运加成后的掉落数量
     *
     * 参考 MC 1.16.5: Fortune对矿石的影响：
     * - Fortune I: 33%概率掉落+1
     * - Fortune II: 25%概率掉落+1, 25%概率掉落+2
     * - Fortune III: 20%概率掉落+1, 20%概率掉落+2, 20%概率掉落+3
     *
     * @param baseCount 基础掉落数量
     * @param fortuneLevel 时运等级（0-3）
     * @param random 随机数生成器
     * @return 加成后的掉落数量
     */
    [[nodiscard]] static i32 applyFortuneBonus(i32 baseCount, i32 fortuneLevel, math::Random& random);

private:
    i32 m_minLevel;
};

/**
 * @brief 随机概率条件
 *
 * 以指定概率满足条件。
 * 参考: net.minecraft.loot.conditions.RandomChance
 *
 * 用于控制掉落的随机性，如:
 * - 50%概率掉落某物品
 * - 受幸运值影响的概率
 */
class RandomChanceCondition : public LootCondition {
public:
    /**
     * @brief 构造随机概率条件
     * @param chance 概率值 (0.0 - 1.0)
     * @param affectedByLuck 是否受幸运值影响
     */
    explicit RandomChanceCondition(f32 chance, bool affectedByLuck = false);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "random_chance"; }

    [[nodiscard]] f32 getChance() const { return m_chance; }
    [[nodiscard]] bool isAffectedByLuck() const { return m_affectedByLuck; }

private:
    f32 m_chance;
    bool m_affectedByLuck;
};

/**
 * @brief 随机概率条件（受幸运值影响）
 *
 * 概率受幸运值影响的随机条件。
 * 参考: net.minecraft.loot.conditions.RandomChanceWithLooting
 *
 * 基础概率 + (幸运值 * 幸运系数)
 */
class RandomChanceWithLuckCondition : public LootCondition {
public:
    /**
     * @brief 构造条件
     * @param baseChance 基础概率
     * @param luckCoefficient 幸运系数（每点幸运增加的概率）
     */
    RandomChanceWithLuckCondition(f32 baseChance, f32 luckCoefficient);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "random_chance_with_luck"; }

    [[nodiscard]] f32 getBaseChance() const { return m_baseChance; }
    [[nodiscard]] f32 getLuckCoefficient() const { return m_luckCoefficient; }

private:
    f32 m_baseChance;
    f32 m_luckCoefficient;
};

/**
 * @brief 取反条件
 *
 * 对内部条件取反。
 * 参考: net.minecraft.loot.conditions.Inverted
 *
 * 用于创建"不满足某条件"的情况，如:
 * - 无精准采集时才掉落普通矿石
 */
class NotCondition : public LootCondition {
public:
    /**
     * @brief 构造取反条件
     * @param condition 要取反的条件
     */
    explicit NotCondition(std::unique_ptr<LootCondition> condition);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "inverted"; }

    [[nodiscard]] const LootCondition* getInnerCondition() const { return m_condition.get(); }

private:
    std::unique_ptr<LootCondition> m_condition;
};

/**
 * @brief 与条件
 *
 * 所有子条件都满足时才满足。
 * 参考: net.minecraft.loot.conditions.Alternative
 */
class AndCondition : public LootCondition {
public:
    AndCondition() = default;
    explicit AndCondition(std::vector<std::unique_ptr<LootCondition>> conditions);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "alternative"; }

    void addCondition(std::unique_ptr<LootCondition> condition);
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

private:
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

/**
 * @brief 或条件
 *
 * 任一子条件满足即满足。
 * 参考: net.minecraft.loot.conditions.Alternative (使用 OR 逻辑)
 */
class OrCondition : public LootCondition {
public:
    OrCondition() = default;
    explicit OrCondition(std::vector<std::unique_ptr<LootCondition>> conditions);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "or"; }

    void addCondition(std::unique_ptr<LootCondition> condition);
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

private:
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

/**
 * @brief 方块状态条件
 *
 * 检查被破坏的方块是否为指定类型，并可选地检查方块属性值。
 *
 * 支持：
 * - 精确匹配：检查属性值是否等于指定值
 * - 范围匹配：检查属性值是否在指定范围内（适用于整数属性）
 *
 * 参考: net.minecraft.loot.conditions.BlockStateProperty
 *
 * JSON 格式示例:
 * @code
 * // 仅检查方块类型
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:beetroots"
 * }
 *
 * // 检查方块类型和精确属性
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:beetroots",
 *   "properties": { "age": "3" }
 * }
 *
 * // 检查方块类型和范围属性
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:wheat",
 *   "properties": { "age": { "min": "5", "max": "7" } }
 * }
 *
 * // 多属性检查
 * {
 *   "condition": "minecraft:block_state_property",
 *   "block": "minecraft:oak_door",
 *   "properties": { "open": "true", "facing": "north" }
 * }
 * @endcode
 */
class BlockStateCondition : public LootCondition {
public:
    /**
     * @brief 构造方块状态条件（仅检查方块ID）
     * @param blockId 方块ID（如 "minecraft:diamond_ore"）
     */
    explicit BlockStateCondition(const std::string& blockId);

    /**
     * @brief 构造方块状态条件（检查方块ID和属性）
     * @param blockId 方块ID（如 "minecraft:beetroots"）
     * @param properties 属性匹配谓词
     */
    BlockStateCondition(const std::string& blockId, StatePropertiesPredicate properties);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "block_state_property"; }

    [[nodiscard]] const std::string& getBlockId() const { return m_blockId; }
    [[nodiscard]] const StatePropertiesPredicate& getProperties() const { return m_properties; }

private:
    std::string m_blockId;
    StatePropertiesPredicate m_properties;
};

/**
 * @brief 工具类型条件
 *
 * 检查使用的工具是否为指定类型。
 * 用于需要特定工具才能获得掉落的情况。
 */
class ToolTypeCondition : public LootCondition {
public:
    /**
     * @brief 构造工具类型条件
     * @param toolType 工具类型（HarvestTool 常量）
     */
    explicit ToolTypeCondition(u8 toolType);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "match_tool"; }

    [[nodiscard]] u8 getToolType() const { return m_toolType; }

private:
    u8 m_toolType;
};

/**
 * @brief 掉落条件构建器
 *
 * 提供流畅的条件构建接口。
 *
 * 示例:
 * @code
 * auto condition = LootConditionBuilder::silkTouch();
 * auto notSilkTouch = LootConditionBuilder::not_(LootConditionBuilder::silkTouch());
 * auto orCondition = LootConditionBuilder::or_({
 *     LootConditionBuilder::silkTouch(),
 *     LootConditionBuilder::toolType(HarvestTool::Pickaxe)
 * });
 * @endcode
 */
class LootConditionBuilder {
public:
    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建精准采集条件
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> silkTouch();

    /**
     * @brief 创建时运条件
     * @param minLevel 最小时运等级
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> fortune(i32 minLevel = 0);

    /**
     * @brief 创建随机概率条件
     * @param chance 概率值 (0.0 - 1.0)
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> randomChance(f32 chance);

    /**
     * @brief 创建受幸运影响的随机概率条件
     * @param baseChance 基础概率
     * @param luckCoefficient 幸运系数
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> randomChanceWithLuck(f32 baseChance, f32 luckCoefficient);

    /**
     * @brief 创建取反条件
     * @param condition 要取反的条件
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> not_(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 创建与条件
     * @param conditions 子条件列表
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> and_(std::vector<std::unique_ptr<LootCondition>> conditions);

    /**
     * @brief 创建或条件
     * @param conditions 子条件列表
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> or_(std::vector<std::unique_ptr<LootCondition>> conditions);

    /**
     * @brief 创建方块状态条件（仅检查方块ID）
     * @param blockId 方块ID
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> blockState(const std::string& blockId);

    /**
     * @brief 创建方块状态条件（检查方块ID和属性）
     * @param blockId 方块ID
     * @param properties 属性匹配谓词
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> blockState(
        const std::string& blockId, StatePropertiesPredicate properties);

    /**
     * @brief 创建工具类型条件
     * @param toolType 工具类型
     */
    [[nodiscard]] static std::unique_ptr<LootCondition> toolType(u8 toolType);
};

/**
 * @brief 钓鱼开放水域条件
 *
 * 检查钓鱼是否在开放水域进行。
 * 用于钓鱼掉落表中宝藏条目的条件判断。
 *
 * 参考: net.minecraft.loot.conditions.EntityProperties + FishingPredicate
 *
 * MC 1.16.5 中，宝藏只有在开放水域才能钓到。
 * 开放水域定义：浮标周围 5x4x5 区域（X-2到X+2，Y-1到Y+2，Z-2到Z+2）
 * - 水面上方层：必须是空气或睡莲
 * - 水层：必须是水源方块
 */
class FishingOpenWaterCondition : public LootCondition {
public:
    /**
     * @brief 构造开放水域条件
     * @param requireOpenWater 是否需要开放水域（默认 true）
     */
    explicit FishingOpenWaterCondition(bool requireOpenWater = true);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "fishing_hook_in_open_water"; }

    [[nodiscard]] bool requireOpenWater() const { return m_requireOpenWater; }

private:
    bool m_requireOpenWater;
};

} // namespace loot
} // namespace mc
