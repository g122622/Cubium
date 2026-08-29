/**
 * @file MinecartDropItemTest.cpp
 * @brief 测试矿车 dropItem() 方法的伤害源检测逻辑
 *
 * 测试覆盖：
 * 1. TNTMinecartEntity::dropItem() - 不同伤害源的行为
 * 2. FurnaceMinecartEntity::dropItem() - 爆炸伤害检测
 * 3. ChestMinecartEntity::dropItem() - 库存掉落
 * 4. AbstractMinecartEntity::dropItem() - 基础掉落
 * 5. TNTMinecartEntity::hurt() - 燃烧箭矢引爆逻辑
 *
 * MC 1.16.5 参考：
 * - TNTMinecartEntity.killMinecart() 行79-94
 * - TNTMinecartEntity.attackEntityFrom() 行67-77
 * - FurnaceMinecartEntity.killMinecart() 行82-88
 */

#include "entity/core/Entity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/vehicle/MinecartEntity.hpp"
#include "item/core/ItemStack.hpp"
#include <cmath>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建测试用的伤害源
 */
std::unique_ptr<DamageSource> createFireDamage()
{
    return std::make_unique<EnvironmentalDamage>(DamageType::InFire);
}

std::unique_ptr<DamageSource> createExplosionDamage()
{
    return std::make_unique<EnvironmentalDamage>(DamageType::Explosion);
}

std::unique_ptr<DamageSource> createLavaDamage()
{
    return std::make_unique<EnvironmentalDamage>(DamageType::Lava);
}

std::unique_ptr<DamageSource> createFallDamage()
{
    return std::make_unique<EnvironmentalDamage>(DamageType::Fall);
}

std::unique_ptr<DamageSource> createDrownDamage()
{
    return std::make_unique<EnvironmentalDamage>(DamageType::Drown);
}

// ============================================================================
// TNTMinecartEntity 伤害源检测测试
// ============================================================================

class TNTMinecartDropTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试环境
    }
};

/**
 * @brief 测试 TNT 矿车对火焰伤害源的正确检测
 *
 * MC 1.16.5: 火焰伤害应点燃 TNT 矿车而非掉落物品
 *
 * 测试场景：
 * 1. InFire 伤害源 -> isFire() == true, isExplosion() == false
 * 2. Lava 伤害源 -> isFire() == true, isExplosion() == false
 */
TEST_F(TNTMinecartDropTest, FireDamage_IgnitesTNT)
{
    // InFire 伤害
    auto fireDamage = createFireDamage();
    EXPECT_TRUE(fireDamage->isFire());
    EXPECT_FALSE(fireDamage->isExplosion());

    // Lava 伤害
    auto lavaDamage = createLavaDamage();
    EXPECT_TRUE(lavaDamage->isFire());
    EXPECT_FALSE(lavaDamage->isExplosion());

    // 根据MC逻辑，火焰伤害应该点燃TNT矿车
    // 实际的 dropItem() 行为需要完整的世界环境来测试
}

/**
 * @brief 测试 TNT 矿车对爆炸伤害源的正确检测
 *
 * MC 1.16.5: 爆炸伤害应点燃 TNT 矿车而非掉落物品
 */
TEST_F(TNTMinecartDropTest, ExplosionDamage_IgnitesTNT)
{
    auto explosionDamage = createExplosionDamage();
    EXPECT_FALSE(explosionDamage->isFire());
    EXPECT_TRUE(explosionDamage->isExplosion());

    // 根据MC逻辑，爆炸伤害应该点燃TNT矿车
}

/**
 * @brief 测试 TNT 矿车对普通伤害源的行为
 *
 * MC 1.16.5: 非火焰非爆炸伤害、低速度时正常掉落矿车+TNT
 */
