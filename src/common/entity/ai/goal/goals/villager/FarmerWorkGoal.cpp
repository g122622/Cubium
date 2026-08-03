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

#include "FarmerWorkGoal.hpp"
#include "WorkAtJobSiteGoal.hpp"

#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "common/world/block/blocks/functional/ComposterBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <algorithm>
#include <optional>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

// ============================================================================
// FarmerWorkGoal - 农民工作目标
// ============================================================================
//
// 实现参考：MC 1.21.11 HarvestFarmland + UseBonemeal + WorkAtComposter
// 农民在耕地区域执行：收获成熟作物、种植种子、堆肥多余种子

FarmerWorkGoal::FarmerWorkGoal(VillagerEntity* villager)
    : WorkAtJobSiteGoal(villager)
    , m_farmerWorkTicks(0)
{}

void FarmerWorkGoal::tick()
{
    if (!m_villager) return;

    m_farmerWorkTicks++;

    // 执行基类的工作逻辑
    WorkAtJobSiteGoal::tick();

    // 农民特有行为
    if (m_farmerWorkTicks % FARMER_WORK_INTERVAL == 0) {
        // 当 mobGriefing 为 false 时，农民不能收获和种植作物
        // 堆肥不受 mobGriefing 限制（原版 WorkAtComposter 不检查此规则）
        IWorld* world = m_villager->world();
        const bool canGrief =
            world != nullptr && world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);

        if (canGrief) {
            // 尝试收获
            _tryHarvest();

            // 尝试种植
            _tryPlant();
        }

        // 尝试堆肥
        _tryCompost();
    }
}

void FarmerWorkGoal::_tryHarvest()
{
    if (!m_villager || !m_villager->world()) return;

    IWorld* world = m_villager->world();

    // MC 原版使用蓄水池抽样随机选取一个有效位置
    auto targetPos = _pickValidFarmland();
    if (!targetPos.has_value()) return;

    BlockPos pos = targetPos.value();
    const BlockState* state = world->getBlockState(pos);
    if (!state) return;

    // 只收获成熟作物
    if (!_isCropMatureAt(pos)) return;

    // 收获作物
    _harvestCrop(pos);
}

void FarmerWorkGoal::_tryPlant()
{
    if (!m_villager || !m_villager->world()) return;

    // 检查是否有可种植的种子
    if (!_hasFarmSeeds()) return;

    IWorld* world = m_villager->world();

    // MC 原版使用蓄水池抽样随机选取一个有效位置
    auto targetPos = _pickValidFarmland();
    if (!targetPos.has_value()) return;

    BlockPos pos = targetPos.value();

    // 只在可种植位置种植（空气+下方耕地）
    if (!_canPlantAt(pos)) return;

    // 找到可种植位置，尝试从背包中获取种子并种植
    // 参考 MC 1.21.11 HarvestFarmland.plantCrop：
    //   - 通过 ItemStack.is(ItemTags.VILLAGER_PLANTABLE_SEEDS) 判断可种植物品
    //   - 通过 BlockItem.placeBlock 植入对应方块（不要求方块继承 CropBlock）
    //   - 播放 SoundEvents.ITEM_CROP_PLANT
    //   - 触发 GameEvent.BLOCK_PLACE
    //
    // 本项目中胡萝卜/马铃薯是食物物品（非 BlockItem），需要通过
    // _getCropBlockForSeed 的硬编码映射处理；其他种子（小麦、甜菜、火把花、瓶草）
    // 均为 SeedsItem（BlockItem 子类），通过 BlockItem::block() 直接获取作物方块。
    IInventory& inventory = m_villager->inventory();
    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (!item) continue;

        // 仅 VILLAGER_PLANTABLE_SEEDS 标签中的物品才能被农民种植
        if (!item->isIn(item::tag::ItemTags::VILLAGER_PLANTABLE_SEEDS())) continue;

        // 获取种子对应的作物方块
        const Block* block = _getCropBlockForSeed(item);
        if (block == nullptr) continue;

        // 种植作物：放置默认状态（age=0）
        // 对应 MC: level.setBlock(pos, block.defaultBlockState(), Block.UPDATE_ALL)
        // flags=3 等价于 Block.UPDATE_ALL（更新邻居 + 通知客户端）
        const BlockState& plantState = block->defaultState();
        world->setBlockState(pos, &plantState, 3);

        // 播放种植音效（对应 MC: level.playSound(null, pos, SoundEvents.ITEM_CROP_PLANT, ...)）
        world->playSound(SoundEvents::ITEM_CROP_PLANT,
            sound::SoundCategory::Blocks,
            Vector3(static_cast<f64>(pos.x) + 0.5, static_cast<f64>(pos.y), static_cast<f64>(pos.z) + 0.5),
            1.0f,
            1.0f);

        // 触发 BLOCK_PLACE 游戏事件（对应 MC: level.gameEvent(GameEvent.BLOCK_PLACE, pos, ...)）
        world->gameEvent(
            gameevent::GameEvents::BLOCK_PLACE, pos, gameevent::GameEvent::Context::of(m_villager, &plantState));

        // 消耗一个种子
        inventory.removeItem(slot, 1);

        // 种植一个就返回
        return;
    }
}

