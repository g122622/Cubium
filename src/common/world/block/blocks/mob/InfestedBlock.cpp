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

#include "InfestedBlock.hpp"
#include "../../../../entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

InfestedBlock::InfestedBlock(u32 hostBlock, const BlockProperties& properties)
    : Block(properties)
    , m_hostBlock(hostBlock)
{
    // 被感染方块没有特殊状态
}

void InfestedBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // MC 1.16.5: InfestedBlock.spawnAdditionalDrops()
    // 当被破坏时，有概率生成蠹虫
    // 注意：实际生成条件需要检查游戏规则 doTileDrops 和精准采集附魔
    // 这些检查在 onBlockHarvested 或 spawnAdditionalDrops 中进行
    // 这里简化处理：直接生成蠹虫

    MC_UNUSED(state);

    // 只在服务端生成
    if (world.isClientSide()) {
        return;
    }

    // MC 1.16.5: 创建蠹虫实体
    // SilverfishEntity silverfishentity = EntityType.SILVERFISH.create(world);
    // silverfishentity.setLocationAndAngles(pos.getX() + 0.5D, pos.getY(), pos.getZ() + 0.5D, 0.0F, 0.0F);
    // world.addEntity(silverfishentity);
    // silverfishentity.spawnExplosionParticle();

    auto silverfish = std::make_unique<SilverfishEntity>(LegacyEntityType::Silverfish, EntityId(0));
    if (silverfish) {
        // 设置位置（方块中心）
        silverfish->setPosition(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
        silverfish->setRotation(0.0f, 0.0f);

        // 生成到世界
        world.spawnEntity(std::move(silverfish));
    }
}

} // namespace blocks
} // namespace mc