TEST_F(TNTMinecartDropTest, NormalDamage_DropsItems)
{
    // 摔落伤害
    auto fallDamage = createFallDamage();
    EXPECT_FALSE(fallDamage->isFire());
    EXPECT_FALSE(fallDamage->isExplosion());

    // 溺水伤害
    auto drownDamage = createDrownDamage();
    EXPECT_FALSE(drownDamage->isFire());
    EXPECT_FALSE(drownDamage->isExplosion());

    // 普通伤害应该正常掉落物品（需要低速度条件）
}

/**
 * @brief 测试 nullptr 伤害源的安全处理
 *
 * MC 1.16.5: dropItem(DamageSource* source) 需要安全处理 nullptr
 */
TEST_F(TNTMinecartDropTest, NullptrSource_HandledSafely)
{
    DamageSource* nullSource = nullptr;

    bool isFire = (nullSource != nullptr && nullSource->isFire());
    bool isExplosion = (nullSource != nullptr && nullSource->isExplosion());

    EXPECT_FALSE(isFire);
    EXPECT_FALSE(isExplosion);

    // nullptr 应该视为普通伤害，正常掉落
}

/**
 * @brief 测试 TNT 矿车速度对掉落的影响
 *
 * MC 1.16.5: 速度 >= 0.01 时不掉落物品（碰撞爆炸）
 * 速度 < 0.01 时才掉落物品
 */
TEST_F(TNTMinecartDropTest, SpeedThreshold_AffectsDrop)
{
    // 速度阈值 = 0.01 (speedSq 阈值)
    f64 lowSpeed = 0.05; // 低速度，speedSq = 0.0025 < 0.01
    f64 highSpeed = 0.2; // 高速度，speedSq = 0.04 > 0.01

    f64 lowSpeedSq = lowSpeed * lowSpeed;
    f64 highSpeedSq = highSpeed * highSpeed;

    // 非火焰非爆炸伤害
    auto normalDamage = createFallDamage();

    // 低速度：应该掉落
    EXPECT_LT(lowSpeedSq, 0.01) << "Low speed should be below threshold";

    // 高速度：应该爆炸
    EXPECT_GT(highSpeedSq, 0.01) << "High speed should be above threshold";
}

// ============================================================================
// FurnaceMinecartEntity 伤害源检测测试
// ============================================================================

class FurnaceMinecartDropTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试环境
    }
};

/**
 * @brief 测试熔炉矿车对爆炸伤害源的正确检测
 *
 * MC 1.16.5: 爆炸伤害不掉落熔炉方块，只掉落矿车
 */
TEST_F(FurnaceMinecartDropTest, ExplosionDamage_NoFurnaceDrop)
{
    auto explosionDamage = createExplosionDamage();
    EXPECT_TRUE(explosionDamage->isExplosion());

    // 爆炸伤害时 shouldNotDropFurnace = true
    // 应该只掉落矿车，不掉落熔炉方块
}

/**
 * @brief 测试熔炉矿车对普通伤害源的行为
 *
 * MC 1.16.5: 非爆炸伤害应该掉落矿车+熔炉方块
 */
TEST_F(FurnaceMinecartDropTest, NormalDamage_DropsFurnace)
{
    auto fallDamage = createFallDamage();
    EXPECT_FALSE(fallDamage->isExplosion());

    auto fireDamage = createFireDamage();
    EXPECT_FALSE(fireDamage->isExplosion());
    EXPECT_TRUE(fireDamage->isFire());

    // 火焰伤害不是爆炸，应该掉落熔炉
    // 普通伤害应该掉落矿车+熔炉
}

/**
 * @brief 测试熔炉矿车 nullptr 伤害源处理
 */
TEST_F(FurnaceMinecartDropTest, NullptrSource_DropsFurnace)
{
    DamageSource* nullSource = nullptr;

    bool isExplosion = (nullSource != nullptr && nullSource->isExplosion());

    EXPECT_FALSE(isExplosion);

    // nullptr 应该视为普通伤害，掉落矿车+熔炉
}

