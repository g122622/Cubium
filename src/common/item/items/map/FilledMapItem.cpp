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

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDataManager.hpp"
#include "common/world/map/MaterialColor.hpp"
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
        _updateMapData(world, entity, *mapData);
    }
}

ItemActionResult FilledMapItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    auto* mapData = getMapData(player.getHeldItem(hand), world);
    if (mapData != nullptr) {
        // 服务端：更新玩家追踪装饰后返回成功
        // TODO(Phase6): 客户端地图渲染链尚未上线，map-data 解析已删，待重建后在此打开地图屏幕
    }
    return ItemActionResult::success(player.getHeldItem(hand));
}

ActionResultType FilledMapItem::onItemUse(ItemUseContext& context)
{
    // 检查点击的方块是否为旗帜
    // 如果是旗帜，在地图上添加/切换旗帜标记
    auto* mapData = getMapData(context.itemStack(), context.world());
    if (mapData != nullptr) {
        auto* blockEntity = context.world().getBlockEntity(context.blockPos());
        if (dynamic_cast<blockentity::BannerEntity*>(blockEntity) != nullptr) {
            if (mapData->tryAddBanner(context.world(), context.blockPos())) {
                return ActionResultType::Success;
            }
        }
    }

    // 默认行为：与右键使用相同
    return ActionResultType::Pass;
}

void FilledMapItem::onCraftedPostProcess(ItemStack& stack, IWorld& world)
{
    auto* tag = stack.getTag();

    // 处理 map_scale_direction NBT 标签（地图缩放）
    if (tag != nullptr && tag->contains("map_scale_direction")) {
        i32 scaleDirection = (*tag)["map_scale_direction"].is_number() ? (*tag)["map_scale_direction"].get<i32>() : 0;
        if (scaleDirection != 0) {
            scaleMap(stack, world, scaleDirection);
        }
        // 移除标签，只处理一次
        stack.removeChildTag("map_scale_direction");
    }

    // 处理 map_lock NBT 标签（地图锁定）
    if (tag != nullptr && tag->contains("map_lock")) {
        lockMap(world, stack);
        // 移除标签，只处理一次
        stack.removeChildTag("map_lock");
    }
}

world::map::MapData* FilledMapItem::getMapData(const ItemStack& stack, IWorld& world)
{
    i32 mapId = getMapId(stack);
    if (mapId < 0) {
        return nullptr;
    }

    auto* manager = world.mapDataManager();
    if (manager == nullptr) {
        return nullptr;
    }

    return manager->getMapData(mapId);
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

    auto* manager = world.mapDataManager();
    if (manager != nullptr) {
        i32 mapId = manager->createMap(x, z, scale, trackingPosition, unlimitedTracking);
        result.getOrCreateTag()["map"] = mapId;
    }

    return result;
}

void FilledMapItem::scaleMap(ItemStack& stack, IWorld& world, i32 scaleChange)
{
    auto* oldData = getMapData(stack, world);
    if (oldData == nullptr) {
        return;
    }

    i32 newScale = std::clamp(oldData->scale() + scaleChange, 0, world::map::MapData::MAX_SCALE);

    auto* manager = world.mapDataManager();
    if (manager == nullptr) {
        return;
    }

    // 创建新缩放级别的地图
    i32 newMapId = manager->createMap(
        oldData->xCenter(), oldData->zCenter(), newScale, oldData->trackingPosition(), oldData->unlimitedTracking());

    // 更新物品的地图ID
    stack.getOrCreateTag()["map"] = newMapId;
}

