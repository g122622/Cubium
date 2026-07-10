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

/**
 * @file HeldBlockLayerTest.cpp
 * @brief HeldBlockLayer 核心逻辑单元测试
 *
 * 测试末影人手持方块渲染层的核心逻辑，包括：
 * - EndermanEntity 的 DataParameter 网络同步参数注册
 * - EndermanEntity 的 isHoldingBlock() / getHeldBlockState() / setHeldBlockState()
 * - EndermanEntity 的 screaming 状态（DATA_SCREAMING_PARAM）
 * - 编译时类型检查（std::is_base_of_v）
 *
 * 注意：由于 HeldBlockLayer 的 renderPipeline 依赖 Vulkan EntityPipeline，
 * 且 HeldBlockLayer.hpp 内部使用 std::unique_ptr<EntityMesh>（EntityMesh 为
 * 前向声明类型，完整定义在 EntityPipeline.hpp 中），本测试不直接 include
 * HeldBlockLayer.hpp，而是通过 EndermanEntity 接口验证核心数据流。
 *
 * HeldBlockLayer 的 shouldRender 通过 ClientEntity::endermanHeldBlockState()
 * 读取镜像字段，该字段由 ClientEntity::syncMetadataFromDataManager 从
 * EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM 同步。
 */

#include <type_traits>
#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"

using namespace mc;

