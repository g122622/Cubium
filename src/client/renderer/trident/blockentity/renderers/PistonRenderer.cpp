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

#include "PistonRenderer.hpp"
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"

namespace mc::client::renderer::trident::blockentity {

PistonRenderer::PistonRenderer()
    : BlockEntityRenderer<mc::blockentity::PistonBlockEntity>()
    , m_helper()
{}

void PistonRenderer::render(const mc::blockentity::PistonBlockEntity& entity, f32 partialTick, u32 light, i64 gameTime)
{
    MC_UNUSED(gameTime);

    // 如果方块状态为空气，不渲染
    // 如果动画完成，方块已经被替换，不需要渲染

    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr || pistonState->isAir()) {
        return;
    }

    // 计算偏移
    const f32 offsetX = entity.getOffsetX(partialTick);
    const f32 offsetY = entity.getOffsetY(partialTick);
    const f32 offsetZ = entity.getOffsetZ(partialTick);
    const f32 progress = entity.getProgress(partialTick);

    // 获取运动方向的反向位置（用于活塞基座渲染）
    const Direction motionDir = entity.getMotionDirection();
    const Direction oppositeDir = Directions::opposite(motionDir);
    const BlockPos basePos = entity.getPos().offset(oppositeDir);

    // 有三种渲染情况
    // 1. 方块是活塞头：设置 SHORT 属性并渲染
    // 2. 需要渲染活塞头且正在收回：渲染活塞头 + 活塞基座
    // 3. 普通情况：渲染被移动的方块

    // 检查是否是活塞头方块
    // 注意：当前方块注册表不完整，无法判断是否为活塞头
    // 完整实现需要检查 pistonState->getBlock() == Blocks::PISTON_HEAD
    const bool isPistonHead = false;

    if (isPistonHead) {
        // 活塞头需要设置 SHORT 属性
        const bool isShort = progress <= 0.5f;
        (void)isShort;

        // 渲染活塞头
        _renderMovingBlock(entity, offsetX, offsetY, offsetZ, light);
    } else if (entity.shouldRenderPistonHead() && !entity.isExtending()) {
        // 收回时需要渲染活塞头
        const bool isShort = progress >= 0.5f;
        (void)isShort;

        // 渲染活塞头
        _renderPistonHead(entity, progress, light);

        // 然后渲染活塞基座（延伸状态）
        const BlockPos pistonBasePos = basePos.offset(motionDir);
        (void)pistonBasePos;
        // 完整实现需要渲染延伸状态的活塞基座
    } else {
        // 普通情况，直接渲染被移动的方块
        _renderMovingBlock(entity, offsetX, offsetY, offsetZ, light);
    }
}

void PistonRenderer::_renderPistonHead(const mc::blockentity::PistonBlockEntity& entity, f32 progress, u32 light)
{
    // 渲染活塞头（收回时需要单独渲染）
    //
    // 活塞头方块状态：
    // - TYPE: STICKY 或 DEFAULT（取决于活塞类型）
    // - FACING: 活塞朝向
    // - SHORT: progress >= 0.5F 时为 true

    const Direction facing = entity.getFacing();
    const BlockPos& pos = entity.getPos();

    // 计算活塞头位置偏移
    const f32 extendedProgress = entity.getExtendedProgress(progress);
    const f32 headOffsetX = static_cast<f32>(Directions::xOffset(facing)) * extendedProgress;
    const f32 headOffsetY = static_cast<f32>(Directions::yOffset(facing)) * extendedProgress;
    const f32 headOffsetZ = static_cast<f32>(Directions::zOffset(facing)) * extendedProgress;

    // 完整实现需要：
    // 1. 获取活塞头方块状态（根据是否粘性选择 STICKY 或 DEFAULT）
    // 2. 设置 FACING 和 SHORT 属性
    // 3. 使用 BlockModelCache 获取模型并渲染

    (void)pos;
    (void)headOffsetX;
    (void)headOffsetY;
    (void)headOffsetZ;
    (void)light;
}

void PistonRenderer::_renderMovingBlock(
    const mc::blockentity::PistonBlockEntity& entity, f32 offsetX, f32 offsetY, f32 offsetZ, u32 light)
{
    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr) {
        return;
    }

    const BlockPos& pos = entity.getPos();

    // 渲染被移动的方块，应用偏移
    // 需要使用 BlockRendererDispatcher 渲染方块模型
    // 当前使用 helper 的占位实现
    if (!m_helper.renderBlockWithOffset(*pistonState, pos, offsetX, offsetY, offsetZ, light)) {
        // 占位实现，暂不处理
        (void)pos;
    }
}

} // namespace mc::client::renderer::trident::blockentity