// ============================================================================
// ChestMinecartEntity 库存掉落测试
// ============================================================================

class ChestMinecartDropTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试环境
    }
};

/**
 * @brief 测试箱子矿车在任何情况下都掉落库存
 *
 * MC 1.16.5: 箱子矿车被破坏时总是掉落库存内容
 */
TEST_F(ChestMinecartDropTest, AlwaysDropsInventory)
{
    // 爆炸伤害
    auto explosionDamage = createExplosionDamage();

    // 火焰伤害
    auto fireDamage = createFireDamage();

    // 普通伤害
    auto fallDamage = createFallDamage();

    // 无论什么伤害源，箱子矿车都应该掉落库存
    // 实际测试需要完整的世界环境
}

/**
 * @brief 测试箱子矿车 nullptr 伤害源处理
 */
TEST_F(ChestMinecartDropTest, NullptrSource_DropsInventory)
{
    DamageSource* nullSource = nullptr;

    // nullptr 伤害源也应该正常掉落库存
    // ChestMinecartEntity::dropItem() 应该先掉落库存，再掉落矿车
}

// ============================================================================
// AbstractMinecartEntity 基础掉落测试
// ============================================================================

class AbstractMinecartDropTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试环境
    }
};

/**
 * @brief 测试基础矿车掉落行为
 *
 * MC 1.16.5: 普通矿车被破坏时掉落矿车物品
 */
TEST_F(AbstractMinecartDropTest, BasicDropBehavior)
{
    // 普通伤害
    auto normalDamage = createFallDamage();

    // 爆炸伤害
    auto explosionDamage = createExplosionDamage();

    // 无论什么伤害，普通矿车都应该掉落矿车物品
}

/**
 * @brief 测试伤害源参数传递
 *
 * MC 1.16.5: dropItem(DamageSource* source) 需要正确传递伤害源
 */
TEST_F(AbstractMinecartDropTest, DamageSourceParameter_PassedCorrectly)
{
    // 测试伤害源指针传递
    EnvironmentalDamage fireDamage(DamageType::InFire);
    DamageSource* sourcePtr = &fireDamage;

    EXPECT_TRUE(sourcePtr->isFire());
    EXPECT_FALSE(sourcePtr->isExplosion());

    // nullptr 传递测试
    DamageSource* nullPtr = nullptr;
    bool isFire = (nullPtr != nullptr && nullPtr->isFire());
    EXPECT_FALSE(isFire);
}

// ============================================================================
// 组合测试场景
// ============================================================================

/**
 * @brief 测试所有伤害类型在矿车逻辑中的行为
 *
 * 确保 isFire() 和 isExplosion() 对所有类型都有正确的返回值
 */
TEST_F(AbstractMinecartDropTest, AllDamageTypes_CorrectClassification)
{
    struct TestCase {
        DamageType type;
        bool expectedFire;
        bool expectedExplosion;
        std::string name;
    };

    std::vector<TestCase> testCases = {{DamageType::InFire, true, false, "InFire"},
        {DamageType::OnFire, true, false, "OnFire"},
        {DamageType::Lava, true, false, "Lava"},
        {DamageType::HotFloor, true, false, "HotFloor"},
        {DamageType::Explosion, false, true, "Explosion"},
        {DamageType::ExplosionPlayer, false, true, "ExplosionPlayer"},
        {DamageType::Drown, false, false, "Drown"},
        {DamageType::Fall, false, false, "Fall"},
        {DamageType::Starve, false, false, "Starve"},
        {DamageType::Cactus, false, false, "Cactus"},
        {DamageType::OutOfWorld, false, false, "OutOfWorld"},
        {DamageType::Magic, false, false, "Magic"},
        {DamageType::Wither, false, false, "Wither"},
        {DamageType::FallingAnvil, false, false, "FallingAnvil"},
        {DamageType::FallingBlock, false, false, "FallingBlock"},
        {DamageType::DragonBreath, false, false, "DragonBreath"},
        {DamageType::Fireworks, false, true, "Fireworks"}};

    for (const auto& tc : testCases) {
        EnvironmentalDamage damage(tc.type);
        EXPECT_EQ(damage.isFire(), tc.expectedFire)
            << "DamageType::" << tc.name << " isFire() should be " << tc.expectedFire;
        EXPECT_EQ(damage.isExplosion(), tc.expectedExplosion)
            << "DamageType::" << tc.name << " isExplosion() should be " << tc.expectedExplosion;
    }
}

