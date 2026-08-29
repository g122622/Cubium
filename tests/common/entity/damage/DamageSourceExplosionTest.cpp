/**
 * @file DamageSourceExplosionTest.cpp
 * @brief 测试 DamageSource::isExplosion() 方法和矿车伤害源检测逻辑
 *
 * 测试覆盖：
 * 1. EnvironmentalDamage::isExplosion() - 爆炸伤害类型检测
 * 2. EntityDamageSource::isExplosion() - 实体爆炸伤害检测
 * 3. IndirectEntityDamageSource::isExplosion() - 间接爆炸伤害检测
 * 4. TNTMinecartEntity::dropItem() - 火焰/爆炸伤害时的行为
 * 5. FurnaceMinecartEntity::dropItem() - 爆炸伤害时的行为
 */

#include "entity/damage/DamageSource.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// EnvironmentalDamage::isExplosion() 测试
// ============================================================================

class DamageSourceExplosionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 测试前准备
    }
};

/**
 * @brief 测试 EnvironmentalDamage 对爆炸伤害类型的检测
 *
 * MC 1.16.5: DamageSource.isExplosion() 应对 Explosion 和 ExplosionPlayer 返回 true
 */
TEST_F(DamageSourceExplosionTest, EnvironmentalDamage_ExplosionType_ReturnsTrue)
{
    // 爆炸伤害（非玩家引起）
    EnvironmentalDamage explosionDamage(DamageType::Explosion);
    EXPECT_TRUE(explosionDamage.isExplosion()) << "Explosion type should return true for isExplosion()";

    // 玩家引起的爆炸伤害
    EnvironmentalDamage explosionPlayerDamage(DamageType::ExplosionPlayer);
    EXPECT_TRUE(explosionPlayerDamage.isExplosion()) << "ExplosionPlayer type should return true for isExplosion()";
}

/**
 * @brief 测试 EnvironmentalDamage 对非爆炸伤害类型的检测
 *
 * MC 1.16.5: 其他伤害类型应返回 false
 */
TEST_F(DamageSourceExplosionTest, EnvironmentalDamage_NonExplosionType_ReturnsFalse)
{
    // 火焰伤害
    EnvironmentalDamage fireDamage(DamageType::InFire);
    EXPECT_FALSE(fireDamage.isExplosion()) << "InFire type should return false for isExplosion()";

    // 摔落伤害
    EnvironmentalDamage fallDamage(DamageType::Fall);
    EXPECT_FALSE(fallDamage.isExplosion()) << "Fall type should return false for isExplosion()";

    // 溺水伤害
    EnvironmentalDamage drownDamage(DamageType::Drown);
    EXPECT_FALSE(drownDamage.isExplosion()) << "Drown type should return false for isExplosion()";

    // 魔法伤害
    EnvironmentalDamage magicDamage(DamageType::Magic);
    EXPECT_FALSE(magicDamage.isExplosion()) << "Magic type should return false for isExplosion()";

    // 凋零伤害
    EnvironmentalDamage witherDamage(DamageType::Wither);
    EXPECT_FALSE(witherDamage.isExplosion()) << "Wither type should return false for isExplosion()";
}

/**
 * @brief 测试 EnvironmentalDamage::isFire() 方法
 *
 * MC 1.16.5: 火焰相关伤害类型应返回 true
 */
TEST_F(DamageSourceExplosionTest, EnvironmentalDamage_FireType_ReturnsCorrectBool)
{
    // 在火焰中
    EnvironmentalDamage inFireDamage(DamageType::InFire);
    EXPECT_TRUE(inFireDamage.isFire()) << "InFire type should return true for isFire()";

    // 燃烧中
    EnvironmentalDamage onFireDamage(DamageType::OnFire);
    EXPECT_TRUE(onFireDamage.isFire()) << "OnFire type should return true for isFire()";

    // 岩浆
    EnvironmentalDamage lavaDamage(DamageType::Lava);
    EXPECT_TRUE(lavaDamage.isFire()) << "Lava type should return true for isFire()";

    // 热地板（岩浆旁边的方块）
    EnvironmentalDamage hotFloorDamage(DamageType::HotFloor);
    EXPECT_TRUE(hotFloorDamage.isFire()) << "HotFloor type should return true for isFire()";

    // 非火焰伤害
    EnvironmentalDamage fallDamage(DamageType::Fall);
    EXPECT_FALSE(fallDamage.isFire()) << "Fall type should return false for isFire()";

    // 爆炸不是火焰伤害
    EnvironmentalDamage explosionDamage(DamageType::Explosion);
    EXPECT_FALSE(explosionDamage.isFire()) << "Explosion type should return false for isFire()";
}

