/**
 * @file ArmorStandEntityTest.cpp
 * @brief 盔甲架装备交互、装备掉落与存档测试
 *
 * 覆盖对齐 vanilla ArmorStand.interactAt / brokenByAnything / addAdditionalSaveData 的行为：
 * - 盔甲自动进入对应护甲槽、非盔甲物品进入主手、名称牌与标记模式放行
 * - 未开启手臂时手部槽位不可交互（Fail）
 * - 空手按命中点取回对应部位装备
 * - 破坏时逐个掉出穿戴装备，并清空槽位
 * - Invisible / Small / ShowArms / DisabledSlots / NoBasePlate / Pose / equipment 存档往返
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

/// 记录生成的实体（用于断言装备掉落）并记录音效的测试世界
class ArmorStandTestWorld final : public mc::test::BaseTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundId = soundId;
        m_soundPlayCount++;
        (void)category;
        (void)pos;
        (void)volume;
        (void)pitch;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const
    {
        return m_spawnedEntities;
    }

    [[nodiscard]] i32 soundPlayCount() const
    {
        return m_soundPlayCount;
    }

    void resetSoundTracking()
    {
        m_lastSoundId = ResourceLocation();
        m_soundPlayCount = 0;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ArmorStandTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ArmorStandTestWorld::tickManager not implemented");
    }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    ResourceLocation m_lastSoundId;
    i32 m_soundPlayCount = 0;
};

/**
 * @brief 收集世界中生成的全部物品实体的物品 ID
 */
std::vector<const Item*> spawnedItemTypes(const ArmorStandTestWorld& world)
{
    std::vector<const Item*> result;
    for (const auto& entity : world.spawnedEntities()) {
        const auto* item = dynamic_cast<const ItemEntity*>(entity.get());
        if (item == nullptr) {
            continue;
        }
        result.push_back(item->getItemStack().getItem());
    }
    return result;
}

} // namespace

class ArmorStandEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
    }

    /// 构造与世界绑定的盔甲架
    std::unique_ptr<ArmorStandEntity> _makeArmorStand(ArmorStandTestWorld& world)
    {
        auto stand = std::make_unique<ArmorStandEntity>(mc::test::testEcsRegistry());
        stand->setWorld(&world);
        stand->setTypeId("minecraft:armor_stand");
        return stand;
    }

    /// 构造主手持指定物品的测试玩家
    std::unique_ptr<Player> _makePlayer(const ItemStack& heldItem)
    {
        auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
        player->setTypeId("minecraft:player");
        player->inventory().setItem(0, heldItem);
        player->inventory().setSelectedSlot(0);
        return player;
    }
};

// ============================================================================
// 装备交互
// ============================================================================

TEST_F(ArmorStandEntityTest, InteractAt_WithChestplate_EquipsChestSlot)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    auto player = _makePlayer(ItemStack(*Items::IRON_CHESTPLATE, 1));

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::SuccessServer);
    ASSERT_NE(stand->getEquipment(EquipmentSlot::Chest).getItem(), nullptr);
    EXPECT_EQ(stand->getEquipment(EquipmentSlot::Chest).getItem(), Items::IRON_CHESTPLATE);
    EXPECT_TRUE(player->getHeldItem(Hand::MainHand).isEmpty());
    // 护甲进胸甲槽时按材质播放装备音效
    EXPECT_EQ(world.soundPlayCount(), 1);
}

TEST_F(ArmorStandEntityTest, InteractAt_WithChestplate_CountGreaterThanOne_MovesOne)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    auto player = _makePlayer(ItemStack(*Items::IRON_CHESTPLATE, 3));

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::SuccessServer);
    EXPECT_EQ(stand->getEquipment(EquipmentSlot::Chest).getCount(), 1);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 2);
}

TEST_F(ArmorStandEntityTest, InteractAt_WithNonArmorItem_FailsWhenArmsHidden)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    ASSERT_FALSE(stand->hasArms());
    ItemStack stick = ItemStack(*Items::STICK, 1);
    auto player = _makePlayer(stick);

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Fail);
    EXPECT_TRUE(stand->getEquipment(EquipmentSlot::MainHand).isEmpty());
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 1);
}