void FarmerWorkGoal::_tryCompost()
{
    if (!m_villager) return;

    IWorld* world = m_villager->world();
    if (!world) return;

    // 查找附近的堆肥桶（农民的工作站点就是堆肥桶）
    auto* villageManager = world->villageManager();
    if (!villageManager) return;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    // 搜索最近的堆肥桶
    using namespace world::village::poi;
    auto composterPos = poiStorage.findNearestFree(villagerPos, PointOfInterestType::Composter, 4.0f);

    if (!composterPos.has_value()) return;

    BlockPos pos = composterPos.value();

    // 检查堆肥桶方块
    const BlockState* state = world->getBlockState(pos);
    if (!state) return;

    // 确认是堆肥桶
    if (!dynamic_cast<const blocks::ComposterBlock*>(&state->getBlock())) return;

    i32 level = blocks::ComposterBlock::getLevel(*state);

    // 如果堆肥桶已满（等级8），先取出骨粉
    if (level >= 8) {
        // MC 原版：满堆肥桶取出骨粉
        // empty 方法需要非 const BlockState 引用，这里重新获取方块并构造可变引用
        BlockState mutableState = *state;
        blocks::ComposterBlock::empty(*world, pos, mutableState);

        // 将骨粉加入村民背包
        const Item* boneMeal = Items::BONE_MEAL;
        if (boneMeal) {
            ItemStack boneMealStack(boneMeal, 1);
            IInventory& inventory = m_villager->inventory();
            ItemStack remaining = inventory.addItem(boneMealStack);
            if (!remaining.isEmpty()) {
                // 装不下就丢在地上
                math::Random& rng = m_villager->getRandom();
                ItemDropHelper::spawnItemEntity(world,
                    remaining,
                    m_villager->x(),
                    m_villager->y() + m_villager->eyeHeight() - 0.3,
                    m_villager->z(),
                    rng);
            }
        }
        return;
    }

    // 尝试将多余的种子堆肥
    // MC 原版 WorkAtComposter 只堆肥小麦种子和甜菜种子
    IInventory& inventory = m_villager->inventory();
    static const Item* compostableItems[] = {Items::WHEAT_SEEDS, Items::BEETROOT_SEEDS};

    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (!item) continue;

        // 检查是否是可堆肥物品
        bool isCompostableItem = false;
        for (const Item* compostable : compostableItems) {
            if (item == compostable) {
                isCompostableItem = true;
                break;
            }
        }
        if (!isCompostableItem) continue;

        // MC 原版：保留10个种子，多余的（超过10个的部分，最多20个）用于堆肥
        i32 count = stack.getCount();
        if (count <= 10) continue;

        i32 compostCount = std::min(count - 10, 20);

        // 尝试堆肥，逐个尝试
        for (i32 i = 0; i < compostCount; ++i) {
            // 重新获取当前状态（可能已因前一次堆肥而改变）
            const BlockState* currentState = world->getBlockState(pos);
            if (!currentState) break;

            BlockState newState = blocks::ComposterBlock::attemptCompost(
                *currentState, *world, pos, currentState->getBlockMutable(), item->itemId());

            // 验证世界状态一致性：attemptCompost 内部调用了 world.setBlockState，
            // 此处通过重新读取确认世界状态与返回值一致
            i32 newLevel = blocks::ComposterBlock::getLevel(newState);
            const BlockState* actualState = world->getBlockState(pos);
            if (actualState) {
                i32 actualLevel = blocks::ComposterBlock::getLevel(*actualState);
                // 如果世界实际等级高于返回值中的等级（例如 tick 已将7推进到8），
                // 使用世界实际等级以确保后续逻辑正确
                if (actualLevel > newLevel) {
                    newLevel = actualLevel;
                }
            }

            if (newLevel > level) {
                level = newLevel;

                // 等级达到7时停止（即将完成，20 tick后自动变为8）
                if (level >= 7) {
                    // 从背包移除已堆肥的种子数量
                    inventory.removeItem(slot, i + 1);
                    return;
                }
            }
        }

        // 从背包移除已堆肥的种子数量
        inventory.removeItem(slot, compostCount);
        return;
    }
}

