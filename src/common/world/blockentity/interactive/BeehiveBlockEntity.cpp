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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "BeehiveBlockEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/blocks/mob/BeehiveBlock.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ============================================================================
// 构造函数
// ============================================================================

BeehiveBlockEntity::BeehiveBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Beehive, pos)
    , m_savedFlowerPos(BlockPos::zero())
{}

// ============================================================================
// BlockEntity 接口
// ============================================================================

void BeehiveBlockEntity::tick(IWorld& world)
{
    _tickOccupants(world);
}

bool BeehiveBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    m_bees.clear();

    if (data.contains("bees") && data["bees"].is_array()) {
        for (const auto& beeData : data["bees"]) {
            BeeOccupant occupant;
            occupant.hasNectar = beeData.value("has_nectar", false);
            occupant.ticksInHive = beeData.value("ticks_in_hive", 0);
            occupant.minTicksInHive = beeData.value("min_ticks_in_hive", MIN_OCCUPATION_TICKS_NECTARLESS);
            m_bees.push_back(occupant);
        }
    }

    if (data.contains("flower_pos")) {
        const auto& fp = data["flower_pos"];
        if (fp.is_array() && fp.size() == 3) {
            m_savedFlowerPos = BlockPos(fp[0].get<i32>(), fp[1].get<i32>(), fp[2].get<i32>());
        }
    }

    return true;
}

void BeehiveBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    auto beesArray = nlohmann::json::array();
    for (const auto& occupant : m_bees) {
        nlohmann::json beeObj;
        beeObj["has_nectar"] = occupant.hasNectar;
        beeObj["ticks_in_hive"] = occupant.ticksInHive;
        beeObj["min_ticks_in_hive"] = occupant.minTicksInHive;
        beesArray.push_back(beeObj);
    }
    data["bees"] = beesArray;

    data["flower_pos"] = {m_savedFlowerPos.x, m_savedFlowerPos.y, m_savedFlowerPos.z};
}

std::unique_ptr<BlockEntity> BeehiveBlockEntity::clone() const
{
    auto copy = std::make_unique<BeehiveBlockEntity>(m_pos);
    copy->m_bees = m_bees;
    copy->m_savedFlowerPos = m_savedFlowerPos;
    return copy;
}

// ============================================================================
// 蜜蜂管理
// ============================================================================

bool BeehiveBlockEntity::addOccupant(BeeEntity& bee)
{
    if (isFull()) {
        return false;
    }

    BeeOccupant occupant;
    occupant.hasNectar = bee.hasNectar();
    occupant.ticksInHive = 0;
    occupant.minTicksInHive = occupant.hasNectar ? MIN_OCCUPATION_TICKS_NECTAR : MIN_OCCUPATION_TICKS_NECTARLESS;

    // 继承蜜蜂的花朵位置（50%概率）
    if (bee.hasFlower()) {
        if (m_rng.nextBoolean()) {
            m_savedFlowerPos = bee.getFlowerPos();
        }
    }

    m_bees.push_back(occupant);

    // 播放蜜蜂进入蜂巢音效
    if (m_world) {
        m_world->playSound(SoundEvents::BLOCK_BEEHIVE_ENTER,
            sound::SoundCategory::Blocks,
            Vector3(static_cast<f32>(m_pos.x + 0.5), static_cast<f32>(m_pos.y + 0.5), static_cast<f32>(m_pos.z + 0.5)),
            1.0f,
            1.0f);
    }

    // 从世界移除蜜蜂实体
    bee.remove();

    setChanged();
    return true;
}

void BeehiveBlockEntity::emptyAllLivingFromHive(
    IWorld& world, Player* player, const BlockState& state, BeeReleaseStatus releaseStatus)
{
    // 前向遍历：释放成功时元素被移除、后续元素前移，i 不递增以处理前移的元素；
    // 释放失败时（天气/夜间阻止），++i 跳过该蜜蜂
    i32 i = 0;
    while (i < static_cast<i32>(m_bees.size())) {
        if (_releaseOccupant(world, state, i, releaseStatus)) {
            // 释放成功，元素已从列表中移除，后续元素前移到索引 i，不需要递增
        } else {
            // 释放失败（天气/夜间阻止或出口被阻挡），跳过该蜜蜂
            ++i;
        }
    }

    // 如果玩家在 4 格内且蜂巢未被营火安抚，激怒蜜蜂
    if (player && !isSedated(world, m_pos)) {
        angerNearbyBees(world, m_pos, *player);
    }
}