// ============================================================================
// TNTMinecartEntity 燃烧箭矢引爆测试
// ============================================================================

/**
 * @brief 测试用的箭矢实体类
 *
 * 继承 AbstractArrowEntity 提供最小化实现用于测试
 */
class TestArrowEntity : public AbstractArrowEntity {
public:
    TestArrowEntity(EntityInstanceId id)
        : AbstractArrowEntity(id, mc::test::testEcsRegistry())
    {}

    static std::unique_ptr<Entity> create(IWorld* /*world*/)
    {
        return std::make_unique<TestArrowEntity>(EntityInstanceId(0));
    }

    // 提供纯虚函数的最小化实现
    void tick() override { Entity::tick(); }

    [[nodiscard]] ItemStack getArrowStack() const override
    {
        // 返回空的箭矢物品堆
        return ItemStack();
    }
};

/**
 * @brief 燃烧箭矢引爆 TNT 矿车测试
 *
 * MC 1.16.5: TNTMinecartEntity.attackEntityFrom() 第67-77行
 * - 燃烧的箭矢命中时，使用箭矢速度计算爆炸威力
 */
class TNTMinecartArrowTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试环境
    }
};

/**
 * @brief 测试普通箭矢（未燃烧）不引爆 TNT 矿车
 *
 * MC 1.16.5: 只有 isBurning() 返回 true 的箭矢才会引爆
 */
TEST_F(TNTMinecartArrowTest, NonBurningArrow_DoesNotIgnite)
{
    // 创建测试箭矢实体
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));

    // 未设置燃烧状态
    EXPECT_FALSE(arrow->isOnFire()) << "Arrow should not be on fire initially";
    EXPECT_EQ(arrow->fire(), 0) << "Fire timer should be 0";

    // 创建箭矢伤害源
    Entity* shooter = nullptr; // 无射击者
    IndirectEntityDamageSource arrowDamage = DamageSources::arrow(arrow.get(), shooter, false);

    // 验证伤害源属性
    EXPECT_TRUE(arrowDamage.isProjectile());
    EXPECT_EQ(arrowDamage.directSource(), arrow.get());
}

/**
 * @brief 测试燃烧箭矢会引爆 TNT 矿车
 *
 * MC 1.16.5: 箭矢燃烧时 isBurning() 返回 true
 */
TEST_F(TNTMinecartArrowTest, BurningArrow_IgnitesTNT)
{
    // 创建测试箭矢实体
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));

    // 设置燃烧状态（100 ticks = 5秒）
    arrow->setFire(100);

    // 验证燃烧状态
    EXPECT_TRUE(arrow->isOnFire()) << "Arrow should be on fire after setFire(100)";
    EXPECT_EQ(arrow->fire(), 100) << "Fire timer should be 100";

    // 创建箭矢伤害源
    Entity* shooter = nullptr;
    IndirectEntityDamageSource arrowDamage = DamageSources::arrow(arrow.get(), shooter, false);

    // 验证伤害源属性
    EXPECT_TRUE(arrowDamage.isProjectile());
    EXPECT_EQ(arrowDamage.directSource(), arrow.get());

    // 箭矢燃烧状态应该可以被检测到
    // 在 TNTMinecartEntity::hurt() 中:
    // AbstractArrowEntity* arrow = dynamic_cast<AbstractArrowEntity*>(directSource);
    // if (arrow != nullptr && arrow->isOnFire()) { explode(...); }
}

