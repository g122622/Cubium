#include "world/blockentity/processing/BeaconEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "entity/Player.hpp"
#include "item/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== 静态数据初始化 ==========

// 每个等级可用的效果
const std::array<std::vector<const BeaconEntity::EffectType*>, 4> BeaconEntity::VALID_EFFECTS = {
    // 等级1: 速度, 急迫
    std::vector<const EffectType*>{
        // &effect::MobEffects::SPEED,
        // &effect::MobEffects::HASTE
    },
    // 等级2: 速度, 急迫, 抗性提升, 跳跃提升
    std::vector<const EffectType*>{
        // &effect::MobEffects::SPEED,
        // &effect::MobEffects::HASTE,
        // &effect::MobEffects::RESISTANCE,
        // &effect::MobEffects::JUMP_BOOST
    },
    // 等级3: 速度, 急迫, 抗性提升, 跳跃提升, 力量
    std::vector<const EffectType*>{
        // &effect::MobEffects::SPEED,
        // &effect::MobEffects::HASTE,
        // &effect::MobEffects::RESISTANCE,
        // &effect::MobEffects::JUMP_BOOST,
        // &effect::MobEffects::STRENGTH
    },
    // 等级4: 同等级3，但辅助效果可以是生命恢复
    std::vector<const EffectType*>{
        // &effect::MobEffects::SPEED,
        // &effect::MobEffects::HASTE,
        // &effect::MobEffects::RESISTANCE,
        // &effect::MobEffects::JUMP_BOOST,
        // &effect::MobEffects::STRENGTH
    }
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
    if (m_primaryEffect != effect) {
        m_primaryEffect = effect;
        setChanged();
    }
}

void BeaconEntity::setSecondaryEffect(const EffectType* effect) {
    if (m_secondaryEffect != effect) {
        m_secondaryEffect = effect;
        setChanged();
    }
}

void BeaconEntity::setPaymentItem(const ItemStack& stack) {
    m_paymentItem = stack;
    setChanged();
}

bool BeaconEntity::canUseEffect(const EffectType* effect) const {
    if (effect == nullptr) {
        return true; // 无效果总是允许
    }

    // 检查效果是否在有效列表中
    for (const auto& effects : VALID_EFFECTS) {
        for (const auto& e : effects) {
            if (e == effect) {
                return true;
            }
        }
    }
    return false;
}

i32 BeaconEntity::getEffectRange() const {
    // 效果范围 = 等级 * 10 + 10
    // 等级1: 20格, 等级2: 30格, 等级3: 40格, 等级4: 50格
    return m_level * 10 + 10;
}

void BeaconEntity::tick(IWorld& world) {
    m_tickCount++;

    // 每80tick检查一次金字塔
    if (m_tickCount % 80 == 0) {
        updateLevels(world);
    }

    // 激活状态下应用效果
    if (isActive() && m_tickCount % 80 == 0) {
        applyEffects(world);
    }
}

void BeaconEntity::updateLevels(IWorld& world) {
    // 检查金字塔结构
    i32 newLevel = 0;

    for (i32 level = 1; level <= MAX_LEVELS; ++level) {
        bool levelValid = true;

        // 检查当前层是否由有效方块组成
        // 金字塔结构：每层比上一层大一圈
        // TODO: 实现金字塔检测逻辑

        if (levelValid) {
            newLevel = level;
        } else {
            break;
        }
    }

    // 检查是否能看到天空
    if (newLevel > 0 && !canSeeSky(world)) {
        newLevel = 0;
    }

    setLevel(newLevel);
}

bool BeaconEntity::canSeeSky(IWorld& world) const {
    // TODO: 检查上方是否有方块遮挡
    // 暂时返回true
    MC_UNUSED(world);
    return true;
}

void BeaconEntity::applyEffects(IWorld& world) {
    if (m_level <= 0 || m_primaryEffect == nullptr) {
        return;
    }

    // 计算效果持续时间
    i32 duration = 9 + m_level * 2; // 秒

    // 计算效果等级
    i32 amplifier = 0;
    if (m_level >= 4 && m_secondaryEffect == m_primaryEffect) {
        // 等级4且辅助效果与主效果相同时，效果等级+1
        amplifier = 1;
    }

    // 获取范围内的玩家
    i32 range = getEffectRange();
    // TODO: 获取范围内的玩家并应用效果

    MC_UNUSED(world);
    MC_UNUSED(duration);
    MC_UNUSED(amplifier);
}

bool BeaconEntity::isValidPayment(u32 itemId) {
    // 有效的支付物品：铁锭、金锭、绿宝石、钻石、下界合金锭
    // TODO: 实现物品ID检查
    MC_UNUSED(itemId);
    return false;
}

bool BeaconEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("level")) {
        m_level = data["level"].get<i32>();
    }

    if (data.contains("primary_effect")) {
        // TODO: 加载效果
    }

    if (data.contains("secondary_effect")) {
        // TODO: 加载效果
    }

    // 加载支付物品
    if (data.contains("payment_item")) {
        // TODO: 加载ItemStack
    }

    return true;
}

void BeaconEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    data["level"] = m_level;

    // TODO: 保存效果
    if (m_primaryEffect != nullptr) {
        // data["primary_effect"] = m_primaryEffect->getName();
    }

    if (m_secondaryEffect != nullptr) {
        // data["secondary_effect"] = m_secondaryEffect->getName();
    }

    // 保存支付物品
    if (!m_paymentItem.isEmpty()) {
        // TODO: 保存ItemStack
    }
}

std::unique_ptr<BlockEntity> BeaconEntity::clone() const {
    auto clone = std::make_unique<BeaconEntity>(m_pos);
    clone->m_level = m_level;
    clone->m_primaryEffect = m_primaryEffect;
    clone->m_secondaryEffect = m_secondaryEffect;
    // TODO: 复制支付物品
    return clone;
}

} // namespace blockentity
} // namespace mc