TEST_F(ArmorStandEntityTest, InteractAt_WithNonArmorItem_EntersMainHandWhenArmsShown)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    stand->setArms(true);
    ItemStack stick = ItemStack(*Items::STICK, 1);
    auto player = _makePlayer(stick);

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::SuccessServer);
    ASSERT_NE(stand->getEquipment(EquipmentSlot::MainHand).getItem(), nullptr);
    EXPECT_EQ(stand->getEquipment(EquipmentSlot::MainHand).getItem(), Items::STICK);
    EXPECT_TRUE(player->getHeldItem(Hand::MainHand).isEmpty());
    // 非盔甲物品不播放护甲装备音效
    EXPECT_EQ(world.soundPlayCount(), 0);
}

TEST_F(ArmorStandEntityTest, InteractAt_WithNameTag_Passes)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    auto player = _makePlayer(ItemStack(*Items::NAME_TAG, 1));

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 1);
}

TEST_F(ArmorStandEntityTest, InteractAt_MarkerMode_PassesEvenWithArmor)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    stand->setMarker(true);
    auto player = _makePlayer(ItemStack(*Items::IRON_HELMET, 1));

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_TRUE(stand->getEquipment(EquipmentSlot::Head).isEmpty());
}

TEST_F(ArmorStandEntityTest, InteractAt_DisabledSlot_Fails)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    // 禁用头部槽位"放入"位：槽位序号 Head=5（MainHand=0/OffHand=1/Feet=2/Legs=3/Chest=4/Head=5），
    // 位组内"放入"位偏移 0，故掩码位号 = 5 * 16 + 0 = 80。
    // 注意：C++ 的 32 位移位只取位移量低 5 位，1u << 80 实际等于 1u << 16，必须分两步。
    constexpr u32 kPlaceBitIndex = 5u * 16u;
    constexpr u32 kDisabledHeadMask = (1u << (kPlaceBitIndex & 31u)) << (kPlaceBitIndex >> 5u);
    stand->setDisabledSlots(static_cast<i32>(kDisabledHeadMask));
    ASSERT_EQ(stand->disabledSlots(), static_cast<i32>(kDisabledHeadMask));

    auto player = _makePlayer(ItemStack(*Items::IRON_HELMET, 1));
    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Fail);
    EXPECT_TRUE(stand->getEquipment(EquipmentSlot::Head).isEmpty());
}

TEST_F(ArmorStandEntityTest, InteractAt_EmptyHand_TakesChestplateByHitHeight)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    stand->setEquipment(EquipmentSlot::Chest, ItemStack(*Items::DIAMOND_CHESTPLATE, 1));
    auto player = _makePlayer(ItemStack());

    // 命中点位于胸甲判定区间（0.9 ≤ y < 1.6）
    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::SuccessServer);
    EXPECT_TRUE(stand->getEquipment(EquipmentSlot::Chest).isEmpty());
    ASSERT_NE(player->getHeldItem(Hand::MainHand).getItem(), nullptr);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getItem(), Items::DIAMOND_CHESTPLATE);
}

TEST_F(ArmorStandEntityTest, InteractAt_EmptyHand_OnEmptyStand_Passes)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    auto player = _makePlayer(ItemStack());

    const ActionResultType result = stand->applyPlayerInteraction(*player, Vector3(0.0f, 1.2f, 0.0f), Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_TRUE(player->getHeldItem(Hand::MainHand).isEmpty());
}

// ============================================================================
// 装备掉落
// ============================================================================

TEST_F(ArmorStandEntityTest, Hurt_MidHealth_DoesNotDropEquipment)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    stand->setEquipment(EquipmentSlot::Head, ItemStack(*Items::IRON_HELMET, 1));

    // 仙人掌伤害不在盔甲架的任意破坏标签内，hurt 直接返回 false、不扣血也不掉落，
    // 用于验证"非破坏性伤害不触发装备掉落"。
    auto source = DamageSources::cactus();
    stand->hurt(source, 0.15f);

    // 饥饿伤害走 BURNS_ARMOR_STANDS 分支的轻伤路径（生命值 2.0 → 1.85，仍高于 0.5）
    auto starveSource = DamageSources::starve();
    stand->hurt(starveSource, 0.15f);

    EXPECT_TRUE(world.spawnedEntities().empty());
    EXPECT_FALSE(stand->getEquipment(EquipmentSlot::Head).isEmpty());
}

