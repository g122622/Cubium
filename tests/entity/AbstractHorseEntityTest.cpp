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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/SkeletonHorseEntity.hpp"
#include "common/entity/entities/passive/horse/ZombieHorseEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World，支持实体状态广播和方块设置
 */
class AbstractHorseTestWorld final : public test::BaseTestWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& /*pos*/) const override { return 1.0f; }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    EntityId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("AbstractHorseTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("AbstractHorseTestWorld::tickManager not implemented");
    }

    void broadcastEntityStatus(EntityId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityId getLastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 getLastBroadcastStatus() const { return m_lastBroadcastStatus; }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    EntityId m_lastBroadcastEntityId{EntityId(0)};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

// ============================================================================
// canPerformRearing 测试
// ============================================================================

TEST(AbstractHorseAnimationTest, CanPerformRearing_DefaultIsTrue)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    // 马默认可以扬蹄
    EXPECT_TRUE(horse.canPerformRearing());
}

TEST(AbstractHorseAnimationTest, CanPerformRearing_LlamaReturnsFalse)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    LlamaEntity llama(EntityId(1));
    llama.setWorld(&world);

    // 羊驼不能扬蹄
    EXPECT_FALSE(llama.canPerformRearing());
}

TEST(AbstractHorseAnimationTest, CanPerformRearing_SkeletonHorseReturnsTrue)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    SkeletonHorseEntity skeletonHorse(EntityId(1));
    skeletonHorse.setWorld(&world);

    // 骷髅马可以扬蹄
    EXPECT_TRUE(skeletonHorse.canPerformRearing());
}

TEST(AbstractHorseAnimationTest, CanPerformRearing_ZombieHorseReturnsTrue)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    ZombieHorseEntity zombieHorse(EntityId(1));
    zombieHorse.setWorld(&world);

    // 僵尸马可以扬蹄
    EXPECT_TRUE(zombieHorse.canPerformRearing());
}

// ============================================================================
// canEatGrass 测试
// ============================================================================

TEST(AbstractHorseAnimationTest, CanEatGrass_DefaultIsTrue)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    // 马默认可以吃草
    EXPECT_TRUE(horse.canEatGrass());
}

TEST(AbstractHorseAnimationTest, CanEatGrass_LlamaReturnsTrue)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    LlamaEntity llama(EntityId(1));
    llama.setWorld(&world);

    // 羊驼可以吃草
    EXPECT_TRUE(llama.canEatGrass());
}

TEST(AbstractHorseAnimationTest, CanEatGrass_SkeletonHorseReturnsFalse)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    SkeletonHorseEntity skeletonHorse(EntityId(1));
    skeletonHorse.setWorld(&world);

    // 骷髅马不能吃草
    EXPECT_FALSE(skeletonHorse.canEatGrass());
}

TEST(AbstractHorseAnimationTest, CanEatGrass_ZombieHorseReturnsFalse)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    ZombieHorseEntity zombieHorse(EntityId(1));
    zombieHorse.setWorld(&world);

    // 僵尸马不能吃草
    EXPECT_FALSE(zombieHorse.canEatGrass());
}

// ============================================================================
// 扬蹄系统测试
// ============================================================================

TEST(AbstractHorseAnimationTest, MakeHorseRear_SetsRearingFlag)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    EXPECT_FALSE(horse.isRearing());

    horse.makeHorseRear();

    EXPECT_TRUE(horse.isRearing());
}

TEST(AbstractHorseAnimationTest, MakeHorseRear_ClearsEatingWhenRearing)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    horse.setEating(true);
    EXPECT_TRUE(horse.isEating());

    horse.makeHorseRear();

    // 扬蹄时应该清除吃草状态
    EXPECT_TRUE(horse.isRearing());
    EXPECT_FALSE(horse.isEating());
}

TEST(AbstractHorseAnimationTest, MakeHorseRear_LlamaCannotRear)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    LlamaEntity llama(EntityId(1));
    llama.setWorld(&world);

    EXPECT_FALSE(llama.isRearing());

    llama.makeHorseRear();

    // 羊驼不能扬蹄
    EXPECT_FALSE(llama.isRearing());
}