// ============================================================================
// 环境检测
// ============================================================================

bool BeehiveBlockEntity::isFireNearby(IWorld& world, const BlockPos& pos)
{
    // 检查 3x3x3 范围内是否有火
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                const BlockState* blockState = world.getBlockState(checkPos);
                if (blockState && blockState->getBlock().material() == Material::FIRE) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool BeehiveBlockEntity::isSedated(IWorld& world, const BlockPos& pos)
{
    // 检查蜂巢下方是否有营火烟雾
    return blocks::CampfireBlock::isSmokeyPos(world, pos);
}

bool BeehiveBlockEntity::hiveContainsBees(IWorld& world, const BlockPos& pos)
{
    auto* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Beehive) {
        return false;
    }
    auto* beehive = static_cast<BeehiveBlockEntity*>(blockEntity);
    return !beehive->isEmpty();
}

// ============================================================================
// 私有方法
// ============================================================================

bool BeehiveBlockEntity::_releaseOccupant(
    IWorld& world, const BlockState& state, i32 occupantIndex, BeeReleaseStatus releaseStatus)
{
    if (occupantIndex < 0 || occupantIndex >= static_cast<i32>(m_bees.size())) {
        return false;
    }

    BeeOccupant occupant = m_bees[occupantIndex];

    // 天气/夜间检查：非紧急释放时，雨天/雷暴/夜间蜜蜂留在巢内
    if (releaseStatus != BeeReleaseStatus::Emergency) {
        if (world.isRaining() || world.isThundering() || !world.isDaytime()) {
            return false;
        }
    }

    m_bees.erase(m_bees.begin() + occupantIndex);

    // 计算释放位置
    auto releasePos = _getReleasePosition(world, m_pos, state);
    if (!releasePos.has_value()) {
        // 出口被阻挡且非紧急释放，不释放蜜蜂
        if (releaseStatus != BeeReleaseStatus::Emergency) {
            // 放回蜜蜂数据
            m_bees.insert(m_bees.begin() + occupantIndex, occupant);
            return false;
        }
        // 紧急释放时，直接使用蜂巢上方位置
        releasePos = m_pos.up();
    }

    // 创建蜜蜂实体
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        m_bees.insert(m_bees.begin() + occupantIndex, occupant);
        return false;
    }
    auto beeEntity = BeeEntity::create(&world, *registry);
    if (!beeEntity) {
        // 创建失败，放回蜜蜂数据
        m_bees.insert(m_bees.begin() + occupantIndex, occupant);
        return false;
    }

    auto* bee = dynamic_cast<BeeEntity*>(beeEntity.get());
    if (!bee) {
        // 创建的实体类型不是 BeeEntity，不应发生，放回蜜蜂数据
        m_bees.insert(m_bees.begin() + occupantIndex, occupant);
        return false;
    }

    // 设置蜜蜂位置
    bee->setPosition(Vector3(
        static_cast<f32>(releasePos->x + 0.5), static_cast<f32>(releasePos->y), static_cast<f32>(releasePos->z + 0.5)));

    // 设置蜂巢位置
    bee->setHivePos(m_pos);
    bee->setHasHive(true);

    // 设置花朵位置（90%概率传递保存的花朵位置）
    if (m_savedFlowerPos != BlockPos::zero() && m_rng.nextFloat() < 0.9f) {
        bee->setFlowerPos(m_savedFlowerPos);
    }

    // 交付花蜜
    if (releaseStatus == BeeReleaseStatus::HoneyDelivered) {
        bee->setHasNectar(false);
        bee->resetCropCounter();
        bee->resetTicksWithoutNectar();

        // 增加蜂蜜等级
        const BlockState* currentState = world.getBlockState(m_pos);
        if (currentState) {
            const auto* beehiveBlock = static_cast<const blocks::BeehiveBlock*>(&currentState->getBlock());
            i32 currentLevel = beehiveBlock->getHoneyLevel(*currentState);
            if (currentLevel < beehiveBlock->getMaxHoneyLevel()) {
                // 1% 概率增加 2 级，99% 增加 1 级
                i32 increment = (m_rng.nextInt(100) == 0) ? 2 : 1;
                i32 newLevel = std::min(currentLevel + increment, beehiveBlock->getMaxHoneyLevel());
                BlockState newState = currentState->with(BlockStateProperties::HONEY_LEVEL_0_5(), newLevel);
                world.setBlockState(m_pos, &newState);
            }
        }
    }

    // 设置重新进入蜂巢的冷却时间
    bee->setStayOutOfHiveCountdown(MIN_TICKS_BEFORE_REENTERING_HIVE);

    // 生成蜜蜂实体
    world.spawnEntity(std::move(beeEntity));

    // 播放蜜蜂离开蜂巢音效
    world.playSound(SoundEvents::BLOCK_BEEHIVE_EXIT,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(m_pos.x + 0.5), static_cast<f32>(m_pos.y + 0.5), static_cast<f32>(m_pos.z + 0.5)),
        1.0f,
        1.0f);

    setChanged();
    return true;
}

