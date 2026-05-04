#pragma once

#include "common/core/Types.hpp"
#include "LootContext.hpp"
#include "LootConditions.hpp"
#include "RandomRanges.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace mc {

// Forward declarations
class ItemStack;
class Entity;
class Player;
class BlockState;

namespace loot {

/**
 * @brief 掉落函数基类
 *
 * 掉落函数用于修改生成的物品堆，如设置数量、应用附魔加成等。
 * 参考: net.minecraft.loot.functions.ILootFunction
 *
 * 函数在条件检查之后、物品返回之前执行。
 * 多个函数按顺序执行，前一个函数的输出作为后一个函数的输入。
 */
class LootFunction {
public:
    virtual ~LootFunction() = default;

    /**
     * @brief 应用函数到物品堆
     *
     * 修改或替换物品堆。
     *
     * @param stack 原始物品堆（可被修改）
     * @param context 掉落上下文
     * @return 修改后的物品堆（可以返回空堆表示不生成物品）
     */
    virtual ItemStack apply(ItemStack stack, LootContext& context) const = 0;

    /**
     * @brief 创建函数副本
     */
    [[nodiscard]] virtual std::unique_ptr<LootFunction> clone() const = 0;

    /**
     * @brief 获取函数类型标识
     */
    [[nodiscard]] virtual String getType() const = 0;

    // ========== 条件管理 ==========

    /**
     * @brief 添加条件
     *
     * 只有所有条件都满足时，函数才会执行。
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
     */
    [[nodiscard]] bool testConditions(LootContext& context) const;

protected:
    LootFunction() = default;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

/**
 * @brief 设置数量函数
 *
 * 设置物品的数量。
 * 参考: net.minecraft.loot.functions.SetCount
 *
 * 可以设置固定数量或随机范围。
 * 受幸运值影响的额外数量可以通过 quality 参数设置。
 */
class SetCountFunction : public LootFunction {
public:
    /**
     * @brief 构造设置数量函数
     * @param count 数量范围
     * @param add 是否在原有数量上增加（默认false，替换）
     */
    explicit SetCountFunction(const RandomValueRange& count, bool add = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_count"; }

    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }
    [[nodiscard]] bool isAdd() const { return m_add; }

private:
    RandomValueRange m_count;
    bool m_add;
};

/**
 * @brief 时运加成函数
 *
 * 根据时运附魔等级增加掉落数量。
 * 参考: net.minecraft.loot.functions.ApplyBonus
 *
 * 用于矿石、农作物等方块的掉落表。
 * MC 1.16.5 时运算法：
 * - Fortune I: 33% 概率 +1
 * - Fortune II: 25% 概率 +1, 25% 概率 +2
 * - Fortune III: 20% 概率 +1, 20% 概率 +2, 20% 概率 +3
 */
class ApplyBonusFunction : public LootFunction {
public:
    /**
     * @brief 加成类型
     *
     * 参考 MC 1.16.5 的三种加成公式
     */
    enum class BonusType : u8 {
        Uniform,        // 均匀分布: count + random(0, bonusMultiplier * fortune)
        Binomial,       // 二项分布: count + binomial(fortune + extra, probability)
        OreDrops        // 矿石掉落: count * random(1, fortune + 1)
    };

    /**
     * @brief 构造时运加成函数
     * @param bonusType 加成类型
     * @param bonusMultiplier Uniform 类型的乘数（默认1）
     * @param extra Binomial 类型的额外试验次数（默认1）
     * @param probability Binomial 类型的成功概率
     */
    explicit ApplyBonusFunction(BonusType bonusType = BonusType::OreDrops,
                                i32 bonusMultiplier = 1,
                                i32 extra = 1,
                                f32 probability = 1.0f);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "apply_bonus"; }

    [[nodiscard]] BonusType getBonusType() const { return m_bonusType; }
    [[nodiscard]] i32 getBonusMultiplier() const { return m_bonusMultiplier; }
    [[nodiscard]] i32 getExtra() const { return m_extra; }
    [[nodiscard]] f32 getProbability() const { return m_probability; }