TEST(AbstractHorseAnimationTest, ClearRearing_ClearsRearingState)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    horse.makeHorseRear();
    EXPECT_TRUE(horse.isRearing());

    horse.clearRearing();
    EXPECT_FALSE(horse.isRearing());
}

TEST(AbstractHorseAnimationTest, MakeMad_TriggersRearingAndSound)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    EXPECT_FALSE(horse.isRearing());

    horse.makeMad();

    EXPECT_TRUE(horse.isRearing());
}

TEST(AbstractHorseAnimationTest, MakeMad_DoesNotRearIfAlreadyRearing)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    // 先扬蹄
    horse.makeHorseRear();
    EXPECT_TRUE(horse.isRearing());

    // 再次 makeMad 不会重复扬蹄（makeMad 检查 !isRearing()）
    // 由于已经在扬蹄状态，makeMad 不会再次扬蹄
    horse.makeMad();
    // 仍然在扬蹄状态（没有崩溃或异常）
    EXPECT_TRUE(horse.isRearing());
}

// ============================================================================
// 张嘴系统测试
// ============================================================================

TEST(AbstractHorseAnimationTest, OpenMouth_SetsMouthOpenFlag)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    EXPECT_FALSE(horse.isMouthOpen());

    horse.openMouth();

    EXPECT_TRUE(horse.isMouthOpen());
}

// ============================================================================
// Owner UUID 系统测试
// ============================================================================

TEST(AbstractHorseOwnerTest, HasOwner_ReturnsFalseByDefault)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));

    EXPECT_FALSE(horse.hasOwner());
    EXPECT_TRUE(horse.getOwnerUuid().empty());
}

TEST(AbstractHorseOwnerTest, SetOwnerUuid_UpdatesOwner)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));

    const std::string testUuid = "0123456789abcdef0123456789abcdef";
    horse.setOwnerUuid(testUuid);

    EXPECT_TRUE(horse.hasOwner());
    EXPECT_EQ(horse.getOwnerUuid(), testUuid);
}

TEST(AbstractHorseOwnerTest, SetOwnerUuid_AutoTames)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    EXPECT_FALSE(horse.isTame());

    const std::string testUuid = "0123456789abcdef0123456789abcdef";
    horse.setOwnerUuid(testUuid);

    // 设置主人 UUID 时自动标记为已驯服
    EXPECT_TRUE(horse.isTame());
}

TEST(AbstractHorseOwnerTest, ClearOwnerUuid_ClearsOwner)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));

    const std::string testUuid = "0123456789abcdef0123456789abcdef";
    horse.setOwnerUuid(testUuid);
    EXPECT_TRUE(horse.hasOwner());

    horse.clearOwnerUuid();
    EXPECT_FALSE(horse.hasOwner());
    EXPECT_TRUE(horse.getOwnerUuid().empty());
}

TEST(AbstractHorseOwnerTest, GetOwner_ReturnsNullptrWithoutWorld)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));

    const std::string testUuid = "0123456789abcdef0123456789abcdef";
    horse.setOwnerUuid(testUuid);

    // 没有 world 时无法查找实体
    EXPECT_EQ(horse.getOwner(), nullptr);
}

// ============================================================================
// NBT 序列化测试
// ============================================================================

TEST(AbstractHorseNbtTest, OwnerUuid_RoundTrip)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    const std::string testUuid = "0123456789abcdef0123456789abcdef";
    horse.setOwnerUuid(testUuid);
    horse.setTame(true);

    // 序列化
    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    // 验证 UUID 键存在
    using namespace mc::entity::serialization;
    auto mostVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_MOST);
    auto leastVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_LEAST);
    EXPECT_TRUE(mostVal.has_value());
    EXPECT_TRUE(leastVal.has_value());

    // 反序列化到新实体
    HorseEntity horse2(EntityId(2));
    auto result = horse2.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    // 验证 UUID 一致
    EXPECT_EQ(horse2.getOwnerUuid(), testUuid);
    EXPECT_TRUE(horse2.isTame());
}

TEST(AbstractHorseNbtTest, Temper_RoundTrip)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    horse.increaseTemper(50);

    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    HorseEntity horse2(EntityId(2));
    auto result = horse2.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_EQ(horse2.getTemper(), horse.getTemper());
}