void FarmerWorkGoal::_harvestCrop(const BlockPos& pos)
{
    if (!m_villager || !m_villager->world()) return;

    IWorld* world = m_villager->world();
    const BlockState* state = world->getBlockState(pos);
    if (!state) return;

    // 确认是 CropBlock
    Block& block = state->getBlockMutable();
    auto* cropBlock = dynamic_cast<blocks::CropBlock*>(&block);
    if (!cropBlock) return;

    // 保存当前状态快照，因为后续操作可能改变方块状态
    BlockState cropState = *state;

    // 收获掉落物处理：成熟时掉落作物和种子
    u32 cropItemId = cropBlock->getCropItem();
    u32 seedItemId = cropBlock->getSeedItem();

    // 计算种子掉落数量：成熟时种子掉落 1+0~2
    math::Random& rng = m_villager->getRandom();
    i32 seedCount = 1 + rng.nextInt(3); // 1~3 颗种子

    // 将作物产品放入村民背包
    if (cropItemId != 0) {
        const Item* cropItem = Item::getItem(cropItemId);
        if (cropItem) {
            ItemStack cropStack(cropItem, 1);
            IInventory& inventory = m_villager->inventory();
            ItemStack remaining = inventory.addItem(cropStack);
            // 装不下的掉落在地上
            if (!remaining.isEmpty()) {
                ItemDropHelper::spawnItemEntity(
                    world, remaining, pos.x + 0.5, static_cast<f64>(pos.y), pos.z + 0.5, rng);
            }
        }
    }

    // 种子掉落
    if (seedItemId != 0) {
        const Item* seedItem = Item::getItem(seedItemId);
        if (seedItem) {
            ItemStack seedStack(seedItem, seedCount);
            IInventory& inventory = m_villager->inventory();
            ItemStack remaining = inventory.addItem(seedStack);
            // 装不下的掉落在地上
            if (!remaining.isEmpty()) {
                ItemDropHelper::spawnItemEntity(
                    world, remaining, pos.x + 0.5, static_cast<f64>(pos.y), pos.z + 0.5, rng);
            }
        }
    }

    // 通知方块即将被移除（触发 onBlockRemoved 回调，如耕地湿润度更新等）
    cropBlock->onBlockRemoved(*world, pos, cropState);

    // 将作物方块设为空气
    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState) {
        world->setBlockState(pos, airState, 2);
    }
}

bool FarmerWorkGoal::_hasFarmSeeds() const
{
    if (!m_villager) return false;

    // 参考 MC 1.21.11 HarvestFarmland.validPos()：
    //   villager.getInventory().hasItem(stack -> stack.is(ItemTags.VILLAGER_PLANTABLE_SEEDS))
    // 标签包含 6 种物品：小麦种子、胡萝卜、马铃薯、甜菜种子、火把花种子、瓶草荚果。
    // 数据包（datapacks/Vanilla/.../villager_plantable_seeds.json）载入后会替换硬编码默认值。
    const item::tag::ItemTag& plantableSeeds = item::tag::ItemTags::VILLAGER_PLANTABLE_SEEDS();

    IInventory& inventory = m_villager->inventory();
    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (!item) continue;

        if (plantableSeeds.contains(item)) return true;
    }

    return false;
}

