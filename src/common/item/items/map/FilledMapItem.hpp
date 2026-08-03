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

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/map/AbstractMapItem.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/map/MapDecoration.hpp"
#include <string>
#include <vector>

namespace mc::world::map {
class MapData;
}

namespace mc {

class Player;
class IWorld;
class Entity;

namespace item::items {

/**
 * @brief 已填充地图物品
 *
 * 包含地图ID，显示地形和玩家位置。支持缩放、锁定和复制。
 */
class FilledMapItem : public AbstractMapItem {
public:
    explicit FilledMapItem(const ItemProperties& properties);

    /**
     * @brief 每tick在物品栏中调用 - 更新地形和玩家追踪
     */
    void inventoryTick(ItemStack& stack, IWorld& world, Entity& entity, i32 itemSlot, bool isSelected) const override;

    /**
     * @brief 右键使用 - 打开地图界面
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 在方块上使用 - 支持旗帜标记
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 合成后处理
     *
     * 处理 map_scale_direction 和 map_lock NBT 标签，
     * 执行地图缩放或锁定操作。
     */
    void onCraftedPostProcess(ItemStack& stack, IWorld& world) override;

    /**
     * @brief 获取地图数据
     */
    [[nodiscard]] static world::map::MapData* getMapData(const ItemStack& stack, IWorld& world);

    /**
     * @brief 获取物品NBT中存储的地图ID
     */
    [[nodiscard]] static i32 getMapId(const ItemStack& stack);

    /**
     * @brief 获取地图名称（用于存储键）
     */
    [[nodiscard]] static std::string getMapName(i32 mapId);

    /**
     * @brief 创建一张新的已填充地图
     */
    [[nodiscard]] static ItemStack setupNewMap(
        IWorld& world, i32 x, i32 z, i32 scale, bool trackingPosition, bool unlimitedTracking);

    /**
     * @brief 缩放地图
     *
     * @param stack 地图物品堆
     * @param world 世界
     * @param scaleChange 缩放变化量（1=放大一级，-1=缩小一级）
     */
    static void scaleMap(ItemStack& stack, IWorld& world, i32 scaleChange);

    /**
     * @brief 锁定地图
     */
    static void lockMap(IWorld& world, ItemStack& stack);

    /**
     * @brief 添加目标装饰（探险地图用）
     */
    static void addTargetDecoration(
        ItemStack& map, const BlockPos& target, const std::string& name, world::map::DecorationType type);

    /**
     * @brief 检查是否为探险地图
     */
    [[nodiscard]] static bool isExplorationMap(const ItemStack& stack);

    /**
     * @brief 检查是否为已填充地图
     */
    [[nodiscard]] static bool isFilledMap(const ItemStack& stack);

    /**
     * @brief 获取使用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& /*stack*/) const override { return UseAction::None; }

    /**
     * @brief 地图不可堆叠
     */
    [[nodiscard]] i32 getMaxStackSize() const { return 1; }

    /**
     * @brief 添加提示信息（地图ID和缩放级别）
     *
     * world 为 null 时（客户端无 IWorld）跳过缩放级别提示，
     * 对应 MC 1.21.11 EMPTY TooltipContext 的 mapData 返回 null。
     */
    void addInformation(
        const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const override;

private:
    /**
     * @brief 更新地图地形数据
     *
     * 扫描玩家可见区域的方块颜色，填充MapData的颜色数组。
     *
     * @param world 世界
     * @param viewer 查看者实体
     * @param data 地图数据
     */
    static void _updateMapData(IWorld& world, Entity& viewer, world::map::MapData& data);

    /**
     * @brief 获取指定位置最高方块的地图颜色
     *
     * @param world 世界
     * @param x 世界X坐标
     * @param z 世界Z坐标
     * @param scale 缩放级别
     * @param centerX 地图中心X
     * @param centerZ 地图中心Z
     * @param outHeight 输出高度（用于阴影计算）
     * @return 地图颜色索引字节
     */
    [[nodiscard]] static u8 _getTopBlockColor(
        IWorld& world, i32 x, i32 z, i32 scale, i32 centerX, i32 centerZ, f64& outHeight);
};

} // namespace item::items
} // namespace mc