void FilledMapItem::lockMap(IWorld& world, ItemStack& stack)
{
    auto* data = getMapData(stack, world);
    if (data == nullptr) {
        return;
    }

    if (!data->locked()) {
        auto* manager = world.mapDataManager();
        if (manager != nullptr) {
            // 创建一个锁定的副本
            i32 lockedMapId = manager->allocateMapId();
            auto* lockedData = manager->createMapData(lockedMapId);
            if (lockedData != nullptr) {
                lockedData->lockFrom(*data);
                // 更新物品的地图ID指向锁定的副本
                stack.getOrCreateTag()["map"] = lockedMapId;
            }
        } else {
            // 无管理器时直接锁定原数据
            data->setLocked(true);
        }
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
    const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const
{
    // world 为 null 时（客户端 Player 无 IWorld）跳过缩放级别提示，
    // 对应 MC 1.21.11 EMPTY TooltipContext 的 mapData 返回 null。
    if (world != nullptr) {
        auto* data = getMapData(stack, *world);
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
    }

    if (advanced) {
        i32 mapId = getMapId(stack);
        tooltip.push_back("Map #" + std::to_string(mapId));
    }
}

void FilledMapItem::_updateMapData(IWorld& world, Entity& viewer, world::map::MapData& data)
{
    const i32 scale = data.scale();
    const i32 centerX = data.xCenter();
    const i32 centerZ = data.zCenter();
    const i32 mapSize = world::map::MapData::MAP_SIZE; // 128

    // 计算每个像素对应的世界坐标范围
    // scale=0: 1像素=1方块, scale=1: 1像素=2方块, ...scale=n: 1像素=2^n方块
    const i32 pixelSize = 1 << scale;

    // 计算地图左上角的世界坐标
    const i32 worldOriginX = centerX - (mapSize / 2) * pixelSize;
    const i32 worldOriginZ = centerZ - (mapSize / 2) * pixelSize;

    // 仅更新玩家附近的区域（性能优化）
    const f64 viewerX = viewer.position().x;
    const f64 viewerZ = viewer.position().z;

    // 玩家在地图上的像素坐标
    const i32 viewerPixelX = static_cast<i32>((viewerX - static_cast<f64>(worldOriginX)) / static_cast<f64>(pixelSize));
    const i32 viewerPixelZ = static_cast<i32>((viewerZ - static_cast<f64>(worldOriginZ)) / static_cast<f64>(pixelSize));

    // 更新范围：玩家周围32像素
    const i32 UPDATE_RADIUS = 32;
    const i32 minX = std::max(0, viewerPixelX - UPDATE_RADIUS);
    const i32 maxX = std::min(mapSize - 1, viewerPixelX + UPDATE_RADIUS);
    const i32 minZ = std::max(0, viewerPixelZ - UPDATE_RADIUS);
    const i32 maxZ = std::min(mapSize - 1, viewerPixelZ + UPDATE_RADIUS);

    f64 prevHeight = 0.0;

    for (i32 z = minZ; z <= maxZ; ++z) {
        for (i32 x = minX; x <= maxX; ++x) {
            const i32 worldX = worldOriginX + x * pixelSize;
            const i32 worldZ = worldOriginZ + z * pixelSize;

            f64 height = 0.0;
            u8 colorIndex = _getTopBlockColor(world, worldX, worldZ, scale, centerX, centerZ, height);

            // 计算阴影（基于高度差）
            u8 shadeIndex = 0;
            if (x > 0 || z > 0) {
                if (height > prevHeight) {
                    shadeIndex = 2; // 高亮
                } else if (height < prevHeight) {
                    shadeIndex = 1; // 中等阴影
                } else {
                    shadeIndex = 0; // 低阴影
                }
            }

            // 特殊处理水面
            // 在MC中，水面根据深度有不同的阴影
            if (colorIndex == static_cast<u8>(world::map::MaterialColorId::WATER)) {
                // 水面使用特殊的深度着色
                shadeIndex = (height > prevHeight) ? 2 : ((height < prevHeight) ? 0 : 1);
            }

            const u8 pixelValue = (colorIndex == 0) ? 0 : static_cast<u8>(colorIndex * 4 + shadeIndex);
            data.setColor(x, z, pixelValue);
            prevHeight = height;

            // 检查并移除该位置已失效的旗帜标记
            // 参考: net.minecraft.world.item.MapItem.update 中对 checkBanners 的调用
            data.removeStaleBanners(world, worldX, worldZ);
        }
    }
}

u8 FilledMapItem::_getTopBlockColor(IWorld& world, i32 x, i32 z, i32 scale, i32 centerX, i32 centerZ, f64& outHeight)
{
    MC_UNUSED(scale);
    MC_UNUSED(centerX);
    MC_UNUSED(centerZ);

    // 获取最高非空气方块
    const i32 topY = world.getHeight(x, z);
    if (topY <= world::MIN_BUILD_HEIGHT) {
        outHeight = 0.0;
        return static_cast<u8>(world::map::MaterialColorId::AIR);
    }

    // 从顶部向下查找第一个非空气方块
    for (i32 y = topY; y >= world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* blockState = world.getBlockState(x, y, z);
        if (blockState != nullptr && !blockState->isAir()) {
            outHeight = static_cast<f64>(y);

            // 获取方块的地图颜色
            world::map::MaterialColorId mapColor = blockState->getMapColor();
            return static_cast<u8>(mapColor);
        }
    }

    outHeight = 0.0;
    return static_cast<u8>(world::map::MaterialColorId::AIR);
}

} // namespace mc::item::items