    /**
     * @brief 计算时运加成数量（矿石掉落算法）
     *
     * MC 1.16.5 OreDropsFormula:
     * if (fortune > 0) {
     *     int i = random.nextInt(fortune + 2) - 1;
     *     if (i < 0) i = 0;
     *     return baseCount * (i + 1);
     * } else {
     *     return baseCount;
     * }
     *
     * @param baseCount 基础掉落数量
     * @param fortuneLevel 时运等级 (0-3)
     * @param random 随机数生成器
     * @return 掉落数量
     */
    [[nodiscard]] static i32 calculateOreDrops(i32 baseCount, i32 fortuneLevel, math::Random& random);

    /**
     * @brief 计算均匀分布加成
     *
     * MC 1.16.5 UniformBonusCountFormula:
     * count + random.nextInt(bonusMultiplier * fortune + 1)
     *
     * @param baseCount 基础掉落数量
     * @param fortuneLevel 时运等级
     * @param bonusMultiplier 乘数（默认1）
     * @param random 随机数生成器
     * @return 掉落数量
     */
    [[nodiscard]] static i32 calculateUniformBonus(i32 baseCount, i32 fortuneLevel, i32 bonusMultiplier, math::Random& random);

    /**
     * @brief 计算二项分布加成
     *
     * MC 1.16.5 BinomialWithBonusCountFormula:
     * for (int i = 0; i < fortune + extra; ++i) {
     *     if (random.nextFloat() < probability) ++count;
     * }
     *
     * @param baseCount 基础掉落数量
     * @param fortuneLevel 时运等级
     * @param extra 额外试验次数（默认1）
     * @param probability 成功概率
     * @param random 随机数生成器
     * @return 掉落数量
     */
    [[nodiscard]] static i32 calculateBinomialBonus(i32 baseCount, i32 fortuneLevel, i32 extra, f32 probability, math::Random& random);

private:
    BonusType m_bonusType;
    i32 m_bonusMultiplier;  // Uniform 类型使用
    i32 m_extra;            // Binomial 类型使用
    f32 m_probability;      // Binomial 类型使用
};

/**
 * @brief 掠夺附魔加成函数
 *
 * 根据掠夺附魔等级增加掉落数量。
 * 参考: net.minecraft.loot.functions.LootingEnchantBonus
 *
 * 公式: count + random(0, lootingLevel) * lootingMultiplier
 * 或者使用随机范围: count + random(min, max) per looting level
 *
 * 用于怪物掉落（如腐肉、骨头等）。
 */
class LootingEnchantBonusFunction : public LootFunction {
public:
    /**
     * @brief 构造掠夺加成函数
     * @param count 额外数量范围
     * @param limit 最大数量限制（0表示无限制）
     */
    explicit LootingEnchantBonusFunction(const RandomValueRange& count = RandomValueRange(0.0f, 1.0f),
                                         i32 limit = 0);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "looting_enchant"; }

    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }
    [[nodiscard]] i32 getLimit() const { return m_limit; }

private:
    RandomValueRange m_count;
    i32 m_limit;  // 0 表示无限制
};

/**
 * @brief 设置损坏函数
 *
 * 设置物品的耐久度损坏程度。
 * 参考: net.minecraft.loot.functions.SetDamage
 *
 * 用于工具、武器等可损坏物品的掉落。
 */
