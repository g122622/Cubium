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

#include "TestInstanceBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/interactive/TestInstanceBlockEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

TestInstanceBlock::TestInstanceBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 测试实例方块无状态属性
}

std::unique_ptr<BlockEntity> TestInstanceBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::TestInstanceBlockEntity>(pos);
}

BlockActionResult TestInstanceBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (world.getBlockEntity(pos) == nullptr) {
        return ActionResultType::Pass;
    }

    // 对齐 vanilla TestInstanceBlock#useWithoutItem：无管理员权限返回 PASS；
    // 否则客户端打开测试实例界面（GUI 尚未实现，服务端返回 SUCCESS）。
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Pass;
    }

    // TODO: 打开测试实例界面（对齐 vanilla Player.openTestInstanceBlock → TestInstanceBlockEditScreen），
    //   当前仅返回成功，GUI 待命令方块界面体系接入后补全。
    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
