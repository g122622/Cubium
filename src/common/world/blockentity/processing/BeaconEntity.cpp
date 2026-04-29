#include "world/blockentity/processing/BeaconEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

#include <algorithm>
#include <limits>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 信标每次重新计算结构与应用效果的 tick 间隔。
 */
constexpr i32 BEACON_UPDATE_INTERVAL = 80;

/**
 * @brief 支付物品 JSON 字段名。
 */
constexpr const char* PAYMENT_ITEM_KEY = "payment_item";

/**
 * @brief 将效果类型编码为稳定整数 ID（0 表示空效果）。
 */
[[nodiscard]] i32 encodeEffect(BeaconEntity::EffectType effect) {
    return static_cast<i32>(effect) + 1;
}

/**
 * @brief 将整数 ID 解码为效果类型。
 */
[[nodiscard]] bool decodeEffect(i32 rawValue, BeaconEntity::EffectType& outEffect) {
    if (rawValue <= 0) {
        return false;
    }

    const i32 enumValue = rawValue - 1;
    if (enumValue <= 0 || enumValue >= static_cast<i32>(entity::effect::EffectType::Count)) {
        return false;
    }

    outEffect = static_cast<BeaconEntity::EffectType>(enumValue);
    return true;
}

/**
 * @brief 检查信标基座方块是否属于可用矿物方块。
 *
 * MC 1.16.5 参考: BlockTags.BEACON_BASE_BLOCKS
 * 包含: 铁块、金块、钻石块、绿宝石块、下界合金块
 */
[[nodiscard]] bool isValidBeaconBaseBlock(const BlockState* state) {
    if (state == nullptr || state->isAir()) {
        return false;
    }

    const Block* block = &state->getBlock();
    return block == VanillaBlocks::IRON_BLOCK ||
           block == VanillaBlocks::GOLD_BLOCK ||
           block == VanillaBlocks::DIAMOND_BLOCK ||
           block == VanillaBlocks::EMERALD_BLOCK;
           // TODO: 添加 VanillaBlocks::NETHERITE_BLOCK 当方块注册后
}

/**
 * @brief 将效果以环境效果形式施加给玩家。
 */
void applyBeaconEffectToPlayer(Player& player,
                               BeaconEntity::EffectType effectType,
                               i32 durationTicks,
                               i32 amplifier) {
    const entity::effect::EffectInstance effect(
        effectType,
        durationTicks,
        amplifier,
        true,
        true,
        true);
    player.addEffect(effect);
}

} // namespace

// ========== 静态数据初始化 ==========

namespace {

// 信标效果存储 - 用于初始化VALID_EFFECTS
// 使用静态变量确保地址稳定
static const BeaconEntity::EffectType s_level1Effects[] = {
    BeaconEntity::EffectType::Speed,
    BeaconEntity::EffectType::Haste
};

static const BeaconEntity::EffectType s_level2Effects[] = {
    BeaconEntity::EffectType::Resistance,
    BeaconEntity::EffectType::JumpBoost
};

static const BeaconEntity::EffectType s_level3Effects[] = {
    BeaconEntity::EffectType::Strength
};

static const BeaconEntity::EffectType s_level4Effects[] = {
    BeaconEntity::EffectType::Regeneration
};

} // namespace

// 信标等级对应的有效效果
// Level 1: Speed, Haste
// Level 2: Resistance, JumpBoost
// Level 3: Strength
// Level 4: Regeneration (仅作为辅助效果)
const std::array<std::vector<const BeaconEntity::EffectType*>, 4> BeaconEntity::VALID_EFFECTS = {
    // Level 1: 速度、急迫
    std::vector<const EffectType*>{&s_level1Effects[0], &s_level1Effects[1]},
    // Level 2: 抗性提升、跳跃提升
    std::vector<const EffectType*>{&s_level2Effects[0], &s_level2Effects[1]},
    // Level 3: 力量
    std::vector<const EffectType*>{&s_level3Effects[0]},
    // Level 4: 生命恢复（仅辅助效果）
    std::vector<const EffectType*>{&s_level4Effects[0]}
};

// ========== BeaconEntity 实现 ==========

BeaconEntity::BeaconEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Beacon, pos) {
}

BeaconEntity::~BeaconEntity() = default;

void BeaconEntity::setLevel(i32 level) {
    level = std::max(0, std::min(level, MAX_LEVELS));
    if (m_level != level) {
        m_level = level;
        setChanged();
    }
}

void BeaconEntity::setPrimaryEffect(const EffectType* effect) {
    const std::optional<EffectType> nextEffect = effect != nullptr ? std::optional<EffectType>(*effect) : std::nullopt;
    if (m_primaryEffect != nextEffect) {
        m_primaryEffect = nextEffect;
        setChanged();
    }
}

void BeaconEntity::setSecondaryEffect(const EffectType* effect) {
    const std::optional<EffectType> nextEffect = effect != nullptr ? std::optional<EffectType>(*effect) : std::nullopt;
    if (m_secondaryEffect != nextEffect) {
        m_secondaryEffect = nextEffect;
        setChanged();
    }
}

void BeaconEntity::setPaymentItem(const ItemStack& stack) {
    if (!stack.isEmpty()) {
        MC_ASSERT_RELEASE(isValidPayment(stack.getItem()->itemId()));
    }
    m_paymentItem = stack;
    setChanged();
}

bool BeaconEntity::canUseEffect(const EffectType* effect) const {
    if (effect == nullptr) {
        return true;
    }

    const i32 effectValue = static_cast<i32>(*effect);
    return effectValue > 0 && effectValue < static_cast<i32>(entity::effect::EffectType::Count);
}

i32 BeaconEntity::getEffectRange() const {
    return m_level * 10 + 10;
}

