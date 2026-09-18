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

// 测试 fixture 类 LavaFluidIgnitionTest 放 mc 命名空间（非匿名），使 LavaFluid.hpp 内
// friend class ::mc::LavaFluidIgnitionTest 声明能跨翻译单元匹配，访问私有 _isBlockFlammable。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/fluid/fluids/LavaFluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <unordered_map>

using namespace mc;
using namespace mc::block_registry;
using namespace mc::fluid;

namespace {

// 最小测试世界：仅覆写 getBlockState/setBlockState，用 map 存方块。其余 IWorld 方法继承
// mc::test::BaseTestWorld 的默认实现（_isBlockFlammable 只用 getBlockState 与高度边界检查）。
// 参考 FireBlockTest.cpp 的 FireSpreadTestWorld 模式：用 BlockPos 作 key，未设置位置返回
// nullptr（_isBlockFlammable 对 nullptr 返回 false），setBlockState 规范化为注册表指针。
class LavaIgnitionTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            return true;
        }
        // 规范化为注册表持有的规范状态指针，避免存调用方栈帧指针
        const BlockState* canonical = BlockRegistry::instance().getBlockState(state->stateId());
        m_blocks[pos] = (canonical != nullptr) ? canonical : state;
        return true;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

} // namespace

namespace mc {

// 测试 fixture：经 friend 访问 LavaFluid::_isBlockFlammable，验证偏离 #8 修复
// （isIgnitedByLava 替代 material().isFlammable()）。
class LavaFluidIgnitionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        // FluidRegistry 在 BaseBlocks 注册时初始化（VanillaBlocks::initialize 触发）
    }

    void SetUp() override
    {
        ASSERT_NE(VanillaBlocks::AIR, nullptr);
        ASSERT_NE(PaleGardenBlocks::PALE_MOSS_BLOCK, nullptr);
        ASSERT_NE(BuildingBlocks::TNT, nullptr);
        ASSERT_NE(RedstoneBlocks::OAK_BUTTON, nullptr);
        ASSERT_NE(VanillaBlocks::STONE, nullptr);
        ASSERT_NE(SignBannerBlocks::CRIMSON_SIGN, nullptr);
        ASSERT_NE(VanillaBlocks::OAK_PLANKS, nullptr);
    }

    // 经 friend 授权访问 LavaFluid::_isBlockFlammable
    static bool callIsBlockFlammable(const LavaFluid& lava, IWorld& world, const BlockPos& pos)
    {
        return lava._isBlockFlammable(world, pos);
    }

    static const LavaFluid* getLavaFluid()
    {
        Fluid* fluid = FluidRegistry::instance().getFluid(FluidRegistry::LAVA_ID);
        return dynamic_cast<const LavaFluid*>(fluid);
    }

    LavaIgnitionTestWorld m_world;
};

// ============================================================================
// LavaFluid::_isBlockFlammable 测试（偏离 #8 修复验证）
// ============================================================================
//
// 对齐 vanilla LavaFluid.isFlammable（LavaFluid.java:134-136）：岩浆能否点燃方块由
// 方块的 ignitedByLava() 属性决定。LavaFluid::_isBlockFlammable 已改为查 isIgnitedByLava()。
// 此前用 material().isFlammable()，对 pale_moss_block（Material::MOSS 不可燃但 vanilla
// ignitedByLava=true）漏判，对木按钮（Material::WOOD 可燃但 vanilla ignitedByLava=false）误判。

TEST_F(LavaFluidIgnitionTest, IsBlockFlammable_PaleMossBlock_True)
{
    // pale_moss_block：Material::MOSS 不可燃，但 vanilla ignitedByLava=true。
    // 修复前 material().isFlammable()=false（漏判）；修复后 isIgnitedByLava()=true。
    const LavaFluid* lava = getLavaFluid();
    ASSERT_NE(lava, nullptr);

    BlockPos pos(5, 64, 5);
    m_world.setBlockAt(pos, &PaleGardenBlocks::PALE_MOSS_BLOCK->defaultState());
    EXPECT_TRUE(callIsBlockFlammable(*lava, m_world, pos));
}

TEST_F(LavaFluidIgnitionTest, IsBlockFlammable_OakPlanks_True)
{
    // 木板：Material::WOOD 可燃 + vanilla ignitedByLava=true，两种判定均 true（碰巧等价）。
    const LavaFluid* lava = getLavaFluid();
    ASSERT_NE(lava, nullptr);

    BlockPos pos(5, 64, 5);
    m_world.setBlockAt(pos, &VanillaBlocks::OAK_PLANKS->defaultState());
    EXPECT_TRUE(callIsBlockFlammable(*lava, m_world, pos));
}

TEST_F(LavaFluidIgnitionTest, IsBlockFlammable_TNT_True)
{
    // TNT：Material::TNT 可燃 + vanilla ignitedByLava=true。
    const LavaFluid* lava = getLavaFluid();
    ASSERT_NE(lava, nullptr);

    BlockPos pos(5, 64, 5);
    m_world.setBlockAt(pos, &BuildingBlocks::TNT->defaultState());
    EXPECT_TRUE(callIsBlockFlammable(*lava, m_world, pos));
}

TEST_F(LavaFluidIgnitionTest, IsBlockFlammable_WoodenButton_False)
{
    // 木按钮：Material::WOOD 可燃，但 vanilla ignitedByLava=false。
    // 修复前 material().isFlammable()=true（误判）；修复后 isIgnitedByLava()=false。
    const LavaFluid* lava = getLavaFluid();
    ASSERT_NE(lava, nullptr);

    BlockPos pos(5, 64, 5);
    m_world.setBlockAt(pos, &RedstoneBlocks::OAK_BUTTON->defaultState());
    EXPECT_FALSE(callIsBlockFlammable(*lava, m_world, pos));
}

TEST_F(LavaFluidIgnitionTest, IsBlockFlammable_Stone_False)
{
    // 石头：Material::ROCK 不可燃 + vanilla ignitedByLava=false，两种判定均 false。
    const LavaFluid* lava = getLavaFluid();
    ASSERT_NE(lava, nullptr);

    BlockPos pos(5, 64, 5);
    m_world.setBlockAt(pos, &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(callIsBlockFlammable(*lava, m_world, pos));
}

TEST_F(LavaFluidIgnitionTest, IsBlockFlammable_NetherWoodSign_False)
{
    // 下界木告示牌：vanilla 下界木不可燃，ignitedByLava=false。
    const LavaFluid* lava = getLavaFluid();
    ASSERT_NE(lava, nullptr);

    BlockPos pos(5, 64, 5);
    m_world.setBlockAt(pos, &SignBannerBlocks::CRIMSON_SIGN->defaultState());
    EXPECT_FALSE(callIsBlockFlammable(*lava, m_world, pos));
}

} // namespace mc
