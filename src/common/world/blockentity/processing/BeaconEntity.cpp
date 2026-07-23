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

#include "world/blockentity/processing/BeaconEntity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 信标每次重新计算结构与应用效果的 tick 间隔。
 */
constexpr i32 BEACON_UPDATE_INTERVAL = 80;

/**
 * @brief 支付物品 JSON 字段名。
 */
constexpr const char* PAYMENT_ITEM_KEY = "payment_item";

/**
 * @brief 将效果类型编码为稳定整数 ID（0 表示空效果）。
 */
[[nodiscard]] i32 encodeEffect(BeaconEntity::EffectType effect)
{
    return static_cast<i32>(effect) + 1;
}

/**
 * @brief 将整数 ID 解码为效果类型。
 */
[[nodiscard]] bool decodeEffect(i32 rawValue, BeaconEntity::EffectType& outEffect)
{
    if (rawValue <= 0) {
        return false;
    }

    const i32 enumValue = rawValue - 1;
    if (enumValue <= 0 || enumValue >= static_cast<i32>(entity::effect::EffectType::Count)) {
        return false;
    }

    outEffect = static_cast<BeaconEntity::EffectType>(enumValue);
    return true;
}

/**
 * @brief 检查信标基座方块是否属于可用矿物方块。
 */
[[nodiscard]] bool isValidBeaconBaseBlock(const BlockState* state)
{
    if (state == nullptr || state->isAir()) {
        return false;
    }

    const Block* block = &state->getBlock();
    return block == VanillaBlocks::IRON_BLOCK || block == VanillaBlocks::GOLD_BLOCK ||
        block == VanillaBlocks::DIAMOND_BLOCK || block == VanillaBlocks::EMERALD_BLOCK ||
        block == VanillaBlocks::NETHERITE_BLOCK;
}

/**
 * @brief 将效果以环境效果形式施加给玩家。
 */
void applyBeaconEffectToPlayer(Player& player, BeaconEntity::EffectType effectType, i32 durationTicks, i32 amplifier)
{
    const entity::effect::EffectInstance effect(effectType, durationTicks, amplifier, true, true, true);
    player.addEffect(effect);
}

} // namespace

// ========== 静态数据初始化 ==========

namespace {

// 信标效果存储 - 用于初始化VALID_EFFECTS
// 使用静态变量确保地址稳定
static const BeaconEntity::EffectType s_level1Effects[] = {
    BeaconEntity::EffectType::Speed, BeaconEntity::EffectType::Haste};

static const BeaconEntity::EffectType s_level2Effects[] = {
    BeaconEntity::EffectType::Resistance, BeaconEntity::EffectType::JumpBoost};

static const BeaconEntity::EffectType s_level3Effects[] = {BeaconEntity::EffectType::Strength};

static const BeaconEntity::EffectType s_level4Effects[] = {BeaconEntity::EffectType::Regeneration};

} // namespace

// 信标等级对应的有效效果
// Level 1: Speed, Haste
// Level 2: Resistance, JumpBoost
// Level 3: Strength
// Level 4: Regeneration (仅作为辅助效果)
const std::array<std::vector<const BeaconEntity::EffectType*>, 4> BeaconEntity::VALID_EFFECTS = {
    // Level 1: 速度、急迫
    std::vector<const EffectType*>{&s_level1Effects[0], &s_level1Effects[1]},
    // Level 2: 抗性提升、跳跃提升
    std::vector<const EffectType*>{&s_level2Effects[0], &s_level2Effects[1]},
    // Level 3: 力量
    std::vector<const EffectType*>{&s_level3Effects[0]},
    // Level 4: 生命恢复（仅辅助效果）
    std::vector<const EffectType*>{&s_level4Effects[0]}};

// ========== BeaconEntity 实现 ==========

BeaconEntity::BeaconEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Beacon, pos)
{}

BeaconEntity::~BeaconEntity() = default;

void BeaconEntity::setLevel(i32 level)
{
    level = std::max(0, std::min(level, MAX_LEVELS));
    if (m_level != level) {
        m_level = level;
        setChanged();
    }
}

void BeaconEntity::setPrimaryEffect(const EffectType* effect)
{
    std::optional<EffectType> nextEffect;
    if (effect != nullptr) {
        nextEffect = *effect;
    }
    if (m_primaryEffect != nextEffect) {
        m_primaryEffect = nextEffect;
        setChanged();
    }
}

void BeaconEntity::setSecondaryEffect(const EffectType* effect)
{
    std::optional<EffectType> nextEffect;
    if (effect != nullptr) {
        nextEffect = *effect;
    }
    if (m_secondaryEffect != nextEffect) {
        m_secondaryEffect = nextEffect;
        setChanged();
    }
}

void BeaconEntity::setPaymentItem(const ItemStack& stack)
{
    if (!stack.isEmpty()) {
        MC_ASSERT_RELEASE(_isValidPayment(stack.getItem()->itemId()));
    }
    m_paymentItem = stack;
    setChanged();
}