class SetDamageFunction : public LootFunction {
public:
    /**
     * @brief 构造设置损坏函数
     * @param durability 损坏程度范围 (0.0 = 完好, 1.0 = 完全损坏)
     * @param add 是否在原有损坏基础上增加（默认false，替换）
     */
    explicit SetDamageFunction(const RandomValueRange& durability, bool add = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_damage"; }

    [[nodiscard]] const RandomValueRange& getDurability() const { return m_durability; }
    [[nodiscard]] bool isAdd() const { return m_add; }

private:
    RandomValueRange m_durability;
    bool m_add;
};

/**
 * @brief 设置名称函数
 *
 * 设置物品的自定义名称。
 * 参考: net.minecraft.loot.functions.SetName
 */
class SetNameFunction : public LootFunction {
public:
    /**
     * @brief 构造设置名称函数
     * @param name 自定义名称（JSON 文本格式或纯文本）
     * @param replace 是否替换原有名称（默认true）
     */
    explicit SetNameFunction(const String& name, bool replace = true);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_name"; }

    [[nodiscard]] const String& getName() const { return m_name; }
    [[nodiscard]] bool isReplace() const { return m_replace; }

private:
    String m_name;
    bool m_replace;
};

/**
 * @brief 设置描述函数
 *
 * 设置物品的 Lore 描述。
 * 参考: net.minecraft.loot.functions.SetLore
 */
class SetLoreFunction : public LootFunction {
public:
    /**
     * @brief 构造设置描述函数
     * @param lore 描述行列表
     * @param replace 是否替换原有描述（默认true）
     */
    explicit SetLoreFunction(const std::vector<String>& lore, bool replace = true);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_lore"; }

    [[nodiscard]] const std::vector<String>& getLore() const { return m_lore; }
    [[nodiscard]] bool isReplace() const { return m_replace; }

private:
    std::vector<String> m_lore;
    bool m_replace;
};

/**
 * @brief 限制数量函数
 *
 * 限制物品数量的最小和最大值。
 * 参考: net.minecraft.loot.functions.LimitCount
 *
 * 用于确保掉落数量在合理范围内。
 */
class LimitCountFunction : public LootFunction {
public:
    /**
     * @brief 构造限制数量函数
     * @param min 最小数量（-1 表示无下限）
     * @param max 最大数量（-1 表示无上限）
     */
    explicit LimitCountFunction(i32 min = -1, i32 max = -1);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "limit_count"; }

    [[nodiscard]] i32 getMin() const { return m_min; }
    [[nodiscard]] i32 getMax() const { return m_max; }

private:
    i32 m_min;
    i32 m_max;
};

/**
 * @brief 熔炼函数
 *
 * 如果物品可以被熔炼，则返回熔炼后的物品。
 * 参考: net.minecraft.loot.functions.FurnaceSmelt
 *
 * 用于检测掉落是否可以被熔炼，如果可以则返回熔炼产物。
 */
class FurnaceSmeltFunction : public LootFunction {
public:
    FurnaceSmeltFunction() = default;

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "furnace_smelt"; }
};

/**
 * @brief 附魔函数
 *
 * 随机附魔物品。
 * 参考: net.minecraft.loot.functions.EnchantWithLevels
 *
 * 用于生成附魔书籍、装备等。
 */
class EnchantWithLevelsFunction : public LootFunction {
public:
    /**
     * @brief 构造附魔函数
     * @param levels 附魔等级范围
     * @param treasure 是否包含宝藏附魔
     */
    explicit EnchantWithLevelsFunction(const RandomValueRange& levels, bool treasure = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "enchant_with_levels"; }

    [[nodiscard]] const RandomValueRange& getLevels() const { return m_levels; }
    [[nodiscard]] bool isTreasure() const { return m_treasure; }

private:
    RandomValueRange m_levels;
    bool m_treasure;
};

/**
 * @brief 随机附魔函数
 *
 * 随机选择附魔类型和等级。
 * 参考: net.minecraft.loot.functions.EnchantRandomly
 */
class EnchantRandomlyFunction : public LootFunction {
public:
    /**
     * @brief 构造随机附魔函数
     * @param enchantments 可选的附魔ID列表（空表示随机选择所有适用附魔）
     * @param treasure 是否包含宝藏附魔
     */
    explicit EnchantRandomlyFunction(const std::vector<String>& enchantments = {}, bool treasure = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "enchant_randomly"; }

