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
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include <optional>
#include <vector>

namespace mc {

class ServerWorld;
class Entity;
class Dimension;
class DimensionType;

/**
 * @brief 传送信息
 *
 * 包含传送目标位置和朝向。
 */
struct PortalInfo {
    Vector3d position;  ///< 目标位置
    f32 yaw = 0.0f;     ///< 目标偏航角
    f32 pitch = 0.0f;   ///< 目标俯仰角
    bool valid = false; ///< 是否有效
};

/**
 * @brief 传送器基类
 *
 * 提供传送门坐标换算、搜索与创建能力。
 *
 * 本类**不负责**执行维度切换：那需要维度管理器处理区块加载/卸载、
 * 实体移交与数据包发送，属于 server 侧职责（见 ServerPlayer::changeDimension）。
 * common 层拿不到目标维度世界，因此这里只暴露与具体世界实例无关的传送门操作。
 *
 * 使用示例：
 * @code
 * NetherTeleporter teleporter;
 * auto info = teleporter.findPortal(*targetWorld, targetPos);
 * if (!info.has_value()) {
 *     info = teleporter.createPortal(*targetWorld, targetPos);
 * }
 * @endcode
 */
class Teleporter {
public:
    virtual ~Teleporter() = default;

    /**
     * @brief 在目标世界查找已存在的传送门
     *
     * @param world 目标世界
     * @param pos 搜索中心位置
     * @return 传送门信息，如果未找到则返回空
     */
    [[nodiscard]] virtual std::optional<PortalInfo> findPortal(IWorld& world, const Vector3d& pos) = 0;

    /**
     * @brief 在目标世界创建新传送门
     *
     * @param world 目标世界
     * @param pos 创建位置
     * @return 传送门信息
     */
    [[nodiscard]] virtual PortalInfo createPortal(IWorld& world, const Vector3d& pos) = 0;

    // ========== 坐标转换 ==========

    /**
     * @brief 获取坐标缩放比例
     *
     * 主世界和末地为 1.0，下界为 8.0。
     *
     * @return 缩放比例
     */
    [[nodiscard]] virtual f32 getCoordinateScale() const
    {
        return 1.0f;
    }

    /**
     * @brief 在两个维度间转换位置
     *
     * @param pos 源位置
     * @param from 源维度类型
     * @param to 目标维度类型
     * @return 转换后的位置
     */
    [[nodiscard]] static Vector3d transformPosition(
        const Vector3d& pos, const DimensionType& from, const DimensionType& to);

    // ========== 传送门搜索半径 ==========

    /// 从主世界到下界的搜索半径（格）
    static constexpr i32 OVERWORLD_TO_NETHER_SEARCH_RADIUS = 128;

    /// 从下界到主世界的搜索半径（格）
    static constexpr i32 NETHER_TO_OVERWORLD_SEARCH_RADIUS = 16;

    /// 创建新传送门时的搜索半径（格）
    static constexpr i32 CREATE_PORTAL_SEARCH_RADIUS = 16;

    /// 末地传送门固定位置（方块顶部中心）
    /// 对应方块坐标 (100, 49, 0) 的顶部中心
    [[nodiscard]] static Vector3d getEndSpawnPosition()
    {
        return Vector3d(100.5, 50.0, 0.5);
    }

protected:
    /**
     * @brief 在世界中搜索传送门方块
     *
     * @param world 世界引用
     * @param center 搜索中心
     * @param radius 搜索半径
     * @return 找到的传送门位置列表
     */
    [[nodiscard]] static std::vector<BlockPos> searchPortalBlocks(IWorld& world, const BlockPos& center, i32 radius);

    /**
     * @brief 放置传送门方块
     *
     * @param world 世界引用
     * @param corner 传送门内部左下角
     * @param width 宽度
     * @param height 高度
     * @param axis 轴向
     */
    static void placePortalBlocks(IWorld& world, const BlockPos& corner, i32 width, i32 height, Direction axis);
};

/**
 * @brief 下界传送器
 *
 * 处理主世界和下界之间的传送。
 * 坐标转换比例：1:8（主世界 8 格 = 下界 1 格）
 */
class NetherTeleporter : public Teleporter {
public:
    NetherTeleporter() = default;
    ~NetherTeleporter() override = default;

