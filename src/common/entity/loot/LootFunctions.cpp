#include "LootFunctions.hpp"
#include "LootConditions.hpp"
#include "LootContext.hpp"
#include "RandomRanges.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace loot {

// ============================================================================
// LootFunction 基类
// ============================================================================

void LootFunction::addCondition(std::unique_ptr<LootCondition> condition) {
    m_conditions.push_back(std::move(condition));
}

bool LootFunction::testConditions(LootContext& context) const {
    return std::all_of(m_conditions.begin(), m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) {
            return cond && cond->test(context);
        });
}

// ============================================================================
// SetCountFunction
// ============================================================================

SetCountFunction::SetCountFunction(const RandomValueRange& count, bool add)
    : m_count(count)
    , m_add(add)
{
}

ItemStack SetCountFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    i32 newCount = m_count.generateInt(context.getRandom());
    if (m_add) {
        stack.grow(newCount);
    } else {
        stack.setCount(newCount);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetCountFunction::clone() const {
    auto func = std::make_unique<SetCountFunction>(m_count, m_add);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// ApplyBonusFunction
// ============================================================================

ApplyBonusFunction::ApplyBonusFunction(BonusType bonusType, i32 baseCount, f32 probability)
    : m_bonusType(bonusType)
    , m_baseCount(baseCount)
    , m_probability(probability)
{
}

ItemStack ApplyBonusFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // 从上下文获取工具
    auto* tool = context.get<ItemStack>(LootParams::TOOL);
    if (!tool || tool->isEmpty()) {
        return stack;
    }

    // 获取时运等级
    i32 fortuneLevel = 0;
    auto* fortuneParam = context.get<i32>(LootParams::FORTUNE_LEVEL);
    if (fortuneParam) {
        fortuneLevel = *fortuneParam;
    } else {
        // 尝试从工具获取时运等级
        fortuneLevel = item::enchant::EnchantmentHelper::getFortuneLevel(*tool);
    }

    // 根据加成类型计算新数量
    i32 newCount = stack.getCount();
    switch (m_bonusType) {
        case BonusType::OreDrops:
            newCount = calculateOreDrops(fortuneLevel, context.getRandom());
            break;
        case BonusType::Uniform:
            newCount = calculateUniformBonus(newCount, fortuneLevel, context.getRandom());
            break;
        case BonusType::Binomial:
            newCount = calculateBinomialBonus(newCount, fortuneLevel, m_probability, context.getRandom());
            break;
    }

    stack.setCount(newCount);
    return stack;
}

std::unique_ptr<LootFunction> ApplyBonusFunction::clone() const {
    auto func = std::make_unique<ApplyBonusFunction>(m_bonusType, m_baseCount, m_probability);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

i32 ApplyBonusFunction::calculateOreDrops(i32 fortuneLevel, math::Random& random) {
    // MC 1.16.5 OreDropsFormula
    // if (fortune > 0) {
    //     int i = random.nextInt(fortune + 2) - 1;
    //     if (i < 0) i = 0;
    //     return baseCount * (i + 1);
    // } else {
    //     return baseCount;
    // }

    if (fortuneLevel <= 0) {
        return 1; // 基础数量为1
    }

    // random.nextInt(fortune + 2) - 1
    // fortune=1: random.nextInt(3) - 1 -> 0, 1, 或 -1 (修正为 0)
    // fortune=2: random.nextInt(4) - 1 -> 0, 1, 2, 或 -1 (修正为 0)
    // fortune=3: random.nextInt(5) - 1 -> 0, 1, 2, 3, 或 -1 (修正为 0)
    i32 i = random.nextInt(fortuneLevel + 2) - 1;
    if (i < 0) {
        i = 0;
    }

    return 1 * (i + 1); // 基础数量为1
}

i32 ApplyBonusFunction::calculateUniformBonus(i32 baseCount, i32 fortuneLevel, math::Random& random) {
    // MC 1.16.5 UniformBonusCountFormula
    // count + random.nextInt(bonusMultiplier * fortune + 1)
    // 默认 bonusMultiplier = 1

    if (fortuneLevel <= 0) {
        return baseCount;
    }

    i32 bonus = random.nextInt(fortuneLevel + 1);
    return baseCount + bonus;
}

i32 ApplyBonusFunction::calculateBinomialBonus(i32 baseCount, i32 fortuneLevel, f32 probability, math::Random& random) {
    // MC 1.16.5 BinomialWithBonusCountFormula
    // for (int i = 0; i < fortune + extra; ++i) {
    //     if (random.nextFloat() < probability) {
    //         ++count;
    //     }
    // }
    // extra 默认为 1

    i32 extra = 1; // 默认额外次数
    i32 trials = fortuneLevel + extra;

    for (i32 i = 0; i < trials; ++i) {
        if (random.nextFloat() < probability) {
            ++baseCount;
        }
    }

    return baseCount;
}

// ============================================================================
// LootingEnchantBonusFunction
// ============================================================================

LootingEnchantBonusFunction::LootingEnchantBonusFunction(const RandomValueRange& count, i32 limit)
    : m_count(count)
    , m_limit(limit)
{
}

ItemStack LootingEnchantBonusFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // 获取掠夺附魔等级
    i32 lootingLevel = context.getLootingModifier();
    if (lootingLevel <= 0) {
        return stack;
    }

    // MC 1.16.5: float f = (float)lootingLevel * count.generateFloat(random);
    // stack.grow(Math.round(f));
    f32 bonus = static_cast<f32>(lootingLevel) * m_count.generateFloat(context.getRandom());
    stack.grow(math::roundTo<i32>(bonus));

    // 检查限制
    if (m_limit > 0 && stack.getCount() > m_limit) {
        stack.setCount(m_limit);
    }

    return stack;
}

std::unique_ptr<LootFunction> LootingEnchantBonusFunction::clone() const {
    auto func = std::make_unique<LootingEnchantBonusFunction>(m_count, m_limit);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetDamageFunction
// ============================================================================

SetDamageFunction::SetDamageFunction(const RandomValueRange& durability, bool add)
    : m_durability(durability)
    , m_add(add)
{
}

ItemStack SetDamageFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty() || !stack.isDamageable()) {
        return stack;
    }

    i32 maxDamage = stack.getMaxDamage();
    if (maxDamage <= 0) {
        return stack;
    }

    // 生成损坏程度 (0.0 = 完好, 1.0 = 完全损坏)
    f32 durabilityRatio = m_durability.generateFloat(context.getRandom());
    durabilityRatio = math::clamp(durabilityRatio, 0.0f, 1.0f);

    // 计算实际耐久度
    i32 damage = static_cast<i32>((1.0f - durabilityRatio) * static_cast<f32>(maxDamage));

    if (m_add) {
        stack.setDamage(stack.getDamage() + damage);
    } else {
        stack.setDamage(damage);
    }

    // 确保不超过最大耐久度
    if (stack.getDamage() >= maxDamage) {
        stack.setDamage(maxDamage - 1);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetDamageFunction::clone() const {
    auto func = std::make_unique<SetDamageFunction>(m_durability, m_add);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetNameFunction
// ============================================================================

SetNameFunction::SetNameFunction(const String& name, bool replace)
    : m_name(name)
    , m_replace(replace)
{
}

ItemStack SetNameFunction::apply(ItemStack stack, LootContext& context) const {
    MC_UNUSED(context);

    if (stack.isEmpty()) {
        return stack;
    }

    // 如果不替换且已有自定义名称，则不设置
    if (!m_replace && stack.hasCustomName()) {
        return stack;
    }

    stack.setCustomName(m_name);
    return stack;
}

std::unique_ptr<LootFunction> SetNameFunction::clone() const {
    auto func = std::make_unique<SetNameFunction>(m_name, m_replace);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetLoreFunction
// ============================================================================

SetLoreFunction::SetLoreFunction(const std::vector<String>& lore, bool replace)
    : m_lore(lore)
    , m_replace(replace)
{
}

ItemStack SetLoreFunction::apply(ItemStack stack, LootContext& context) const {
    MC_UNUSED(context);

    if (stack.isEmpty()) {
        return stack;
    }

    if (m_replace) {
        stack.clearLore();
    }

    for (const auto& line : m_lore) {
        stack.addLoreLine(line);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetLoreFunction::clone() const {
    auto func = std::make_unique<SetLoreFunction>(m_lore, m_replace);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// LimitCountFunction
// ============================================================================

LimitCountFunction::LimitCountFunction(i32 min, i32 max)
    : m_min(min)
    , m_max(max)
{
}

ItemStack LimitCountFunction::apply(ItemStack stack, LootContext& context) const {
    MC_UNUSED(context);

    if (stack.isEmpty()) {
        return stack;
    }

    i32 count = stack.getCount();

    if (m_min >= 0 && count < m_min) {
        stack.setCount(m_min);
    }

    if (m_max >= 0 && count > m_max) {
        stack.setCount(m_max);
    }

    return stack;
}

std::unique_ptr<LootFunction> LimitCountFunction::clone() const {
    auto func = std::make_unique<LimitCountFunction>(m_min, m_max);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// FurnaceSmeltFunction
// ============================================================================

ItemStack FurnaceSmeltFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现熔炼逻辑
    // 需要查找熔炼配方，返回熔炼后的物品
    // 参考: net.minecraft.loot.functions.FurnaceSmelt

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> FurnaceSmeltFunction::clone() const {
    auto func = std::make_unique<FurnaceSmeltFunction>();
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// EnchantWithLevelsFunction
// ============================================================================

EnchantWithLevelsFunction::EnchantWithLevelsFunction(const RandomValueRange& levels, bool treasure)
    : m_levels(levels)
    , m_treasure(treasure)
{
}

ItemStack EnchantWithLevelsFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现附魔逻辑
    // 需要附魔系统支持
    // 参考: net.minecraft.loot.functions.EnchantWithLevels

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> EnchantWithLevelsFunction::clone() const {
    auto func = std::make_unique<EnchantWithLevelsFunction>(m_levels, m_treasure);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// EnchantRandomlyFunction
// ============================================================================

EnchantRandomlyFunction::EnchantRandomlyFunction(const std::vector<String>& enchantments, bool treasure)
    : m_enchantments(enchantments)
    , m_treasure(treasure)
{
}

ItemStack EnchantRandomlyFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现随机附魔逻辑
    // 需要附魔系统支持
    // 参考: net.minecraft.loot.functions.EnchantRandomly

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> EnchantRandomlyFunction::clone() const {
    auto func = std::make_unique<EnchantRandomlyFunction>(m_enchantments, m_treasure);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// ExplosionDecayFunction
// ============================================================================

ItemStack ExplosionDecayFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // MC 1.16.5: 每个物品有 1/explosionRadius 的概率保留
    // 如果没有爆炸信息，默认使用半径 1.0（100% 保留）
    f32 explosionRadius = 1.0f;
    auto* radiusParam = context.get<f32>(LootParams::EXPLOSION_RADIUS);
    if (radiusParam != nullptr && *radiusParam > 0.0f) {
        explosionRadius = *radiusParam;
    }

    // 使用二项分布计算保留数量
    f32 keepChance = math::clamp(1.0f / explosionRadius, 0.0f, 1.0f);
    BinomialRange binomial(stack.getCount(), keepChance);
    i32 keptCount = binomial.generateInt(context.getRandom());

    if (keptCount <= 0) {
        return ItemStack();
    }

    stack.setCount(keptCount);
    return stack;
}

std::unique_ptr<LootFunction> ExplosionDecayFunction::clone() const {
    auto func = std::make_unique<ExplosionDecayFunction>();
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetNbtFunction
// ============================================================================

SetNbtFunction::SetNbtFunction(const String& nbtString)
    : m_nbtString(nbtString)
{
}

ItemStack SetNbtFunction::apply(ItemStack stack, LootContext& context) const {
    MC_UNUSED(context);

    if (stack.isEmpty() || m_nbtString.empty()) {
        return stack;
    }

    // TODO: 实现 NBT 解析和应用
    // 需要集成 NBT 系统
    // 参考: net.minecraft.loot.functions.SetNbt
    // stack.setTag(nbt);

    return stack;
}

std::unique_ptr<LootFunction> SetNbtFunction::clone() const {
    auto func = std::make_unique<SetNbtFunction>(m_nbtString);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// CopyNameFunction
// ============================================================================

CopyNameFunction::CopyNameFunction(Source source)
    : m_source(source)
{
}

ItemStack CopyNameFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现名称复制
    // 参考: net.minecraft.loot.functions.CopyName
    // 需要从以下来源获取名称:
    // - Source::This: 从 THIS_ENTITY 参数获取实体名称
    // - Source::Killer: 从 KILLER_ENTITY 参数获取击杀者名称
    // - Source::KillerPlayer: 从 KILLER_PLAYER 参数获取玩家名称
    // - Source::BlockEntity: 从 BLOCK_STATE 参数获取方块实体名称
    // 需要 Entity/Player 类实现 INameable 接口

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> CopyNameFunction::clone() const {
    auto func = std::make_unique<CopyNameFunction>(m_source);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// CopyBlockStateFunction
// ============================================================================

CopyBlockStateFunction::CopyBlockStateFunction(const String& blockId,
                                               const std::vector<String>& properties)
    : m_blockId(blockId)
    , m_properties(properties)
{
}

ItemStack CopyBlockStateFunction::apply(ItemStack stack, LootContext& context) const {
    MC_UNUSED(context);

    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现方块状态复制
    // 参考: net.minecraft.loot.functions.CopyBlockState
    // 需要将 BlockState 的属性值复制到 ItemStack 的 NBT 中

    return stack;
}

std::unique_ptr<LootFunction> CopyBlockStateFunction::clone() const {
    auto func = std::make_unique<CopyBlockStateFunction>(m_blockId, m_properties);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// CopyNbtFunction
// ============================================================================

CopyNbtFunction::CopyNbtFunction(Source source)
    : m_source(source)
{
}

void CopyNbtFunction::addOperation(const String& sourcePath, const String& targetPath, Operation operation) {
    m_operations.push_back({sourcePath, targetPath, operation});
}

ItemStack CopyNbtFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty() || m_operations.empty()) {
        return stack;
    }

    // TODO: 实现 NBT 复制
    // 参考: net.minecraft.loot.functions.CopyNbt
    // 需要 NBT 路径解析和操作实现

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> CopyNbtFunction::clone() const {
    auto func = std::make_unique<CopyNbtFunction>(m_source);
    func->m_operations = m_operations;
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// FillPlayerHeadFunction
// ============================================================================

FillPlayerHeadFunction::FillPlayerHeadFunction(CopyNameFunction::Source source)
    : m_source(source)
{
}

ItemStack FillPlayerHeadFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现玩家头颅填充
    // 参考: net.minecraft.loot.functions.FillPlayerHead
    // 需要获取玩家信息并设置到头颅物品的 NBT 中

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> FillPlayerHeadFunction::clone() const {
    auto func = std::make_unique<FillPlayerHeadFunction>(m_source);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetAttributesFunction
// ============================================================================

void SetAttributesFunction::addModifier(const AttributeModifier& modifier) {
    m_modifiers.push_back(modifier);
}

ItemStack SetAttributesFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty() || m_modifiers.empty()) {
        return stack;
    }

    // TODO: 实现属性设置
    // 参考: net.minecraft.loot.functions.SetAttributes
    // 需要属性系统支持

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> SetAttributesFunction::clone() const {
    auto func = std::make_unique<SetAttributesFunction>();
    func->m_modifiers = m_modifiers;
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetContentsFunction
// ============================================================================

ItemStack SetContentsFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现内容物设置
    // 参考: net.minecraft.loot.functions.SetContents
    // 需要容器物品系统支持

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> SetContentsFunction::clone() const {
    auto func = std::make_unique<SetContentsFunction>();
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetLootTableFunction
// ============================================================================

SetLootTableFunction::SetLootTableFunction(const String& lootTableId, u64 seed)
    : m_lootTableId(lootTableId)
    , m_seed(seed)
{
}

ItemStack SetLootTableFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty() || m_lootTableId.empty()) {
        return stack;
    }

    // TODO: 实现掉落表设置
    // 参考: net.minecraft.loot.functions.SetLootTable
    // 需要将掉落表ID设置到物品的NBT中

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> SetLootTableFunction::clone() const {
    auto func = std::make_unique<SetLootTableFunction>(m_lootTableId, m_seed);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// ExplorationMapFunction
// ============================================================================

ExplorationMapFunction::ExplorationMapFunction(Destination destination)
    : m_destination(destination)
{
}

ItemStack ExplorationMapFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty()) {
        return stack;
    }

    // TODO: 实现探险地图生成
    // 参考: net.minecraft.loot.functions.ExplorationMap
    // 需要地图系统和世界探索追踪支持

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> ExplorationMapFunction::clone() const {
    auto func = std::make_unique<ExplorationMapFunction>(m_destination);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// SetStewEffectFunction
// ============================================================================

void SetStewEffectFunction::addEffect(const String& effectId, const RandomValueRange& duration) {
    m_effects.push_back({effectId, duration});
}

ItemStack SetStewEffectFunction::apply(ItemStack stack, LootContext& context) const {
    if (stack.isEmpty() || m_effects.empty()) {
        return stack;
    }

    // TODO: 实现炖菜效果设置
    // 参考: net.minecraft.loot.functions.SetStewEffect
    // 需要药水效果系统支持

    MC_UNUSED(context);
    return stack;
}

std::unique_ptr<LootFunction> SetStewEffectFunction::clone() const {
    auto func = std::make_unique<SetStewEffectFunction>();
    func->m_effects = m_effects;
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

// ============================================================================
// LootFunctionBuilder
// ============================================================================

std::unique_ptr<LootFunction> LootFunctionBuilder::setCount(const RandomValueRange& count, bool add) {
    return std::make_unique<SetCountFunction>(count, add);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setCount(i32 count, bool add) {
    return std::make_unique<SetCountFunction>(RandomValueRange(static_cast<f32>(count)), add);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::applyBonus(
    ApplyBonusFunction::BonusType bonusType,
    i32 baseCount,
    f32 probability)
{
    return std::make_unique<ApplyBonusFunction>(bonusType, baseCount, probability);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::lootingEnchantBonus(
    const RandomValueRange& count,
    i32 limit)
{
    return std::make_unique<LootingEnchantBonusFunction>(count, limit);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setDamage(const RandomValueRange& durability, bool add) {
    return std::make_unique<SetDamageFunction>(durability, add);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setName(const String& name, bool replace) {
    return std::make_unique<SetNameFunction>(name, replace);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setLore(const std::vector<String>& lore, bool replace) {
    return std::make_unique<SetLoreFunction>(lore, replace);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::limitCount(i32 min, i32 max) {
    return std::make_unique<LimitCountFunction>(min, max);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::furnaceSmelt() {
    return std::make_unique<FurnaceSmeltFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::enchantWithLevels(
    const RandomValueRange& levels,
    bool treasure)
{
    return std::make_unique<EnchantWithLevelsFunction>(levels, treasure);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::enchantRandomly(
    const std::vector<String>& enchantments,
    bool treasure)
{
    return std::make_unique<EnchantRandomlyFunction>(enchantments, treasure);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::explosionDecay() {
    return std::make_unique<ExplosionDecayFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setNbt(const String& nbtString) {
    return std::make_unique<SetNbtFunction>(nbtString);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::copyName(CopyNameFunction::Source source) {
    return std::make_unique<CopyNameFunction>(source);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::copyBlockState(const String& blockId) {
    return std::make_unique<CopyBlockStateFunction>(blockId);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::copyNbt(CopyNbtFunction::Source source) {
    return std::make_unique<CopyNbtFunction>(source);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::fillPlayerHead() {
    return std::make_unique<FillPlayerHeadFunction>(CopyNameFunction::Source::KillerPlayer);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setAttributes() {
    return std::make_unique<SetAttributesFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setContents() {
    return std::make_unique<SetContentsFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setLootTable(const String& lootTableId) {
    return std::make_unique<SetLootTableFunction>(lootTableId);
}

std::unique_ptr<LootFunction> LootFunctionBuilder::explorationMap() {
    return std::make_unique<ExplorationMapFunction>();
}

std::unique_ptr<LootFunction> LootFunctionBuilder::setStewEffect() {
    return std::make_unique<SetStewEffectFunction>();
}

} // namespace loot
} // namespace mc
