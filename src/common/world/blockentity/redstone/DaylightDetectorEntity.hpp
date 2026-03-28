#pragma once

#include "world/blockentity/BlockEntity.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 日光探测器方块实体
 *
 * 日光探测器使用方块 tick 来管理定期更新，每20游戏tick更新一次信号强度。
 * 这个方块实体主要用于未来的扩展（如自定义名称存储）。
 *
 * ## 注意
 * 当前的日光探测器使用方块 tick 机制而非方块实体 tick。
 * 此实体存在主要是为了与 MC Java 架构保持一致。
 *
 * ## 参考
 * - MC 1.16.5: net.minecraft.tileentity.DaylightDetectorTileEntity
 */
class DaylightDetectorEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit DaylightDetectorEntity(const BlockPos& pos);

    /**
     * @brief 创建方块实体的副本
     * @return 副本的unique_ptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;
};

} // namespace blockentity
} // namespace mc