bool BeaconEntity::canUseEffect(const EffectType* effect) const
{
    if (effect == nullptr) {
        return true;
    }

    const i32 effectValue = static_cast<i32>(*effect);
    return effectValue > 0 && effectValue < static_cast<i32>(entity::effect::EffectType::Count);
}

i32 BeaconEntity::getEffectRange() const
{
    return m_level * 10 + 10;
}

void BeaconEntity::tick(IWorld& world)
{
    m_tickCount++;

    const i32 oldLevel = m_level;

    if (m_tickCount % BEACON_UPDATE_INTERVAL == 0) {
        _updateLevels(world);
    }

    // 激活/取消激活音效
    if (m_level != oldLevel) {
        const Vector3 center = m_pos.center();
        if (m_level > 0 && oldLevel == 0) {
            // 激活
            world.playSound(SoundEvents::BLOCK_BEACON_ACTIVATE, sound::SoundCategory::Blocks, center, 1.0f, 1.0f);
        } else if (m_level == 0 && oldLevel > 0) {
            // 取消激活
            world.playSound(SoundEvents::BLOCK_BEACON_DEACTIVATE, sound::SoundCategory::Blocks, center, 1.0f, 1.0f);
        }
    }

    if (isActive() && m_tickCount % BEACON_UPDATE_INTERVAL == 0) {
        _applyEffects(world);
    }

    // 激活状态下播放环境音效
    if (isActive() && !world.isClientSide() && m_tickCount % BEACON_UPDATE_INTERVAL == 0) {
        world.playSound(SoundEvents::BLOCK_BEACON_AMBIENT, sound::SoundCategory::Blocks, m_pos.center(), 1.0f, 1.0f);
    }
}

void BeaconEntity::_updateLevels(IWorld& world)
{
    // 金字塔检测不需要检查天空可见性
    i32 newLevel = 0;

    for (i32 level = 1; level <= MAX_LEVELS; ++level) {
        const i32 y = m_pos.y - level;
        bool levelValid = true;

        for (i32 dx = -level; dx <= level && levelValid; ++dx) {
            for (i32 dz = -level; dz <= level; ++dz) {
                const BlockPos basePos(m_pos.x + dx, y, m_pos.z + dz);
                if (!isValidBeaconBaseBlock(world.getBlockState(basePos))) {
                    levelValid = false;
                    break;
                }
            }
        }

        if (!levelValid) {
            break;
        }
        newLevel = level;
    }

    setLevel(newLevel);
}

bool BeaconEntity::_canSeeSky(IWorld& world) const
{
    const i32 maxY = world.getHeight(m_pos.x, m_pos.z);
    for (i32 y = m_pos.y + 1; y <= maxY; ++y) {
        const BlockState* state = world.getBlockState(m_pos.x, y, m_pos.z);
        if (state != nullptr && !state->isAir()) {
            return false;
        }
    }
    return true;
}

void BeaconEntity::_applyEffects(IWorld& world)
{
    if (m_level <= 0 || !m_primaryEffect.has_value()) {
        return;
    }

    i32 amplifier = 0;
    if (m_level >= 4 && m_secondaryEffect.has_value() && m_secondaryEffect.value() == m_primaryEffect.value()) {
        amplifier = 1;
    }

    const i32 durationTicks = (9 + m_level * 2) * 20;
    const i32 range = getEffectRange();
    const Vector3 center = m_pos.center();

    const std::vector<Entity*> entities = world.getEntitiesInRange(center, static_cast<f32>(range), nullptr);
    for (Entity* entity : entities) {
        auto* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        applyBeaconEffectToPlayer(*player, m_primaryEffect.value(), durationTicks, amplifier);

        if (m_level >= 4 && m_secondaryEffect.has_value() && m_secondaryEffect.value() != m_primaryEffect.value()) {
            applyBeaconEffectToPlayer(*player, m_secondaryEffect.value(), durationTicks, 0);
        }
    }
}

bool BeaconEntity::_isValidPayment(u32 itemId)
{
    const Item* item = ItemRegistry::instance().getItem(itemId);
    if (item == nullptr) {
        return false;
    }

    return item == Items::IRON_INGOT || item == Items::GOLD_INGOT || item == Items::DIAMOND || item == Items::EMERALD ||
        item == Items::NETHERITE_INGOT;
}

bool BeaconEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    setLevel(data.value("level", 0));

    m_primaryEffect = std::nullopt;
    m_secondaryEffect = std::nullopt;

    if (data.contains("primary_effect") && data["primary_effect"].is_number_integer()) {
        EffectType primary = EffectType::Speed;
        if (decodeEffect(data["primary_effect"].get<i32>(), primary)) {
            m_primaryEffect = primary;
        }
    }

    if (data.contains("secondary_effect") && data["secondary_effect"].is_number_integer()) {
        EffectType secondary = EffectType::Speed;
        if (decodeEffect(data["secondary_effect"].get<i32>(), secondary)) {
            m_secondaryEffect = secondary;
        }
    }

    m_paymentItem = ItemStack::EMPTY;
    if (data.contains(PAYMENT_ITEM_KEY)) {
        const auto paymentResult = ItemStack::fromJson(data[PAYMENT_ITEM_KEY]);
        if (paymentResult.success()) {
            const ItemStack loadedPayment = paymentResult.value();
            if (loadedPayment.isEmpty() || _isValidPayment(loadedPayment.getItem()->itemId())) {
                m_paymentItem = loadedPayment;
            }
        }
    }

    return true;
}

void BeaconEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["level"] = m_level;

    if (m_primaryEffect.has_value()) {
        data["primary_effect"] = encodeEffect(m_primaryEffect.value());
    } else {
        data["primary_effect"] = 0;
    }

    if (m_secondaryEffect.has_value()) {
        data["secondary_effect"] = encodeEffect(m_secondaryEffect.value());
    } else {
        data["secondary_effect"] = 0;
    }

    if (!m_paymentItem.isEmpty()) {
        data[PAYMENT_ITEM_KEY] = m_paymentItem.toJson();
    } else {
        data.erase(PAYMENT_ITEM_KEY);
    }
}

std::unique_ptr<BlockEntity> BeaconEntity::clone() const
{
    auto cloned = std::make_unique<BeaconEntity>(m_pos);
    cloned->m_level = m_level;
    cloned->m_tickCount = m_tickCount;
    cloned->m_primaryEffect = m_primaryEffect;
    cloned->m_secondaryEffect = m_secondaryEffect;
    cloned->m_paymentItem = m_paymentItem.copy();
    cloned->m_lastBeamState = m_lastBeamState;
    cloned->m_beamSegments = m_beamSegments;
    return cloned;
}

std::array<f32, 3> BeaconEntity::_blendColors(const std::array<f32, 3>& current, const std::array<f32, 3>& newColor)
{
    // 颜色平均混合
    return {(current[0] + newColor[0]) / 2.0f, (current[1] + newColor[1]) / 2.0f, (current[2] + newColor[2]) / 2.0f};
}

void BeaconEntity::_updateBeamSegments(IWorld& world)
{
    m_beamSegments.clear();

    if (m_level <= 0) {
        return;
    }

    // 遍历光束经过的方块
    BeaconBeamSegment* currentSegment = nullptr;

    // 从信标上方开始遍历，直到世界顶部
    const i32 worldHeight = world.getHeight(m_pos.x, m_pos.z);

    for (i32 y = m_pos.y + 1; y <= worldHeight; ++y) {
        const BlockPos checkPos(m_pos.x, y, m_pos.z);
        const BlockState* state = world.getBlockState(checkPos);

        if (state == nullptr || state->isAir()) {
            // 空气，增加当前段高度
            if (currentSegment != nullptr) {
                currentSegment->incrementHeight();
            }
            continue;
        }

        const Block* block = &state->getBlock();

        // 检查方块是否阻挡光束（不透明方块且不是基岩）
        const i32 opacity = state->getOpacity();
        const bool blocksBeam = (opacity >= 15 && block != VanillaBlocks::BEDROCK);

        if (blocksBeam) {
            // 光束被阻挡，清除所有段
            m_beamSegments.clear();
            return;
        }

        // 从方块获取信标光束颜色
        const std::array<f32, 3>* blockColorPtr = block->getBeaconColorMultiplier(*state, &world, &checkPos, &m_pos);

        if (blockColorPtr == nullptr) {
            // 方块不提供颜色，继续增加当前段高度
            if (currentSegment != nullptr) {
                currentSegment->incrementHeight();
            } else {
                // 第一个段，创建白色段
                m_beamSegments.emplace_back(1.0f, 1.0f, 1.0f);
                currentSegment = &m_beamSegments.back();
            }
        } else {
            // 方块提供了颜色
            std::array<f32, 3> blockColor = *blockColorPtr;

            if (m_beamSegments.empty()) {
                // 第一个段，使用方块颜色
                m_beamSegments.emplace_back(blockColor);
                currentSegment = &m_beamSegments.back();
            } else if (currentSegment != nullptr) {
                // 检查颜色是否相同（使用容差比较）
                const f32 epsilon = 0.001f;
                bool sameColor = (std::abs(currentSegment->colors[0] - blockColor[0]) < epsilon &&
                    std::abs(currentSegment->colors[1] - blockColor[1]) < epsilon &&
                    std::abs(currentSegment->colors[2] - blockColor[2]) < epsilon);

                if (sameColor) {
                    currentSegment->incrementHeight();
                } else {
                    // 颜色不同，创建新段（颜色混合）
                    std::array<f32, 3> blendedColor = _blendColors(currentSegment->colors, blockColor);
                    m_beamSegments.emplace_back(blendedColor);
                    currentSegment = &m_beamSegments.back();
                }
            }
        }
    }

    // 如果没有段，创建一个默认的白色段
    if (m_beamSegments.empty()) {
        m_beamSegments.emplace_back(1.0f, 1.0f, 1.0f);
    }
}

} // namespace blockentity
} // namespace mc