    [[nodiscard]] const std::vector<String>& getEnchantments() const { return m_enchantments; }
    [[nodiscard]] bool isTreasure() const { return m_treasure; }

private:
    std::vector<String> m_enchantments;
    bool m_treasure;
};

/**
 * @brief 爆炸衰减函数
 *
 * 控制爆炸时物品的消失概率。
 * 参考: net.minecraft.loot.functions.ExplosionDecay
 *
 * 用于 TNT 爆炸、苦力怕爆炸等场景的掉落物处理。
 * 每个物品有 1/explosionRadius 的概率保留。
 */
class ExplosionDecayFunction : public LootFunction {
public:
    ExplosionDecayFunction() = default;

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "explosion_decay"; }
};

/**
 * @brief 设置NBT标签函数
 *
 * 设置物品的自定义NBT数据。
 * 参考: net.minecraft.loot.functions.SetNBT
 *
 * 用于设置物品的自定义数据，如药水效果、显示名称等。
 */
class SetNbtFunction : public LootFunction {
public:
    /**
     * @brief 构造设置NBT函数
     * @param nbtString NBT标签字符串（JSON格式）
     */
    explicit SetNbtFunction(const String& nbtString);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_nbt"; }

    [[nodiscard]] const String& getNbtString() const { return m_nbtString; }

private:
    String m_nbtString;
};

/**
 * @brief 复制名称函数
 *
 * 从掉落源复制名称到物品。
 * 参考: net.minecraft.loot.functions.CopyName
 *
 * 用于命名实体掉落物品时保留名称（如命名生物掉落的物品）。
 */
class CopyNameFunction : public LootFunction {
public:
    /**
     * @brief 名称来源
     */
    enum class Source : u8 {
        This,           // 当前实体
        Killer,         // 击杀者
        KillerPlayer,   // 击杀玩家
        BlockEntity     // 方块实体
    };

    /**
     * @brief 构造复制名称函数
     * @param source 名称来源
     */
    explicit CopyNameFunction(Source source);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "copy_name"; }

    [[nodiscard]] Source getSource() const { return m_source; }

private:
    Source m_source;
};

/**
 * @brief 复制方块状态函数
 *
 * 从被破坏的方块复制状态属性到物品。
 * 参考: net.minecraft.loot.functions.CopyBlockState
 *
 * 用于保留方块的某些状态属性（如容器的朝向）。
 */
class CopyBlockStateFunction : public LootFunction {
public:
    /**
     * @brief 构造复制方块状态函数
     * @param blockId 方块ID
     * @param properties 要复制的属性名列表（空表示复制所有）
     */
    explicit CopyBlockStateFunction(const String& blockId,
                                    const std::vector<String>& properties = {});

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "copy_block_state"; }

    [[nodiscard]] const String& getBlockId() const { return m_blockId; }
    [[nodiscard]] const std::vector<String>& getProperties() const { return m_properties; }

private:
    String m_blockId;
    std::vector<String> m_properties;
};

/**
 * @brief 复制NBT函数
 *
 * 从掉落源复制NBT数据到物品。
 * 参考: net.minecraft.loot.functions.CopyNbt
 *
 * 用于复制实体或方块的NBT数据到掉落物品。
 */
class CopyNbtFunction : public LootFunction {
public:
    /**
     * @brief NBT来源
     */
    enum class Source : u8 {
        This,           // 当前实体
        Killer,         // 击杀者
        KillerPlayer,   // 击杀玩家
        BlockEntity     // 方块实体
    };

    /**
     * @brief NBT操作类型
     */
    enum class Operation : u8 {
        Replace,    // 替换
        Append,     // 追加
        Merge       // 合并
    };

    /**
     * @brief NBT操作定义
     */
    struct NbtOperation {
        String sourcePath;
        String targetPath;
        Operation operation;
    };

    /**
     * @brief 构造复制NBT函数
     * @param source NBT来源
     */
    explicit CopyNbtFunction(Source source);

