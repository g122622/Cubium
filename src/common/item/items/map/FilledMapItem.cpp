/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or/or sell
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

#include "FilledMapItem.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/assert/AssertMacros.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/dimension/MapDimensionId.hpp"
#include "../../../world/map/MapData.hpp"
#include "../../../world/map/MapDataManager.hpp"
#include "../../../world/map/MaterialColor.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../core/ItemRegistry.hpp"
#include "../../core/ItemStack.hpp"
#include <algorithm>
#include <cmath>

namespace mc::item::items {

FilledMapItem::FilledMapItem(const ItemProperties& properties)
    : AbstractMapItem(properties)
{}

void FilledMapItem::inventoryTick(ItemStack& stack, IWorld& world, Entity& entity, i32 itemSlot, bool isSelected) const
{
    if (world.isClientSide()) {
        return;
    }

    auto* mapData = getMapData(stack, world);
    if (mapData == nullptr) {
        return;
    }

    // 更新玩家位置标记
    auto* player = dynamic_cast<Player*>(&entity);
    if (player != nullptr) {
        mapData->updateDecoration(world::map::DecorationType::PLAYER,
            &world,
            "player-" + std::to_string(player->id()),
            player->position().x,
            player->position().z,
            static_cast<f64>(player->yaw()),
            nullptr);
    }

    // 如果地图未锁定且被选中，更新地形
    if (!mapData->locked() && isSelected) {
        // TODO: updateMapData(world, entity, *mapData);
        // 地形更新逻辑需要遍历可见区域获取方块颜色，较为复杂
        // 将在后续迭代中实现
    }
}

ItemActionResult FilledMapItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    auto* mapData = getMapData(player.getHeldItem(hand), world);
    if (mapData != nullptr) {
        // TODO: 打开地图界面
        // 在客户端实现MapScreen
    }
    return ItemActionResult::success(player.getHeldItem(hand));
}

ActionResultType FilledMapItem::onItemUse(ItemUseContext& context)
{
    // 检查点击的方块是否为旗帜
    // 如果是旗帜，在地图上添加旗帜标记
    auto* blockState = context.world().getBlockState(context.blockPos());
    if (blockState != nullptr) {
        // TODO: 检查方块是否为旗帜 (BlockTags::BANNERS)
        // 如果是旗帜，调用 mapData->tryAddBanner()
    }

    // 默认行为：与右键使用相同
    return ActionResultType::Pass;
}

world::map::MapData* FilledMapItem::getMapData(const ItemStack& stack, IWorld& world)
{
    i32 mapId = getMapId(stack);
    if (mapId < 0) {
        return nullptr;
    }

    // TODO: 通过ServerWorld获取MapDataManager
    // auto* manager = world.getMapDataManager();
    // return manager ? manager->getMapData(mapId) : nullptr;
    (void)world;
    return nullptr;
}

i32 FilledMapItem::getMapId(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return -1;
    }

    const auto* tag = stack.getTag();
    if (tag != nullptr && tag->contains("map") && (*tag)["map"].is_number()) {
        return (*tag)["map"].get<i32>();
    }
    return -1;
}

std::string FilledMapItem::getMapName(i32 mapId)
{
    return world::map::MapData::getMapName(mapId);
}

ItemStack FilledMapItem::setupNewMap(
    IWorld& world, i32 x, i32 z, i32 scale, bool trackingPosition, bool unlimitedTracking)
{
    ItemStack result(ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map")), 1);

    // TODO: 通过ServerWorld获取MapDataManager
    // i32 mapId = world.getMapDataManager()->createMap(world, x, z, scale, trackingPosition, unlimitedTracking);
    // result.getOrCreateTag()["map"] = mapId;

    return result;
}

void FilledMapItem::scaleMap(ItemStack& stack, IWorld& world, i32 scaleChange)
{
    auto* oldData = getMapData(stack, world);
    if (oldData == nullptr) {
        return;
    }

    i32 newScale = std::clamp(oldData->scale() + scaleChange, 0, world::map::MapData::MAX_SCALE);

    // 创建新缩放级别的地图
    // TODO: 通过MapDataManager创建新地图
    (void)newScale;
}

void FilledMapItem::lockMap(IWorld& world, ItemStack& stack)
{
    auto* data = getMapData(stack, world);
    if (data == nullptr) {
        return;
    }

    if (!data->locked()) {
        // 创建一个锁定的副本
        // TODO: 通过MapDataManager创建锁定地图
        data->setLocked(true);
    }
}

void FilledMapItem::addTargetDecoration(
    ItemStack& map, const BlockPos& target, const std::string& name, world::map::DecorationType type)
{
    auto& tag = map.getOrCreateTag();

    // 在NBT中添加Decorations列表
    nlohmann::json decorations = nlohmann::json::array();
    if (tag.contains("Decorations")) {
        decorations = tag["Decorations"];
    }

    nlohmann::json decoration;
    decoration["type"] = static_cast<i32>(type);
    decoration["id"] = name;
    decoration["x"] = static_cast<f64>(target.x);
    decoration["z"] = static_cast<f64>(target.z);
    decoration["rot"] = 180.0;

    decorations.push_back(decoration);
    tag["Decorations"] = decorations;

    // 设置地图颜色（用于物品栏显示）
    if (world::map::hasMapColor(type)) {
        i32 color = world::map::getMapColor(type);
        tag["display"]["MapColor"] = color;
    }
}

bool FilledMapItem::isExplorationMap(const ItemStack& stack)
{
    // 探险地图在Decorations中包含MANSION或MONUMENT标记
    const auto* tag = stack.getTag();
    if (tag == nullptr || !tag->contains("Decorations")) {
        return false;
    }

    const auto& decorations = (*tag)["Decorations"];
    if (!decorations.is_array()) {
        return false;
    }

    for (const auto& deco : decorations) {
        i32 type = deco.value("type", 0);
        if (type == static_cast<i32>(world::map::DecorationType::MANSION) ||
            type == static_cast<i32>(world::map::DecorationType::MONUMENT)) {
            return true;
        }
    }
    return false;
}

bool FilledMapItem::isFilledMap(const ItemStack& stack)
{
    return !stack.isEmpty() && stack.getItem() != nullptr &&
        stack.getItem()->itemLocation() == ResourceLocation("minecraft:filled_map");
}

void FilledMapItem::addInformation(
    const ItemStack& stack, IWorld& world, std::vector<std::string>& tooltip, bool advanced) const
{
    auto* data = getMapData(stack, world);
    if (data != nullptr) {
        // 缩放级别提示
        i32 scale = data->scale();
        std::string scaleText;
        switch (scale) {
            case 0:
                scaleText = "1:1";
                break;
            case 1:
                scaleText = "1:2";
                break;
            case 2:
                scaleText = "1:4";
                break;
            case 3:
                scaleText = "1:8";
                break;
            case 4:
                scaleText = "1:16";
                break;
            default:
                scaleText = "1:" + std::to_string(1 << scale);
                break;
        }
        tooltip.push_back(scaleText);

        // 锁定状态提示
        if (data->locked()) {
            tooltip.push_back("Locked");
        }
    }

    if (advanced) {
        i32 mapId = getMapId(stack);
        tooltip.push_back("Map #" + std::to_string(mapId));
    }
}

} // namespace mc::item::items