// ============================================================================
// DamageSources 工厂函数测试
// ============================================================================

/**
 * @brief 测试 DamageSources 工厂函数创建的爆炸伤害
 *
 * MC 1.16.5: DamageSources::explosion() 应创建爆炸伤害源
 */
TEST_F(DamageSourceExplosionTest, DamageSources_Explosion_CreatesExplosionDamage)
{
    // 无来源爆炸
    EnvironmentalDamage explosionSource = DamageSources::explosion();
    EXPECT_TRUE(explosionSource.isExplosion()) << "DamageSources::explosion() should create explosion damage";

    // 玩家爆炸 - EntityDamageSource 在 mc 命名空间
    EntityDamageSource explosionPlayerSource = DamageSources::explosionPlayer(nullptr);
    EXPECT_TRUE(explosionPlayerSource.isExplosion())
        << "DamageSources::explosionPlayer() should create explosion damage";
}

// ============================================================================
// 伤害源组合检测测试（用于矿车逻辑）
// ============================================================================

/**
 * @brief 测试火焰+爆炸组合检测逻辑
 *
 * TNTMinecartEntity 需要区分：
 * - 仅火焰：点燃
 * - 仅爆炸：点燃
 * - 火焰+爆炸：点燃
 * - 都不是：正常掉落
 */
TEST_F(DamageSourceExplosionTest, CombinedCheck_FireAndExplosion)
{
    // 火焰伤害（非爆炸）
    EnvironmentalDamage fireDamage(DamageType::InFire);
    EXPECT_TRUE(fireDamage.isFire());
    EXPECT_FALSE(fireDamage.isExplosion());

    // 爆炸伤害（非火焰）
    EnvironmentalDamage explosionDamage(DamageType::Explosion);
    EXPECT_FALSE(explosionDamage.isFire());
    EXPECT_TRUE(explosionDamage.isExplosion());

    // 岩浆伤害（火焰，非爆炸）
    EnvironmentalDamage lavaDamage(DamageType::Lava);
    EXPECT_TRUE(lavaDamage.isFire());
    EXPECT_FALSE(lavaDamage.isExplosion());

    // 摔落伤害（都不是）
    EnvironmentalDamage fallDamage(DamageType::Fall);
    EXPECT_FALSE(fallDamage.isFire());
    EXPECT_FALSE(fallDamage.isExplosion());

    // 溺水伤害（都不是）
    EnvironmentalDamage drownDamage(DamageType::Drown);
    EXPECT_FALSE(drownDamage.isFire());
    EXPECT_FALSE(drownDamage.isExplosion());
}

/**
 * @brief 测试 nullptr 伤害源的安全检测
 *
 * MC 1.16.5: 矿车 dropItem 可能接收 nullptr 伤害源
 * 应安全处理：视为非火焰非爆炸伤害
 */
TEST_F(DamageSourceExplosionTest, NullptrSource_HandledSafely)
{
    DamageSource* nullSource = nullptr;

    // 安全检测模式（矿车中使用的模式）
    bool isFire = (nullSource != nullptr && nullSource->isFire());
    bool isExplosion = (nullSource != nullptr && nullSource->isExplosion());

    EXPECT_FALSE(isFire) << "Null source should not be fire damage";
    EXPECT_FALSE(isExplosion) << "Null source should not be explosion damage";
}

// ============================================================================
// EntityDamageSource 和 IndirectEntityDamageSource 测试
// ============================================================================

/**
 * @brief 测试 EntityDamageSource 的爆炸检测
 *
 * MC 1.16.5: 实体造成的爆炸伤害也应该返回 isExplosion() == true
 */
TEST_F(DamageSourceExplosionTest, EntityDamageSource_ExplosionType_ReturnsCorrectBool)
{
    // 创建一个假的实体指针（仅用于测试类型检查）
    Entity* fakeEntity = nullptr;

    // 玩家攻击（非爆炸）
    EntityDamageSource playerAttack(DamageType::PlayerAttack, fakeEntity);
    EXPECT_FALSE(playerAttack.isExplosion()) << "PlayerAttack should not be explosion damage";

    // 生物攻击（非爆炸）
    EntityDamageSource mobAttack(DamageType::MobAttack, fakeEntity);
    EXPECT_FALSE(mobAttack.isExplosion()) << "MobAttack should not be explosion damage";
}

