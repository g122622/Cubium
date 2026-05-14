#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../IWorld.hpp"
#include "../../block/BlockPos.hpp"
#include <optional>

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
 * 参考 MC 1.16.5 ITeleporter
 * 处理实体在维度间的传送逻辑。
 *
 * 使用示例:
 * @code
 * NetherTeleporter teleporter;
 * bool success = teleporter.teleport(entity, DimensionManager::NETHER);
 * @endcode
 */
class Teleporter {
public:
    virtual ~Teleporter() = default;

    // ========== 核心传送方法 ==========

    /**
     * @brief 将实体传送到目标维度
     *
     * @param entity 要传送的实体
     * @param targetDim 目标维度ID
     * @return 是否成功传送
     */
    virtual bool teleport(Entity& entity, DimensionId targetDim) = 0;

    // ========== 传送门搜索 ==========

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
    [[nodiscard]] virtual f32 getCoordinateScale() const { return 1.0f; }

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
    /// MC 1.16.5: 主世界坐标 ÷ 8 = 下界坐标，需要大范围搜索避免分散
    static constexpr i32 OVERWORLD_TO_NETHER_SEARCH_RADIUS = 128;

    /// 从下界到主世界的搜索半径（格）
    /// MC 1.16.5: 下界坐标 × 8 = 主世界坐标，需要小范围搜索
    static constexpr i32 NETHER_TO_OVERWORLD_SEARCH_RADIUS = 16;

    /// 创建新传送门时的搜索半径（格）
    static constexpr i32 CREATE_PORTAL_SEARCH_RADIUS = 16;

    /// 末地传送门固定位置
    /// MC 1.16.5 ServerWorld.field_241108_a_ = new BlockPos(100, 50, 0)
    /// 玩家出生在方块中心，所以加 0.5
    [[nodiscard]] static Vector3d getEndSpawnPosition() { return Vector3d(100.5, 50.0, 0.5); }

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
 *
 * 参考 MC 1.16.5 NetherTeleporter
 */
class NetherTeleporter : public Teleporter {
public:
    NetherTeleporter() = default;
    ~NetherTeleporter() override = default;

    // ========== Teleporter 接口实现 ==========

    bool teleport(Entity& entity, DimensionId targetDim) override;
    [[nodiscard]] std::optional<PortalInfo> findPortal(IWorld& world, const Vector3d& pos) override;
    [[nodiscard]] PortalInfo createPortal(IWorld& world, const Vector3d& pos) override;

    [[nodiscard]] f32 getCoordinateScale() const override { return 8.0f; }

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
 * 参考 MC 1.16.5 EndTeleporter
 */
class EndTeleporter : public Teleporter {
public:
    EndTeleporter() = default;
    ~EndTeleporter() override = default;

    // ========== Teleporter 接口实现 ==========

    bool teleport(Entity& entity, DimensionId targetDim) override;
    [[nodiscard]] std::optional<PortalInfo> findPortal(IWorld& world, const Vector3d& pos) override;
    [[nodiscard]] PortalInfo createPortal(IWorld& world, const Vector3d& pos) override;

    [[nodiscard]] f32 getCoordinateScale() const override { return 1.0f; }

    // ========== 末地特有方法 ==========

    /**
     * @brief 创建末地返回传送门
     *
     * 在末地击败末影龙后创建返回传送门。
     *
     * @param world 末地世界
     * @param pos 创建位置
     */
    static void createExitPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 创建末地出生平台
     *
     * 玩家首次进入末地时创建的黑曜石平台。
     *
     * @param world 末地世界
     */
    static void createEndSpawnPlatform(IWorld& world);

    /**
     * @brief 获取末地出生位置
     */
    [[nodiscard]] static Vector3d getEndSpawnPosition() { return Vector3d(100.0, 49.0, 0.0); }

private:
    /**
     * @brief 放置末地传送门框架
     */
    static void placeEndPortalFrame(IWorld& world, const BlockPos& center);
};

} // namespace mc