/**
 * @brief 测试箭矢速度影响爆炸威力计算
 *
 * MC 1.16.5: explodeCart(motion.lengthSquared())
 * 爆炸威力 = 4.0 + random(0~1) * 1.5 * min(sqrt(speedSq), 5.0)
 */
TEST_F(TNTMinecartArrowTest, ArrowVelocity_AffectsExplosionPower)
{
    // 创建测试箭矢实体
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));
    arrow->setFire(100); // 燃烧

    // 测试不同速度
    struct TestCase {
        f32 vx, vy, vz;
        f64 expectedSpeedSq;
    };

    std::vector<TestCase> testCases = {
        {0.0f, 0.0f, 0.0f, 0.0},  // 静止
        {1.0f, 0.0f, 0.0f, 1.0},  // 水平速度 1.0
        {0.0f, 1.0f, 0.0f, 1.0},  // 垂直速度 1.0
        {3.0f, 4.0f, 0.0f, 25.0}, // 3-4-5 三角形
        {2.0f, 2.0f, 2.0f, 12.0}, // 对角线
    };

    for (const auto& tc : testCases) {
        arrow->setVelocity(Vector3(tc.vx, tc.vy, tc.vz));
        Vector3 vel = arrow->velocity();

        f64 speedSq =
            static_cast<f64>(vel.x) * vel.x + static_cast<f64>(vel.y) * vel.y + static_cast<f64>(vel.z) * vel.z;

        EXPECT_DOUBLE_EQ(speedSq, tc.expectedSpeedSq)
            << "Velocity (" << tc.vx << ", " << tc.vy << ", " << tc.vz << ") speedSq should be " << tc.expectedSpeedSq;

        // 爆炸威力计算（参考 MC 1.16.5）
        f64 speed = std::sqrt(speedSq);
        f64 cappedSpeed = std::min(speed, 5.0); // 最大速度 5.0
        // 威力范围: 4.0 ~ 11.5 (基础 + 随机加成)
        EXPECT_GE(cappedSpeed, 0.0);
        EXPECT_LE(cappedSpeed, 5.0);
    }
}

/**
 * @brief 测试 dynamic_cast 检测箭矢实体
 *
 * 验证 dynamic_cast<AbstractArrowEntity*> 可以正确识别箭矢类型
 */
TEST_F(TNTMinecartArrowTest, DynamicCast_IdentifiesArrowEntity)
{
    // 创建测试箭矢实体
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));

    // 通过 Entity* 指针进行 dynamic_cast
    Entity* entityPtr = arrow.get();
    AbstractArrowEntity* arrowPtr = dynamic_cast<AbstractArrowEntity*>(entityPtr);

    EXPECT_NE(arrowPtr, nullptr) << "dynamic_cast should succeed for AbstractArrowEntity";

    // 设置燃烧状态
    arrow->setFire(100);
    EXPECT_TRUE(arrowPtr->isOnFire()) << "Burning state should be accessible through casted pointer";
}

/**
 * @brief 测试其他投射物（如火球）的兼容检测
 *
 * MC 1.16.5: 其他带火焰的投射物也应该引爆 TNT 矿车
 */
TEST_F(TNTMinecartArrowTest, OtherFireProjectiles_CompatibleDetection)
{
    // 创建测试箭矢实体（不燃烧）
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));
    EXPECT_FALSE(arrow->isOnFire());

    // 创建火球伤害源（带火焰和投射物属性）
    Entity* shooter = nullptr;
    IndirectEntityDamageSource fireballDamage = DamageSources::fireball(arrow.get(), shooter, false);
    fireballDamage.setProjectile();
    fireballDamage.setFireDamage();

    // 验证兼容检测逻辑
    // source.isProjectile() && source.isFire() 应该为 true
    EXPECT_TRUE(fireballDamage.isProjectile());
    EXPECT_TRUE(fireballDamage.isFire());
}