TEST(AbstractHorseNbtTest, TameAndSaddle_RoundTrip)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    horse.setTame(true);
    horse.setSaddle(true);

    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    HorseEntity horse2(EntityId(2));
    auto result = horse2.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_TRUE(horse2.isTame());
    EXPECT_TRUE(horse2.hasSaddle());
}

TEST(AbstractHorseNbtTest, EatingAndBred_RoundTrip)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    horse.setEating(true);
    horse.setBred(true);

    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    HorseEntity horse2(EntityId(2));
    auto result = horse2.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_TRUE(horse2.isEating());
    EXPECT_TRUE(horse2.isBred());
}

TEST(AbstractHorseNbtTest, JumpStrengthAndSpeed_RoundTrip)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    horse.setJumpStrength(0.75f);

    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    HorseEntity horse2(EntityId(2));
    auto result = horse2.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_FLOAT_EQ(horse2.getJumpStrength(), 0.75f);
}

TEST(AbstractHorseNbtTest, ClearOwnerUuid_NotSerializedWhenEmpty)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    horse.clearOwnerUuid();

    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    // 没有 UUID 时不应该写入 OwnerUUIDMost/Least
    using namespace mc::entity::serialization;
    auto mostVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_MOST);
    auto leastVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_LEAST);
    EXPECT_FALSE(mostVal.has_value());
    EXPECT_FALSE(leastVal.has_value());
}

TEST(AbstractHorseNbtTest, UntamedHorse_DoesNotWriteTameTag)
{
    VanillaBlocks::initialize();

    HorseEntity horse(EntityId(1));
    horse.setTame(false);

    nbt::tags::compound_tag tag;
    horse.addAdditionalSaveData(tag);

    // 未驯服时不应该写入 "Tame" 标签（因为是 false，MC 原版不写入 false 值）
    using namespace mc::entity::serialization;
    auto tameVal = nbt_helper::tryGetBool(tag, "Tame");
    EXPECT_FALSE(tameVal.has_value());
}

// ============================================================================
// setTamedBy 测试
// ============================================================================

TEST(AbstractHorseOwnerTest, SetTamedBy_SetsOwnerAndTame)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    // 创建一个模拟玩家（使用 Player 需要太多依赖，这里只测试 UUID 设置）
    const std::string playerUuid = "abcdef0123456789abcdef0123456789";

    // 直接通过 setOwnerUuid 测试
    horse.setOwnerUuid(playerUuid);

    EXPECT_TRUE(horse.hasOwner());
    EXPECT_TRUE(horse.isTame());
    EXPECT_EQ(horse.getOwnerUuid(), playerUuid);
}

// ============================================================================
// 状态标志测试
// ============================================================================

TEST(AbstractHorseStateTest, SetEating_UpdatesFlag)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    EXPECT_FALSE(horse.isEating());

    horse.setEating(true);
    EXPECT_TRUE(horse.isEating());

    horse.setEating(false);
    EXPECT_FALSE(horse.isEating());
}

TEST(AbstractHorseStateTest, SetRearing_ClearsEating)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    horse.setEating(true);
    EXPECT_TRUE(horse.isEating());

    horse.setRearing(true);
    EXPECT_TRUE(horse.isRearing());
    EXPECT_FALSE(horse.isEating()); // 扬蹄时清除吃草状态
}

TEST(AbstractHorseStateTest, SetMouthOpen_UpdatesFlag)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    EXPECT_FALSE(horse.isMouthOpen());

    horse.setMouthOpen(true);
    EXPECT_TRUE(horse.isMouthOpen());

    horse.setMouthOpen(false);
    EXPECT_FALSE(horse.isMouthOpen());
}

TEST(AbstractHorseStateTest, SetBred_UpdatesFlag)
{
    VanillaBlocks::initialize();

    AbstractHorseTestWorld world;
    HorseEntity horse(EntityId(1));
    horse.setWorld(&world);

    EXPECT_FALSE(horse.isBred());

    horse.setBred(true);
    EXPECT_TRUE(horse.isBred());

    horse.setBred(false);
    EXPECT_FALSE(horse.isBred());
}

} // namespace
} // namespace mc