    /**
     * @brief 添加NBT操作
     */
    void addOperation(const String& sourcePath, const String& targetPath, Operation operation);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "copy_nbt"; }

    [[nodiscard]] Source getSource() const { return m_source; }
    [[nodiscard]] const std::vector<NbtOperation>& getOperations() const { return m_operations; }

private:
    Source m_source;
    std::vector<NbtOperation> m_operations;
};

/**
 * @brief 填充玩家头颅函数
 *
 * 用玩家信息填充玩家头颅物品。
 * 参考: net.minecraft.loot.functions.FillPlayerHead
 *
 * 用于生成玩家头颅时填充皮肤信息。
 */
class FillPlayerHeadFunction : public LootFunction {
public:
    /**
     * @brief 构造填充玩家头颅函数
     * @param source 玩家来源（This/Killer/KillerPlayer）
     */
    explicit FillPlayerHeadFunction(CopyNameFunction::Source source);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "fill_player_head"; }

    [[nodiscard]] CopyNameFunction::Source getSource() const { return m_source; }

private:
    CopyNameFunction::Source m_source;
};

/**
 * @brief 设置属性函数
 *
 * 给物品添加属性修饰符。
 * 参考: net.minecraft.loot.functions.SetAttributes
 *
 * 用于生成带有特定属性的装备。
 * 注意：需要属性系统支持，当前为桩实现。
 */
class SetAttributesFunction : public LootFunction {
public:
    /**
     * @brief 属性修饰符定义
     */
    struct AttributeModifier {
        String name;            // 修饰符名称
        String attributeId;     // 属性ID
        f32 value;              // 修饰值
        u8 operation;           // 操作类型
        String slot;            // 装备槽位
    };

    /**
     * @brief 构造设置属性函数
     */
    SetAttributesFunction() = default;

    /**
     * @brief 添加属性修饰符
     */
    void addModifier(const AttributeModifier& modifier);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_attributes"; }

    [[nodiscard]] const std::vector<AttributeModifier>& getModifiers() const { return m_modifiers; }

private:
    std::vector<AttributeModifier> m_modifiers;
};

/**
 * @brief 设置内容物函数
 *
 * 设置容器类物品的内容物。
 * 参考: net.minecraft.loot.functions.SetContents
 *
 * 用于生成带有物品的容器（如箱子矿车）。
 */
class SetContentsFunction : public LootFunction {
public:
    SetContentsFunction() = default;

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_contents"; }
};

/**
 * @brief 设置掉落表函数
 *
 * 为容器设置战利品表。
 * 参考: net.minecraft.loot.functions.SetLootTable
 *
 * 用于生成未开封的箱子等。
 */
class SetLootTableFunction : public LootFunction {
public:
    /**
     * @brief 构造设置掉落表函数
     * @param lootTableId 掉落表ID
     * @param seed 随机种子（0表示使用世界种子）
     */
    explicit SetLootTableFunction(const String& lootTableId, u64 seed = 0);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_loot_table"; }

    [[nodiscard]] const String& getLootTableId() const { return m_lootTableId; }
    [[nodiscard]] u64 getSeed() const { return m_seed; }

private:
    String m_lootTableId;
    u64 m_seed;
};

/**
 * @brief 探险地图函数
 *
 * 生成探险地图。
 * 参考: net.minecraft.loot.functions.ExplorationMap
 *
 * 用于生成藏宝图、林地府邸地图等。
 */
class ExplorationMapFunction : public LootFunction {
public:
    /**
     * @brief 地图目的地类型
     */
    enum class Destination : u8 {
        BuriedTreasure,     // 埋藏的宝藏
        Mansion,            // 林地府邸
        Monument,           // 海底神殿
        Shipwreck,          // 沉船
        RuinedPortal        // 废弃传送门
    };