void BeaconEntity::tick(IWorld& world) {
    m_tickCount++;

    if (m_tickCount % BEACON_UPDATE_INTERVAL == 0) {
        updateLevels(world);
    }

    if (isActive() && m_tickCount % BEACON_UPDATE_INTERVAL == 0) {
        applyEffects(world);
    }
}

void BeaconEntity::updateLevels(IWorld& world) {
    // MC 1.16.5: 金字塔检测不需要检查天空可见性
    // 参考: BeaconTileEntity.checkBeaconLevel()
    i32 newLevel = 0;

    for (i32 level = 1; level <= MAX_LEVELS; ++level) {
        const i32 y = m_pos.y - level;
        bool levelValid = true;

        for (i32 dx = -level; dx <= level && levelValid; ++dx) {
            for (i32 dz = -level; dz <= level; ++dz) {
                const BlockPos basePos(m_pos.x + dx, y, m_pos.z + dz);
                if (!isValidBeaconBaseBlock(world.getBlockState(basePos))) {
                    levelValid = false;
                    break;
                }
            }
        }

        if (!levelValid) {
            break;
        }
        newLevel = level;
    }

    setLevel(newLevel);
}

bool BeaconEntity::canSeeSky(IWorld& world) const {
    const i32 maxY = world.getHeight(m_pos.x, m_pos.z);
    for (i32 y = m_pos.y + 1; y <= maxY; ++y) {
        const BlockState* state = world.getBlockState(m_pos.x, y, m_pos.z);
        if (state != nullptr && !state->isAir()) {
            return false;
        }
    }
    return true;
}

void BeaconEntity::applyEffects(IWorld& world) {
    if (m_level <= 0 || !m_primaryEffect.has_value()) {
        return;
    }

    i32 amplifier = 0;
    if (m_level >= 4 && m_secondaryEffect.has_value() && m_secondaryEffect.value() == m_primaryEffect.value()) {
        amplifier = 1;
    }

    const i32 durationTicks = (9 + m_level * 2) * 20;
    const i32 range = getEffectRange();
    const Vector3 center(
        static_cast<f32>(m_pos.x) + 0.5f,
        static_cast<f32>(m_pos.y) + 0.5f,
        static_cast<f32>(m_pos.z) + 0.5f);

    const std::vector<Entity*> entities = world.getEntitiesInRange(center, static_cast<f32>(range), nullptr);
    for (Entity* entity : entities) {
        auto* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        applyBeaconEffectToPlayer(*player, m_primaryEffect.value(), durationTicks, amplifier);

        if (m_level >= 4 && m_secondaryEffect.has_value() && m_secondaryEffect.value() != m_primaryEffect.value()) {
            applyBeaconEffectToPlayer(*player, m_secondaryEffect.value(), durationTicks, 0);
        }
    }
}

bool BeaconEntity::isValidPayment(u32 itemId) {
    if (itemId > static_cast<u32>(std::numeric_limits<ItemId>::max())) {
        return false;
    }

    const Item* item = ItemRegistry::instance().getItem(static_cast<ItemId>(itemId));
    if (item == nullptr) {
        return false;
    }

    return item == Items::IRON_INGOT ||
           item == Items::GOLD_INGOT ||
           item == Items::DIAMOND ||
           item == Items::EMERALD ||
           item == Items::NETHERITE_INGOT;
}

bool BeaconEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    setLevel(data.value("level", 0));

    m_primaryEffect = std::nullopt;
    m_secondaryEffect = std::nullopt;

    if (data.contains("primary_effect") && data["primary_effect"].is_number_integer()) {
        EffectType primary = EffectType::Speed;
        if (decodeEffect(data["primary_effect"].get<i32>(), primary)) {
            m_primaryEffect = primary;
        }
    }

    if (data.contains("secondary_effect") && data["secondary_effect"].is_number_integer()) {
        EffectType secondary = EffectType::Speed;
        if (decodeEffect(data["secondary_effect"].get<i32>(), secondary)) {
            m_secondaryEffect = secondary;
        }
    }

    m_paymentItem = ItemStack::EMPTY;
    if (data.contains(PAYMENT_ITEM_KEY)) {
        const auto paymentResult = ItemStack::fromJson(data[PAYMENT_ITEM_KEY]);
        if (paymentResult.success()) {
            const ItemStack loadedPayment = paymentResult.value();
            if (loadedPayment.isEmpty() || isValidPayment(loadedPayment.getItem()->itemId())) {
                m_paymentItem = loadedPayment;
            }
        }
    }

    return true;
}

void BeaconEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    data["level"] = m_level;

    if (m_primaryEffect.has_value()) {
        data["primary_effect"] = encodeEffect(m_primaryEffect.value());
    } else {
        data["primary_effect"] = 0;
    }

    if (m_secondaryEffect.has_value()) {
        data["secondary_effect"] = encodeEffect(m_secondaryEffect.value());
    } else {
        data["secondary_effect"] = 0;
    }

    if (!m_paymentItem.isEmpty()) {
        data[PAYMENT_ITEM_KEY] = m_paymentItem.toJson();
    } else {
        data.erase(PAYMENT_ITEM_KEY);
    }
}

std::unique_ptr<BlockEntity> BeaconEntity::clone() const {
    auto cloned = std::make_unique<BeaconEntity>(m_pos);
    cloned->m_level = m_level;
    cloned->m_tickCount = m_tickCount;
    cloned->m_primaryEffect = m_primaryEffect;
    cloned->m_secondaryEffect = m_secondaryEffect;
    cloned->m_paymentItem = m_paymentItem.copy();
    cloned->m_lastBeamState = m_lastBeamState;
    return cloned;
}

} // namespace blockentity
} // namespace mc
