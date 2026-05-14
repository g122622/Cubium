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
    virtual bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    /**
     * @brief 设置方块状态（带标志）
     * @param x 世界 X 坐标
     * @param y 世界 Y 坐标
     * @param z 世界 Z 坐标
     * @param state 方块状态
     * @param flags 更新标志（默认为 3：通知邻居 + 更新客户端）
     * @return 是否成功
     */
    virtual bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags)
    {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    /**
     * @brief 设置方块状态（方块位置版本）
     * @param pos 方块位置
     * @param state 方块状态
     * @return 是否成功
     */
    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }
};

} // namespace mc