    // ========== Teleporter 接口实现 ==========

    [[nodiscard]] std::optional<PortalInfo> findPortal(IWorld& world, const Vector3d& pos) override;
    [[nodiscard]] PortalInfo createPortal(IWorld& world, const Vector3d& pos) override;

    [[nodiscard]] f32 getCoordinateScale() const override
    {
        return 8.0f;
    }

private:
    /**
     * @brief 在下界搜索半径内查找传送门
     */
    [[nodiscard]] std::optional<PortalInfo> findPortalInNether(IWorld& world, const BlockPos& pos);

    /**
     * @brief 在主世界搜索半径内查找传送门
     */
    [[nodiscard]] std::optional<PortalInfo> findPortalInOverworld(IWorld& world, const BlockPos& pos);

    /**
     * @brief 创建下界传送门
     */
    [[nodiscard]] PortalInfo createNetherPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 放置黑曜石框架
     */
    static void placeObsidianFrame(IWorld& world, const BlockPos& corner, i32 width, i32 height, Direction axis);
};

/**
 * @brief 末地传送器
 *
 * 处理主世界和末地之间的传送。
 * 坐标无缩放，固定传送到 (100, 49, 0)。
 *
 * 末地出口传送门（讲台）结构：
 * - 中心柱（4格基岩）
 * - 传送门环（内径 2.5，外径 3.5 的圆环状基岩/末地石结构）
 * - 4个墙上火把
 * - 传送门方块填充圆环内部
 *
 */
class EndTeleporter : public Teleporter {
public:
    EndTeleporter() = default;
    ~EndTeleporter() override = default;

    // ========== Teleporter 接口实现 ==========

    [[nodiscard]] std::optional<PortalInfo> findPortal(IWorld& world, const Vector3d& pos) override;
    [[nodiscard]] PortalInfo createPortal(IWorld& world, const Vector3d& pos) override;

    [[nodiscard]] f32 getCoordinateScale() const override
    {
        return 1.0f;
    }

    // ========== 末地特有方法 ==========

    /**
     * @brief 创建末地出口传送门（讲台）
     *
     * 在末地击败末影龙后创建返回主世界的传送门讲台。
     * 讲台由基岩柱、传送门环和墙上火把组成。
     * active=true 时放置末地传送门方块（已击败龙），
     * active=false 时仅放置空气（未击败龙或龙正在重生）。
     *
     * 调用方：EnderDragonEntity::_onDeathUpdate()（龙死亡动画结束时）与
     * EndDragonFight（龙重生/重置时以 active=false 清除讲台）。
     *
     * @param world 末地世界
     * @param pos 讲台中心底部位置
     * @param active 是否为激活状态（放置传送门方块）
     */
    static void createExitPortal(IWorld& world, const BlockPos& pos, bool active);

    /**
     * @brief 创建末地出生平台
     *
     * 玩家首次进入末地时创建的黑曜石平台。
     * 在 (100, 48, 0) 放置 5×5 黑曜石，并在上方清除 5×5×4 的空气空间。
     *
     * @param world 末地世界
     */
    static void createEndSpawnPlatform(IWorld& world);

    /**
     * @brief 获取末地出生位置
     *
     * 返回末地出生平台中心上方一格的位置 (100.5, 50.0, 0.5)。
     * 此位置对应方块 (100, 49, 0) 的顶部中心。
     */
    [[nodiscard]] static Vector3d getEndSpawnPosition()
    {
        return Vector3d(100.5, 50.0, 0.5);
    }

    /// 末地出口传送门讲台半径
    static constexpr i32 PODIUM_RADIUS = 4;

    /// 末地出口传送门中心柱高度
    static constexpr i32 PODIUM_PILLAR_HEIGHT = 4;
};

} // namespace mc