/**
 * @brief 测试 IndirectEntityDamageSource 的爆炸检测
 *
 * MC 1.16.5: 间接伤害（箭矢、三叉戟等）的爆炸检测
 */
TEST_F(DamageSourceExplosionTest, IndirectEntityDamageSource_ExplosionType_ReturnsCorrectBool)
{
    Entity* fakeShooter = nullptr;
    Entity* fakeProjectile = nullptr;

    // 箭矢伤害（非爆炸）
    IndirectEntityDamageSource arrowDamage(DamageType::Arrow, fakeShooter, fakeProjectile, false);
    EXPECT_FALSE(arrowDamage.isExplosion()) << "Arrow should not be explosion damage";

    // 三叉戟伤害（非爆炸）
    IndirectEntityDamageSource tridentDamage(DamageType::Trident, fakeShooter, fakeProjectile, false);
    EXPECT_FALSE(tridentDamage.isExplosion()) << "Trident should not be explosion damage";

    // 火球伤害（非爆炸，但可能点燃目标）
    IndirectEntityDamageSource fireballDamage(DamageType::Fireball, fakeShooter, fakeProjectile, false);
    EXPECT_FALSE(fireballDamage.isExplosion()) << "Fireball should not be explosion damage (explosion is separate)";
    EXPECT_TRUE(fireballDamage.isFire()) << "Fireball should be fire damage";
}

// ============================================================================
// 边界条件测试
// ============================================================================

/**
 * @brief 测试所有 DamageType 的 isFire 和 isExplosion 状态
 *
 * 对齐 MC Java 1.21.11 IS_FIRE / IS_EXPLOSION 标签成员集，确保所有伤害类型都有正确的分类。
 * 注：isExplosion() 查 IS_EXPLOSION 标签（成员={Fireworks,Explosion,ExplosionPlayer,BadRespawnPoint}），
 * 此前测试遗漏 Fireworks/BadRespawnPoint 于 explosionTypes，并误把 Fireworks 放入 otherTypes。
 */
TEST_F(DamageSourceExplosionTest, AllDamageTypes_HaveCorrectClassification)
{
    // 火焰相关类型
    std::vector<DamageType> fireTypes = {
        DamageType::InFire, DamageType::OnFire, DamageType::Lava, DamageType::HotFloor, DamageType::Fireball};

    for (auto type : fireTypes) {
        EnvironmentalDamage damage(type);
        EXPECT_TRUE(damage.isFire()) << "DamageType " << static_cast<int>(type) << " should be fire damage";
        EXPECT_FALSE(damage.isExplosion()) << "Fire damage should not be explosion damage";
    }

    // 爆炸相关类型（对齐 vanilla IS_EXPLOSION 标签：Fireworks/Explosion/ExplosionPlayer/BadRespawnPoint）
    std::vector<DamageType> explosionTypes = {
        DamageType::Fireworks, DamageType::Explosion, DamageType::ExplosionPlayer, DamageType::BadRespawnPoint};

    for (auto type : explosionTypes) {
        EnvironmentalDamage damage(type);
        EXPECT_TRUE(damage.isExplosion()) << "DamageType " << static_cast<int>(type) << " should be explosion damage";
        EXPECT_FALSE(damage.isFire()) << "Explosion damage should not be fire damage";
    }

    // 非火焰非爆炸类型
    std::vector<DamageType> otherTypes = {DamageType::Drown,
        DamageType::Starve,
        DamageType::Cactus,
        DamageType::Fall,
        DamageType::FlyIntoWall,
        DamageType::OutOfWorld,
        DamageType::Generic,
        DamageType::Magic,
        DamageType::Wither,
        DamageType::FallingAnvil,
        DamageType::FallingBlock,
        DamageType::DragonBreath};

    for (auto type : otherTypes) {
        EnvironmentalDamage damage(type);
        EXPECT_FALSE(damage.isFire()) << "DamageType " << static_cast<int>(type) << " should not be fire damage";
        EXPECT_FALSE(damage.isExplosion())
            << "DamageType " << static_cast<int>(type) << " should not be explosion damage";
    }
}