const Block* FarmerWorkGoal::_getCropBlockForSeed(const Item* seedItem)
{
    if (!seedItem) return nullptr;

    // 大部分种子（小麦种子、甜菜种子、火把花种子、瓶草荚果）注册为 SeedsItem
    // （BlockItem 子类），直接通过 BlockItem::block() 获取关联的作物方块。
    const auto* blockItem = dynamic_cast<const BlockItem*>(seedItem);
    if (blockItem != nullptr) {
        return &blockItem->block();
    }

    // 胡萝卜和马铃薯在本项目中是普通食物物品（非 BlockItem），但 MC 原版中
    // 它们属于 VILLAGER_PLANTABLE_SEEDS 标签，村民可以种植。需要直接映射到
    // 对应的作物方块（VanillaBlocks::CARROTS / POTATOES）。
    if (seedItem == Items::CARROT) {
        return VanillaBlocks::CARROTS;
    }
    if (seedItem == Items::POTATO) {
        return VanillaBlocks::POTATOES;
    }

    return nullptr;
}

bool FarmerWorkGoal::_isCropMatureAt(const BlockPos& pos) const
{
    IWorld* world = m_villager->world();
    if (!world) return false;

    const BlockState* state = world->getBlockState(pos);
    if (!state) return false;

    // 检查是否是 CropBlock 且已成熟
    const Block& block = state->getBlock();
    auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(&block);
    if (!cropBlock) return false;

    return cropBlock->isMaxAge(*state);
}

bool FarmerWorkGoal::_canPlantAt(const BlockPos& pos) const
{
    IWorld* world = m_villager->world();
    if (!world) return false;

    // 检查目标位置是否为空气或可替换
    const BlockState* state = world->getBlockState(pos);
    if (!state) return false;
    if (!state->isAir() && !state->canBeReplaced()) return false;

    // 检查下方是否是耕地
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world->getBlockState(belowPos);
    if (!belowState) return false;

    return belowState->is(VanillaBlocks::FARMLAND);
}

bool FarmerWorkGoal::_isValidFarmPos(const BlockPos& pos) const
{
    // 对应 MC HarvestFarmland.validPos()：
    // 1. 位置是 CropBlock 且已成熟（可收获）
    // 2. 位置是空气且下方是耕地（可种植）
    return _isCropMatureAt(pos) || _canPlantAt(pos);
}

std::optional<BlockPos> FarmerWorkGoal::_pickValidFarmland() const
{
    if (!m_villager || !m_villager->world()) return std::nullopt;

    IWorld* world = m_villager->world();
    i32 cx = static_cast<i32>(m_villager->x());
    i32 cy = static_cast<i32>(m_villager->y());
    i32 cz = static_cast<i32>(m_villager->z());

    // MC 原版使用蓄水池抽样算法随机选取有效位置
    i32 count = 0;
    std::optional<BlockPos> result;

    math::Random& rng = m_villager->getRandom();

    for (i32 dx = -FARMER_SEARCH_RANGE; dx <= FARMER_SEARCH_RANGE; ++dx) {
        for (i32 dy = -FARMER_SEARCH_RANGE; dy <= FARMER_SEARCH_RANGE; ++dy) {
            for (i32 dz = -FARMER_SEARCH_RANGE; dz <= FARMER_SEARCH_RANGE; ++dz) {
                BlockPos checkPos(cx + dx, cy + dy, cz + dz);
                if (_isValidFarmPos(checkPos)) {
                    // 蓄水池抽样：以 1/(count+1) 的概率替换当前结果
                    if (rng.nextInt(++count) == 0) {
                        result = checkPos;
                    }
                }
            }
        }
    }

    return result;
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