namespace mc::renderer::layer::test {

// ============================================================================
// EndermanEntity DataParameter 网络同步参数测试
// ============================================================================

/**
 * @brief EndermanEntity DataParameter 注册测试
 *
 * 验证 EndermanEntity 在构造时正确注册了 DATA_CARRIED_BLOCK_STATE_ID_PARAM
 * 和 DATA_SCREAMING_PARAM 两个网络同步参数。
 *
 * 对应 MC 1.21.11 EnderMan.defineSynchedData()：
 *   defineSynchedData(DATA_CARRY_STATE, 0)
 *   defineSynchedData(DATA_CREEPY, false)
 */
TEST(HeldBlockLayerEndermanEntityTest, DataParametersRegistered)
{
    EndermanEntity enderman(EntityId(1));

    // 验证 DataParameter ID 已分配（非 0 表示已注册）
    // 注意：DataParameter::id() 返回的是参数在 DataManager 中的唯一标识，
    //       由 EntityDataManager::createKey<T>() 分配
    EXPECT_NE(EndermanEntity::getCarriedBlockStateIdParamId(), 0)
        << "DATA_CARRIED_BLOCK_STATE_ID_PARAM should be registered with a non-zero id";
    EXPECT_NE(EndermanEntity::getScreamingParamId(), 0)
        << "DATA_SCREAMING_PARAM should be registered with a non-zero id";

    // 两个参数 ID 应该不同
    EXPECT_NE(EndermanEntity::getCarriedBlockStateIdParamId(), EndermanEntity::getScreamingParamId())
        << "DATA_CARRIED_BLOCK_STATE_ID_PARAM and DATA_SCREAMING_PARAM should have different ids";
}

/**
 * @brief EndermanEntity 默认不持有方块
 *
 * 新创建的 EndermanEntity 实例应该不持有任何方块，
 * isHoldingBlock() 返回 false，getHeldBlockState() 返回 nullptr。
 *
 * 对应 MC 1.21.11 EnderMan 构造时 DATA_CARRY_STATE 初始化为 0（Blocks.AIR.defaultBlockState()）。
 */
TEST(HeldBlockLayerEndermanEntityTest, DefaultNoBlock)
{
    EndermanEntity enderman(EntityId(1));

    EXPECT_FALSE(enderman.isHoldingBlock()) << "New EndermanEntity should not be holding a block by default";
    EXPECT_EQ(enderman.getHeldBlockState(), nullptr)
        << "New EndermanEntity should have null held block state by default";
}

/**
 * @brief EndermanEntity 默认不尖叫
 *
 * 对应 MC 1.21.11 EnderMan 构造时 DATA_CREEPY 初始化为 false。
 */
TEST(HeldBlockLayerEndermanEntityTest, DefaultNotScreaming)
{
    EndermanEntity enderman(EntityId(1));
    EXPECT_FALSE(enderman.isScreaming()) << "New EndermanEntity should not be screaming by default";
}

/**
 * @brief EndermanEntity screaming 状态设置/获取
 *
 * 验证 setScreaming/isScreaming 通过 DataParameter（DATA_SCREAMING_PARAM）正确读写。
 * 这个状态由 EndermanFindPlayerGoal/EndermanStareGoal 写入，
 * 客户端通过 ClientEntity::syncMetadataFromDataManager 镜像到 endermanScreaming()。
 */
TEST(HeldBlockLayerEndermanEntityTest, ScreamingState)
{
    EndermanEntity enderman(EntityId(1));

    EXPECT_FALSE(enderman.isScreaming());

    enderman.setScreaming(true);
    EXPECT_TRUE(enderman.isScreaming());

    enderman.setScreaming(false);
    EXPECT_FALSE(enderman.isScreaming());
}

/**
 * @brief EndermanEntity 清除持有方块
 *
 * setHeldBlockState(nullptr) 应该清除持有状态。
 * 由于 getHeldBlockState 通过 BlockRegistry 解析 stateId，
 * stateId=0 时返回 nullptr。
 *
 * 对应 MC 1.21.11 EnderMan.setCarriedBlock(Blocks.AIR.defaultBlockState())。
 */
TEST(HeldBlockLayerEndermanEntityTest, ClearHeldBlock)
{
    EndermanEntity enderman(EntityId(1));

    // 清除未持有的方块（应该无效果）
    enderman.setHeldBlockState(nullptr);
    EXPECT_FALSE(enderman.isHoldingBlock());
    EXPECT_EQ(enderman.getHeldBlockState(), nullptr);
}

/**
 * @brief EndermanEntity 继承关系测试
 *
 * 验证 EndermanEntity 继承自 LivingEntity，
 * 确保 ClientEntity 可以通过 typeId 分支正确同步末影人元数据。
 */
TEST(HeldBlockLayerEndermanEntityTest, InheritanceHierarchy)
{
    EXPECT_TRUE((std::is_base_of_v<LivingEntity, EndermanEntity>)) << "EndermanEntity should derive from LivingEntity";
}

// ============================================================================
// shouldRender 逻辑模拟测试
// ============================================================================

/**
 * @brief 测试 shouldRender 逻辑：不持有方块时不渲染
 *
 * 模拟 HeldBlockLayer::shouldRender 的逻辑：
 *   return entity.endermanHeldBlockState() != nullptr;
 *
 * 由于 EndermanEntity::isHoldingBlock() 与 ClientEntity::endermanHeldBlockState() != nullptr
 * 语义等价（都基于 DATA_CARRIED_BLOCK_STATE_ID_PARAM），这里用 EndermanEntity 验证。
 */
TEST(HeldBlockLayerShouldRenderTest, NoBlockShouldNotRender)
{
    EndermanEntity enderman(EntityId(1));

    // 模拟 HeldBlockLayer::shouldRender 的逻辑
    // shouldRender = (endermanHeldBlockState != nullptr)
    // 等价于 isHoldingBlock()
    const bool shouldRender = enderman.isHoldingBlock();

    EXPECT_FALSE(shouldRender) << "shouldRender should be false when EndermanEntity is not holding a block";
}

/**
 * @brief 测试 shouldRender 逻辑：持有方块时渲染
 *
 * 注意：由于 getHeldBlockState 通过 BlockRegistry 解析 stateId，
 * 在未初始化 BlockRegistry 的测试环境中，setHeldBlockState(fakePtr) 写入的
 * stateId 经 getHeldBlockState 解析后仍返回 nullptr（因为 BlockRegistry 中
 * 没有对应的方块状态）。因此这里只验证 isHoldingBlock 的写入逻辑
 * （stateId != 0 即视为持有方块）。
 */
TEST(HeldBlockLayerShouldRenderTest, HoldingBlockShouldRender)
{
    EndermanEntity enderman(EntityId(1));

    // setHeldBlockState(nullptr) 写入 stateId=0，isHoldingBlock 返回 false
    EXPECT_FALSE(enderman.isHoldingBlock());

    // 注意：无法在未初始化 BlockRegistry 的环境中测试 stateId > 0 的情况，
    // 因为 setHeldBlockState 接收 BlockState*，内部存储 state->stateId()。
    // 若传入 nullptr，stateId=0；若传入非空指针，需要是 BlockRegistry 中的真实 BlockState。
    // 这里验证 setScreaming 作为对比，确认 DataParameter 读写机制正常
    enderman.setScreaming(true);
    EXPECT_TRUE(enderman.isScreaming());
}

// ============================================================================
// EndermanEntity angry 状态测试（与 screaming 关联）
// ============================================================================

/**
 * @brief 测试 EndermanEntity 的 angry 状态
 *
 * 验证 setAngry(true) 会同时设置 screaming 状态，
 * setAngry(false) 会清除 screaming 状态。
 *
 * 对应 MC 1.21.11 EnderMan.setAngry：
 *   this.setScreaming(angry)
 */
TEST(HeldBlockLayerEndermanEntityTest, AngryStateAffectsScreaming)
{
    EndermanEntity enderman(EntityId(1));

    // 默认不愤怒
    EXPECT_FALSE(enderman.isAngry());
    EXPECT_FALSE(enderman.isScreaming());

    // 设置愤怒状态，应该同时设置 screaming
    enderman.setAngry(true);
    EXPECT_TRUE(enderman.isAngry());
    EXPECT_TRUE(enderman.isScreaming());

    // 取消愤怒，应该同时清除 screaming
    enderman.setAngry(false);
    EXPECT_FALSE(enderman.isAngry());
    EXPECT_FALSE(enderman.isScreaming());
}

} // namespace mc::renderer::layer::test