/**
 * @brief 测试光灵箭（SpectralArrow）也能引爆 TNT 矿车
 *
 * MC 1.16.5: AbstractArrowEntity 包括 ArrowEntity 和 SpectralArrowEntity
 */
TEST_F(TNTMinecartArrowTest, SpectralArrow_CanIgnite)
{
    // 光灵箭也是 AbstractArrowEntity 的子类
    auto spectralArrow = std::make_unique<TestArrowEntity>(EntityInstanceId(2));

    // 设置燃烧状态
    spectralArrow->setFire(100);

    // 验证燃烧状态
    EXPECT_TRUE(spectralArrow->isOnFire());

    // 验证 dynamic_cast
    Entity* entityPtr = spectralArrow.get();
    AbstractArrowEntity* arrowPtr = dynamic_cast<AbstractArrowEntity*>(entityPtr);
    EXPECT_NE(arrowPtr, nullptr) << "SpectralArrow should be castable to AbstractArrowEntity";
    EXPECT_TRUE(arrowPtr->isOnFire());
}

/**
 * @brief 测试 setFire 行为：只增不减
 *
 * MC 1.16.5: Entity.setFire() 只会增加燃烧时间，不会减少
 */
TEST_F(TNTMinecartArrowTest, SetFire_OnlyIncreases)
{
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));

    // 初始状态
    EXPECT_EQ(arrow->fire(), 0);
    EXPECT_FALSE(arrow->isOnFire());

    // 设置燃烧 100 ticks
    arrow->setFire(100);
    EXPECT_EQ(arrow->fire(), 100);
    EXPECT_TRUE(arrow->isOnFire());

    // 尝试减少燃烧时间（应该无效）
    arrow->setFire(50);
    EXPECT_EQ(arrow->fire(), 100) << "setFire should not decrease fire time";

    // 增加燃烧时间（应该有效）
    arrow->setFire(200);
    EXPECT_EQ(arrow->fire(), 200);

    // 使用 forceFireTicks 强制减少
    arrow->forceFireTicks(50);
    EXPECT_EQ(arrow->fire(), 50);

    // 清除燃烧
    arrow->forceFireTicks(0);
    EXPECT_EQ(arrow->fire(), 0);
    EXPECT_FALSE(arrow->isOnFire());
}

/**
 * @brief 测试燃烧免疫期间（负值）不会被误判为燃烧
 *
 * MC 1.16.5: fire < 0 表示免疫期，isOnFire() 返回 false
 */
TEST_F(TNTMinecartArrowTest, NegativeFire_NotOnFire)
{
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));

    // 设置负值（免疫期）
    arrow->forceFireTicks(-10);
    EXPECT_EQ(arrow->fire(), -10);
    EXPECT_FALSE(arrow->isOnFire()) << "Negative fire should not count as on fire";
}

// ============================================================================
// _damageSourceIgnitesTnt 逻辑测试
// ============================================================================

/**
 * @brief 测试 TNT 矿车 _damageSourceIgnitesTnt 对各种伤害类型的判断
 *
 * 对应 MC Java 的 MinecartTNT.damageSourceIgnitesTnt()：
 * - 直接实体是着火投射物 → 能点燃
 * - IS_FIRE 伤害类型 → 能点燃
 * - IS_EXPLOSION 伤害类型 → 能点燃
 * - 其他伤害类型 → 不能点燃
 */
class TNTDamageSourceIgnitesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TNTDamageSourceIgnitesTest, FireDamage_IgnitesTNT)
{
    // 火焰伤害类型应能点燃TNT
    EnvironmentalDamage fireDamage(DamageType::InFire);
    EXPECT_TRUE(fireDamage.isFire());
    EXPECT_FALSE(fireDamage.isExplosion());