TEST_F(ArmorStandEntityTest, CauseDamageToDeath_DropsAllWornEquipment)
{
    ArmorStandTestWorld world;
    auto stand = _makeArmorStand(world);
    stand->setEquipment(EquipmentSlot::Head, ItemStack(*Items::IRON_HELMET, 1));
    stand->setEquipment(EquipmentSlot::Chest, ItemStack(*Items::DIAMOND_CHESTPLATE, 1));

    // 着火状态下 solid fire 伤害 4.0 直接扣至 0.5 以下，走 brokenByAnything + kill
    auto source = DamageSources::onFire();
    stand->hurt(source, 4.0f);

    EXPECT_TRUE(stand->isRemoved());
    EXPECT_TRUE(stand->getEquipment(EquipmentSlot::Head).isEmpty());
    EXPECT_TRUE(stand->getEquipment(EquipmentSlot::Chest).isEmpty());

    // 掉出两件装备（落点为物品实体）
    const auto dropped = spawnedItemTypes(world);
    EXPECT_EQ(dropped.size(), 2u);
}

// ============================================================================
// 存档往返
// ============================================================================

TEST_F(ArmorStandEntityTest, NbtRoundTrip_StateAndEquipment)
{
    auto stand = std::make_unique<ArmorStandEntity>(mc::test::testEcsRegistry());
    stand->setInvisible(true);
    stand->setSmall(true);
    stand->setArms(true);
    stand->setBasePlate(false);
    // 头部槽位（序号 5）禁用"放入"（组内 +0 → 位号 80），胸甲槽位（序号 4）禁用"取出"（组内 +8 → 位号 72）。
    // 位号超过 31，按"低 5 位 + 高位移位"两段计算，避免 32 位移位只取低 5 位的陷阱。
    const auto disabledBit = [](u32 bitIndex) -> u32 {
        return (1u << (bitIndex & 31u)) << (bitIndex >> 5u);
    };
    const i32 disabledSlots = static_cast<i32>(disabledBit(5u * 16u) | disabledBit(4u * 16u + 8u));
    stand->setDisabledSlots(disabledSlots);
    stand->setHeadRotation(10.0f, 20.0f, 30.0f);
    stand->setLeftArmRotation(-45.0f, 0.0f, 90.0f);
    stand->setEquipment(EquipmentSlot::Chest, ItemStack(*Items::IRON_CHESTPLATE, 1));

    nbt::tags::compound_tag tag;
    stand->writeToNBT(tag);

    using namespace mc::entity::serialization;
    EXPECT_TRUE(nbt_helper::tryGetBool(tag, nbt_keys::ARMOR_STAND_INVISIBLE).value_or(false));
    EXPECT_TRUE(nbt_helper::tryGetBool(tag, nbt_keys::ARMOR_STAND_SMALL).value_or(false));
    EXPECT_TRUE(nbt_helper::tryGetBool(tag, nbt_keys::ARMOR_STAND_SHOW_ARMS).value_or(false));
    EXPECT_TRUE(nbt_helper::tryGetBool(tag, nbt_keys::ARMOR_STAND_NO_BASE_PLATE).value_or(false));
    EXPECT_TRUE(nbt_helper::tryGetCompound(tag, nbt_keys::ARMOR_STAND_POSE) != nullptr);
    EXPECT_TRUE(nbt_helper::tryGetCompound(tag, nbt_keys::EQUIPMENT) != nullptr);

    auto restored = std::make_unique<ArmorStandEntity>(mc::test::testEcsRegistry());
    const auto result = restored->readFromNBT(tag);
    EXPECT_TRUE(result.success());

    EXPECT_TRUE(restored->isInvisible());
    EXPECT_TRUE(restored->isSmall());
    EXPECT_TRUE(restored->hasArms());
    EXPECT_FALSE(restored->hasBasePlate());
    EXPECT_EQ(restored->disabledSlots(), disabledSlots);
    EXPECT_FLOAT_EQ(restored->armorStandState().m_pose.head[0], 10.0f);
    EXPECT_FLOAT_EQ(restored->armorStandState().m_pose.head[2], 30.0f);
    EXPECT_FLOAT_EQ(restored->armorStandState().m_pose.leftArm[0], -45.0f);
    EXPECT_FLOAT_EQ(restored->armorStandState().m_pose.leftArm[2], 90.0f);
    ASSERT_NE(restored->getEquipment(EquipmentSlot::Chest).getItem(), nullptr);
    EXPECT_EQ(restored->getEquipment(EquipmentSlot::Chest).getItem(), Items::IRON_CHESTPLATE);
}