void BeehiveBlockEntity::_tickOccupants(IWorld& world)
{
    // 检查火灾，有火时紧急释放所有蜜蜂
    if (isFireNearby(world, m_pos)) {
        const BlockState* state = world.getBlockState(m_pos);
        if (state) {
            emptyAllLivingFromHive(world, nullptr, *state, BeeReleaseStatus::Emergency);
        }
        return;
    }

    // 从后向前遍历：删除当前索引的元素时，前面的元素不受影响，--i 自然指向下一个待检查的蜜蜂
    i32 i = static_cast<i32>(m_bees.size()) - 1;
    while (i >= 0) {
        if (m_bees[i].tick()) {
            BeeReleaseStatus status =
                m_bees[i].hasNectar ? BeeReleaseStatus::HoneyDelivered : BeeReleaseStatus::BeeReleased;
            const BlockState* state = world.getBlockState(m_pos);
            if (state) {
                _releaseOccupant(world, *state, i, status);
            }
        }
        --i;
    }

    // 随机播放工作音效（0.5% 概率）
    if (!m_bees.empty() && m_rng.nextFloat() < 0.005f) {
        world.playSound(SoundEvents::BLOCK_BEEHIVE_WORK,
            sound::SoundCategory::Blocks,
            Vector3(static_cast<f32>(m_pos.x + 0.5), static_cast<f32>(m_pos.y + 0.5), static_cast<f32>(m_pos.z + 0.5)),
            1.0f,
            1.0f);
    }
}

std::optional<BlockPos> BeehiveBlockEntity::_getReleasePosition(
    IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 获取蜂巢朝向，计算出口位置
    Direction facing = Direction::North;
    if (state.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    }

    // 计算出口位置（蜂巢前面）
    BlockPos exitPos = pos.offset(facing);

    // 检查出口是否被阻挡
    const BlockState* exitState = world.getBlockState(exitPos);
    if (exitState && !exitState->getBlock().material().isSolid()) {
        return exitPos;
    }

    // 出口被阻挡，尝试蜂巢上方
    BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState && !aboveState->getBlock().material().isSolid()) {
        return abovePos;
    }

    // 都被阻挡
    return std::nullopt;
}

void BeehiveBlockEntity::angerNearbyBees(IWorld& world, const BlockPos& pos, Player& player)
{
    // 在 8x6x8 范围内搜索蜜蜂实体，使其攻击玩家
    Vector3 center(static_cast<f32>(pos.x + 0.5), static_cast<f32>(pos.y + 0.5), static_cast<f32>(pos.z + 0.5));

    auto entities = world.getEntitiesInRange(center, 8.0f, &player);
    for (auto* entity : entities) {
        auto* bee = dynamic_cast<BeeEntity*>(entity);
        if (bee && !bee->hasStung()) {
            bee->setAngry(true);
            bee->setAttackTarget(&player);
        }
    }
}

} // namespace blockentity
} // namespace mc