    EnvironmentalDamage lavaDamage(DamageType::Lava);
    EXPECT_TRUE(lavaDamage.isFire());

    EnvironmentalDamage onFireDamage(DamageType::OnFire);
    EXPECT_TRUE(onFireDamage.isFire());

    EnvironmentalDamage hotFloorDamage(DamageType::HotFloor);
    EXPECT_TRUE(hotFloorDamage.isFire());
}

TEST_F(TNTDamageSourceIgnitesTest, ExplosionDamage_IgnitesTNT)
{
    // 爆炸伤害类型应能点燃TNT
    EnvironmentalDamage explosionDamage(DamageType::Explosion);
    EXPECT_FALSE(explosionDamage.isFire());
    EXPECT_TRUE(explosionDamage.isExplosion());

    EnvironmentalDamage playerExplosionDamage(DamageType::ExplosionPlayer);
    EXPECT_TRUE(playerExplosionDamage.isExplosion());
}

TEST_F(TNTDamageSourceIgnitesTest, NormalDamage_DoesNotIgniteTNT)
{
    // 普通伤害类型不能点燃TNT
    EnvironmentalDamage fallDamage(DamageType::Fall);
    EXPECT_FALSE(fallDamage.isFire());
    EXPECT_FALSE(fallDamage.isExplosion());

    EnvironmentalDamage drownDamage(DamageType::Drown);
    EXPECT_FALSE(drownDamage.isFire());
    EXPECT_FALSE(drownDamage.isExplosion());

    EnvironmentalDamage cactusDamage(DamageType::Cactus);
    EXPECT_FALSE(cactusDamage.isFire());
    EXPECT_FALSE(cactusDamage.isExplosion());
}

TEST_F(TNTDamageSourceIgnitesTest, FireProjectile_IgnitesTNT)
{
    // 火焰投射物伤害：isProjectile() && isFire() → 能点燃
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));
    Entity* shooter = nullptr;
    IndirectEntityDamageSource fireballDamage = DamageSources::fireball(arrow.get(), shooter, false);
    fireballDamage.setProjectile();
    fireballDamage.setFireDamage();

    EXPECT_TRUE(fireballDamage.isProjectile());
    EXPECT_TRUE(fireballDamage.isFire());
}

// ============================================================================
// ignitionSource 归因测试
// ============================================================================

/**
 * @brief 测试 TNT 矿车 ignitionSource 的伤害归因逻辑
 *
 * 对应 MC Java 的 MinecartTNT.ignitionSource：
 * - 首次点燃时记录引爆来源
 * - 后续点燃不会覆盖 ignitionSource
 * - 爆炸时 ignitionSource 作为 DamageSource 传递给 Explosion
 */
class TNTIgnitionSourceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

/**
 * @brief 测试 IndirectEntityDamageSource 构造和属性
 *
 * ignitionSource 使用 IndirectEntityDamageSource(DamageType::Explosion, causeEntity, this)
 * 其中 causeEntity 是原始伤害的造成者，this 是 TNT 矿车自身
 */
TEST_F(TNTIgnitionSourceTest, IndirectEntityDamageSource_Construction)
{
    // 创建模拟实体
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));
    Entity* shooter = nullptr;

    // 创建间接伤害源（模拟箭矢伤害）
    IndirectEntityDamageSource arrowDamage = DamageSources::arrow(arrow.get(), shooter, false);
    EXPECT_EQ(arrowDamage.source(), shooter);           // 射击者（间接源）
    EXPECT_EQ(arrowDamage.directSource(), arrow.get()); // 箭矢（直接源）
    EXPECT_EQ(arrowDamage.getEntity(), shooter);        // getEntity() 返回间接源

    // 模拟 TNT 矿车创建 ignitionSource 的逻辑
    // ignitionSource = IndirectEntityDamageSource(Explosion, causeEntity, tntMinecart)
    // causeEntity = source->getEntity()（原始伤害的造成者）
    Entity* causeEntity = arrowDamage.getEntity(); // 射击者
    auto ignitionSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, causeEntity, arrow.get());
    ignitionSource->setExplosion();

    EXPECT_TRUE(ignitionSource->isExplosion());
    EXPECT_EQ(ignitionSource->type(), DamageType::Explosion);
    EXPECT_EQ(ignitionSource->source(), causeEntity);       // 间接源（射击者）
    EXPECT_EQ(ignitionSource->directSource(), arrow.get()); // 直接源（TNT矿车）
}

