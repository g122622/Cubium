#pragma once

#include "../core/Types.hpp"

namespace mc {

// 前向声明
class BlockState;

/**
 * @brief 世界写入器接口
 *
 * 提供结构生成和特征放置时写入世界的抽象接口。
   TODO 为了更清晰的架构，应该放到gen目录下，并且命名为 IWorldGenWriter 之类的名字。命名空间也不要直接放mc下。
 * 参考 MC 1.16.5: net.minecraft.world.IWorldWriter
 */
class IWorldWriter {
public:
    virtual ~IWorldWriter() = default;

    /**
     * @brief 设置方块状态
     * @param x 世界 X 坐标
     * @param y 世界 Y 坐标
     * @param z 世界 Z 坐标
     * @param state 方块状态
     * @return 是否成功
     */
    virtual bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    /**
     * @brief 设置方块状态（带标志）
     * @param x 世界 X 坐标
     * @param y 世界 Y 坐标
     * @param z 世界 Z 坐标
     * @param state 方块状态
     * @param flags 更新标志（默认为 3：通知邻居 + 更新客户端）
     * @return 是否成功
     */
    virtual bool setBlock(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) {
        (void)flags;
        return setBlock(x, y, z, state);
    }

    /**
     * @brief 设置方块状态（方块位置版本）
     * @param pos 方块位置
     * @param state 方块状态
     * @return 是否成功
     */
    bool setBlock(const BlockPos& pos, const BlockState* state) {
        return setBlock(pos.x, pos.y, pos.z, state);
    }
};

} // namespace mc