    /**
     * @brief 构造探险地图函数
     * @param destination 目的地类型
     */
    explicit ExplorationMapFunction(Destination destination = Destination::BuriedTreasure);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "exploration_map"; }

    [[nodiscard]] Destination getDestination() const { return m_destination; }

private:
    Destination m_destination;
};

/**
 * @brief 设置炖菜效果函数
 *
 * 设置谜之炖菜的状态效果。
 * 参考: net.minecraft.loot.functions.SetStewEffect
 *
 * 用于生成带有特定效果的谜之炖菜。
 */
class SetStewEffectFunction : public LootFunction {
public:
    /**
     * @brief 效果定义
     */
    struct EffectEntry {
        String effectId;        // 效果ID
        RandomValueRange duration;  // 持续时间（秒）
    };

    SetStewEffectFunction() = default;

    /**
     * @brief 添加效果
     */
    void addEffect(const String& effectId, const RandomValueRange& duration);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] String getType() const override { return "set_stew_effect"; }

    [[nodiscard]] const std::vector<EffectEntry>& getEffects() const { return m_effects; }

private:
    std::vector<EffectEntry> m_effects;
};

/**
 * @brief 掉落函数构建器
 *
 * 提供流畅的函数构建接口。
 */
class LootFunctionBuilder {
public:
    LootFunctionBuilder() = default;

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建设置数量函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setCount(const RandomValueRange& count, bool add = false);

    /**
     * @brief 创建设置数量函数（固定值）
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setCount(i32 count, bool add = false);

    /**
     * @brief 创建时运加成函数
     * @param bonusType 加成类型
     * @param bonusMultiplier Uniform 类型的乘数（默认1）
     * @param extra Binomial 类型的额外试验次数（默认1）
     * @param probability Binomial 类型的成功概率
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> applyBonus(
        ApplyBonusFunction::BonusType bonusType = ApplyBonusFunction::BonusType::OreDrops,
        i32 bonusMultiplier = 1,
        i32 extra = 1,
        f32 probability = 1.0f);

    /**
     * @brief 创建掠夺加成函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> lootingEnchantBonus(
        const RandomValueRange& count = RandomValueRange(0.0f, 1.0f),
        i32 limit = 0);

    /**
     * @brief 创建设置损坏函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setDamage(const RandomValueRange& durability, bool add = false);

    /**
     * @brief 创建设置名称函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setName(const String& name, bool replace = true);

    /**
     * @brief 创建设置描述函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setLore(const std::vector<String>& lore, bool replace = true);

    /**
     * @brief 创建限制数量函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> limitCount(i32 min = -1, i32 max = -1);

    /**
     * @brief 创建熔炼函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> furnaceSmelt();

    /**
     * @brief 创建附魔函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> enchantWithLevels(
        const RandomValueRange& levels, bool treasure = false);

    /**
     * @brief 创建随机附魔函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> enchantRandomly(
        const std::vector<String>& enchantments = {}, bool treasure = false);

    /**
     * @brief 创建爆炸衰减函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> explosionDecay();

    /**
     * @brief 创建设置NBT函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setNbt(const String& nbtString);

    /**
     * @brief 创建复制名称函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> copyName(CopyNameFunction::Source source);

    /**
     * @brief 创建复制方块状态函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> copyBlockState(const String& blockId);

    /**
     * @brief 创建复制NBT函数
     * @param source 数据源
     * @param operations NBT路径操作列表（暂未实现完整操作解析）
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> copyNbt(CopyNbtFunction::Source source);

    /**
     * @brief 创建填充玩家头颅函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> fillPlayerHead();

    /**
     * @brief 创建设置属性函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setAttributes();

    /**
     * @brief 创建设置内容物函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setContents();

    /**
     * @brief 创建设置掉落表函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setLootTable(const String& lootTableId);

    /**
     * @brief 创建探险地图函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> explorationMap();

    /**
     * @brief 创建设置炖菜效果函数
     */
    [[nodiscard]] static std::unique_ptr<LootFunction> setStewEffect();
};

} // namespace loot
} // namespace mc
