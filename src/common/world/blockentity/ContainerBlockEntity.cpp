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

#include "world/blockentity/ContainerBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/entities/player/Player.hpp"

namespace mc {

void ContainerBlockEntity::openContainer(Player* player)
{
    // 观察者模式玩家不计入打开数
    if (player != nullptr && player->isSpectator()) {
        return;
    }
    // 负数保护（防止数据损坏）
    if (m_openCount < 0) {
        m_openCount = 0;
    }
    ++m_openCount;
}

void ContainerBlockEntity::closeContainer(Player* player)
{
    // 观察者模式玩家不计入打开数
    if (player != nullptr && player->isSpectator()) {
        return;
    }
    if (m_openCount > 0) {
        --m_openCount;
    }
}

bool ContainerBlockEntity::isUsableByPlayer(const Player& player, f32 maxDistanceSq) const
{
    // 检查：
    // 1. 方块实体仍然存在于世界中（m_world != nullptr 且未被移除）
    // 2. 玩家在指定距离范围内

    // 如果方块实体已被移除，返回 false
    if (isRemoved()) {
        return false;
    }

    // 计算玩家与方块中心的距离平方
    const BlockPos pos = getPos();
    return player.distanceSqTo(static_cast<f32>(pos.x) + 0.5f,
               static_cast<f32>(pos.y) + 0.5f,
               static_cast<f32>(pos.z) + 0.5f) <= maxDistanceSq;
}

} // namespace mc