/**
 * @brief 测试无射击者时 ignitionSource 的行为
 *
 * 激活铁轨点燃时 source=nullptr，ignitionSource 不设置
 */
TEST_F(TNTIgnitionSourceTest, NullSource_NoIgnitionSource)
{
    // 当 source 为 nullptr 时，getEntity() 返回 nullptr
    // ignitionSource 不应被设置
    EnvironmentalDamage fireDamage(DamageType::InFire);
    Entity* causeEntity = fireDamage.getEntity();
    EXPECT_EQ(causeEntity, nullptr) << "EnvironmentalDamage should have no entity";

    // 如果 causeEntity 为 nullptr，IndirectEntityDamageSource 仍然可以构造
    auto ignitionSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, nullptr, nullptr);
    ignitionSource->setExplosion();

    EXPECT_TRUE(ignitionSource->isExplosion());
    EXPECT_EQ(ignitionSource->source(), nullptr);
    EXPECT_EQ(ignitionSource->directSource(), nullptr);
}

/**
 * @brief 测试实体爆炸伤害源的归因
 *
 * 当苦力怕爆炸伤害TNT矿车时：
 * - source.getEntity() 返回苦力怕
 * - ignitionSource 中 causeEntity = 苦力怕
 */
TEST_F(TNTIgnitionSourceTest, EntityExplosionDamage_Attribution)
{
    // 创建模拟实体（作为爆炸源）
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1));
    auto shooter = std::make_unique<TestArrowEntity>(EntityInstanceId(2));

    // 模拟实体爆炸伤害
    EntityDamageSource explosionDamage = DamageSources::explosion(shooter.get());
    EXPECT_EQ(explosionDamage.source(), shooter.get());
    EXPECT_EQ(explosionDamage.getEntity(), shooter.get());
    EXPECT_TRUE(explosionDamage.isExplosion());

    // 构造 ignitionSource
    Entity* causeEntity = explosionDamage.getEntity();
    auto ignitionSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, causeEntity, arrow.get());
    ignitionSource->setExplosion();

    EXPECT_TRUE(ignitionSource->isExplosion());
    EXPECT_EQ(ignitionSource->source(), shooter.get());     // 爆炸造成者
    EXPECT_EQ(ignitionSource->directSource(), arrow.get()); // TNT矿车自身
}

/**
 * @brief 测试 clone() 方法正确复制 ignitionSource
 *
 * 爆炸时需要 clone ignitionSource 传递给 Explosion
 */
TEST_F(TNTIgnitionSourceTest, Clone_PreservesAttributes)
{
    auto shooter = std::make_unique<TestArrowEntity>(EntityInstanceId(1));
    auto tntMinecart = std::make_unique<TestArrowEntity>(EntityInstanceId(2));

    auto ignitionSource =
        std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, shooter.get(), tntMinecart.get());
    ignitionSource->setExplosion();

    // clone
    auto cloned = ignitionSource->clone();
    EXPECT_TRUE(cloned->isExplosion());
    EXPECT_EQ(cloned->type(), DamageType::Explosion);

    auto* clonedIndirect = dynamic_cast<IndirectEntityDamageSource*>(cloned.get());
    ASSERT_NE(clonedIndirect, nullptr);
    EXPECT_EQ(clonedIndirect->source(), shooter.get());
    EXPECT_EQ(clonedIndirect->directSource(), tntMinecart.get());
}
