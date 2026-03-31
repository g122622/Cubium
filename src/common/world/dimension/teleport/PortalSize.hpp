#pragma once

#include "../../block/BlockPos.hpp"
#include "../../../util/Direction.hpp"
#include "../../../core/Types.hpp"
#include <vector>
#include <optional>

namespace mc {

class IWorld;
class BlockState;

/**
 * @brief 传送门尺寸检测结果
 *
 * 包含检测到的传送门的位置和尺寸信息。
 */
struct PortalSizeResult {
    BlockPos corner;      ///< 传送门内部左下角位置
    i32 width;            ///< 宽度 (2-21)
    i32 height;           ///< 高度 (3-21)
    Direction axis;       ///< 传送门轴向 (X 或 Z)
    bool valid = false;   ///< 是否有效

    /**
     * @brief 获取传送门方块位置列表
     *
     * @return 传送门内部所有方块位置
     */
    [[nodiscard]] std::vector<BlockPos> getPortalBlocks() const;
};

/**
 * @brief 传送门尺寸检测工具
 *
 * 参考 MC 1.16.5 PortalSize
 * 检测黑曜石框架（下界传送门）和末地传送门框架。
 *
 * 下界传送门规则：
 * - 框架由黑曜石构成
 * - 宽度：2-21 格
 * - 高度：3-21 格
 * - 内部必须为空气
 * - 激活后在内部放置下界传送门方块
 *
 * 使用示例:
 * @code
 * auto result = PortalSize::findNetherPortal(world, pos);
 * if (result.valid) {
 *     PortalSize::lightPortal(world, result);
 * }
 * @endcode
 */
class PortalSize {
public:
    // ========== 常量 ==========

    /// 最小宽度
    static constexpr i32 MIN_WIDTH = 2;

    /// 最大宽度
    static constexpr i32 MAX_WIDTH = 21;

    /// 最小高度
    static constexpr i32 MIN_HEIGHT = 3;

    /// 最大高度
    static constexpr i32 MAX_HEIGHT = 21;

    // ========== 下界传送门 ==========

    /**
     * @brief 在指定位置寻找有效的下界传送门框架
     *
     * @param world 世界引用
     * @param pos 搜索中心位置
     * @return 检测结果，如果未找到则 valid=false
     */
    [[nodiscard]] static std::optional<PortalSizeResult> findNetherPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查指定位置是否有有效的下界传送门
     *
     * @param world 世界引用
     * @param pos 检测位置
     * @return 是否有有效传送门
     */
    [[nodiscard]] static bool isNetherPortalAt(IWorld& world, const BlockPos& pos);

    /**
     * @brief 点燃下界传送门
     *
     * 在传送门内部放置下界传送门方块。
     *
     * @param world 世界引用
     * @param portal 传送门检测结果
     * @return 是否成功点燃
     */
    static bool lightNetherPortal(IWorld& world, const PortalSizeResult& portal);

    // ========== 末地传送门 ==========

    /**
     * @brief 在指定位置寻找有效的末地传送门框架
     *
     * @param world 世界引用
     * @param pos 搜索中心位置
     * @return 检测结果，如果未找到则 valid=false
     */
    [[nodiscard]] static std::optional<PortalSizeResult> findEndPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查指定位置是否有有效的末地传送门
     *
     * @param world 世界引用
     * @param pos 检测位置
     * @return 是否有有效传送门
     */
    [[nodiscard]] static bool isEndPortalAt(IWorld& world, const BlockPos& pos);

    /**
     * @brief 激活末地传送门
     *
     * 在传送门内部放置末地传送门方块。
     *
     * @param world 世界引用
     * @param portal 传送门检测结果
     * @return 是否成功激活
     */
    static bool activateEndPortal(IWorld& world, const PortalSizeResult& portal);

    // ========== 工具方法 ==========

    /**
     * @brief 获取传送门内部所有方块位置
     *
     * @param portal 传送门检测结果
     * @return 方块位置列表
     */
    [[nodiscard]] static std::vector<BlockPos> getPortalBlocks(const PortalSizeResult& portal);

private:
    // ========== 内部检测方法 ==========

    /**
     * @brief 检测水平框架线
     *
     * @param world 世界引用
     * @param start 起始位置
     * @param direction 方向
     * @param length 期望长度
     * @param frameBlock 框架方块状态
     * @return 实际长度，如果不符合则返回 -1
     */
    [[nodiscard]] static i32 checkHorizontalFrame(
        IWorld& world,
        const BlockPos& start,
        Direction direction,
        i32 length,
        const BlockState* frameBlock);

    /**
     * @brief 检测垂直框架线
     *
     * @param world 世界引用
     * @param start 起始位置
     * @param length 期望长度
     * @param frameBlock 框架方块状态
     * @return 实际长度，如果不符合则返回 -1
     */
    [[nodiscard]] static i32 checkVerticalFrame(
        IWorld& world,
        const BlockPos& start,
        i32 length,
        const BlockState* frameBlock);

    /**
     * @brief 检测内部是否为空气
     *
     * @param world 世界引用
     * @param corner 内部左下角
     * @param width 宽度
     * @param height 高度
     * @param axis 轴向
     * @return 是否为空气
     */
    [[nodiscard]] static bool checkInteriorEmpty(
        IWorld& world,
        const BlockPos& corner,
        i32 width,
        i32 height,
        Direction axis);

    /**
     * @brief 尝试在指定轴向检测传送门
     *
     * @param world 世界引用
     * @param pos 搜索位置
     * @param axis 轴向 (X 或 Z)
     * @param frameBlock 框架方块
     * @return 检测结果
     */
    [[nodiscard]] static std::optional<PortalSizeResult> tryFindPortalOnAxis(
        IWorld& world,
        const BlockPos& pos,
        Direction axis,
        const BlockState* frameBlock);

    /**
     * @brief 从左下角开始检测完整的传送门框架
     *
     * @param world 世界引用
     * @param bottomLeft 左下角位置
     * @param widthDir 宽度方向
     * @param frameBlock 框架方块
     * @return 检测结果
     */
    [[nodiscard]] static std::optional<PortalSizeResult> detectFrameFromCorner(
        IWorld& world,
        const BlockPos& bottomLeft,
        Direction widthDir,
        const BlockState* frameBlock);
};

} // namespace mc
