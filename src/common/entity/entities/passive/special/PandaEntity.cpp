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

#include "PandaEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp" // 包含 LookRandomlyGoal
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/PandaGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp" // HurtByTargetGoal
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

PandaEntity::PandaEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 随机生成性格
    randomizePersonality();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 补调 registerData：AnimalEntity 构造期间 vtable 指向基类，派生 override 不执行，
    // 须在派生类构造显式调用（同 BeeEntity）。
    registerData();
}

entity::DataParameter<i8> PandaEntity::DATA_PANDA_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();

void PandaEntity::registerData()
{
    AnimalEntity::registerData();
    // 对齐 vanilla Panda.DATA_ID_FLAGS（Byte，默认 0）：同步标志位字节。
    m_dataManager.registerParam(DATA_PANDA_FLAGS_PARAM, static_cast<i8>(0));
}

bool PandaEntity::_getPandaFlag(i8 flag) const
{
    return (static_cast<u8>(m_dataManager.get(DATA_PANDA_FLAGS_PARAM)) & static_cast<u8>(flag)) != 0;
}

void PandaEntity::_setPandaFlag(i8 flag, bool value)
{
    u8 flags = static_cast<u8>(m_dataManager.get(DATA_PANDA_FLAGS_PARAM));
    if (value) {
        flags = static_cast<u8>(flags | static_cast<u8>(flag));
    } else {
        flags = static_cast<u8>(flags & ~static_cast<u8>(flag));
    }
    m_dataManager.set(DATA_PANDA_FLAGS_PARAM, static_cast<i8>(flags));
}

bool PandaEntity::isSneezing() const
{
    return _getPandaFlag(PANDA_FLAG_SNEEZING);
}

void PandaEntity::setSneezing(bool sneezing)
{
    _setPandaFlag(PANDA_FLAG_SNEEZING, sneezing);
}

bool PandaEntity::isEating() const
{
    return m_eating;
}

void PandaEntity::setEating(bool eating)
{
    m_eating = eating;
}

bool PandaEntity::isSitting() const
{
    return _getPandaFlag(PANDA_FLAG_SITTING);
}

bool PandaEntity::isOnBack() const
{
    return _getPandaFlag(PANDA_FLAG_ON_BACK);
}

void PandaEntity::setOnBack(bool onBack)
{
    _setPandaFlag(PANDA_FLAG_ON_BACK, onBack);
}

std::unique_ptr<Entity> PandaEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PandaEntity>(0, registry);
}

void PandaEntity::randomizePersonality()
{
    math::IRandom& rng = getRandom();

    // 熊猫性格概率分布
    // 普通: 32%, 懒惰: 32%, 忧愁: 16%, 顽皮: 16%, 好斗: 1.6%, 虚弱: 0.08%, 棕色: 2.4%
    i32 value = rng.nextInt(0, 1249);

    if (value == 0) {
        // 虚弱（极稀有）
        m_personality = Personality::Weak;
        m_mainGene = 5;
    } else if (value <= 19) {
        // 好斗（稀有）
        m_personality = Personality::Aggressive;
        m_mainGene = 4;
    } else if (value <= 49) {
        // 棕色（较稀有）
        m_personality = Personality::Brown;
        m_mainGene = 6;
    } else if (value <= 249) {
        // 忧愁
        m_personality = Personality::Worried;
        m_mainGene = 2;
    } else if (value <= 449) {
        // 顽皮
        m_personality = Personality::Playful;
        m_mainGene = 3;
    } else if (value <= 849) {
        // 懒惰
        m_personality = Personality::Lazy;
        m_mainGene = 1;
    } else {
        // 普通
        m_personality = Personality::Normal;
        m_mainGene = 0;
    }

    // 隐藏基因随机
    m_hiddenGene = static_cast<u8>(rng.nextInt(0, 5));
}

bool PandaEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 检查物品是否为竹子
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // Items::BAMBOO 可能在初始化期间为 nullptr
    if (Items::BAMBOO == nullptr) {
        return false;
    }
    return item == Items::BAMBOO;
}

// ========== 基因系统实现 ==========

PandaEntity::Personality PandaEntity::calculateExpressedPersonality() const
{
    // 根据主基因和隐藏基因计算表达的性格
    // 基因表达规则：
    // 1. 如果主基因是显性的（Aggressive），直接返回主基因
    // 2. 如果主基因是隐性的（Lazy、Worried、Playful、Weak、Brown、Normal）：
    //    a. 如果主基因是 Lazy 且隐藏基因是 Aggressive，返回 Aggressive
    //    b. 否则返回主基因
    // 注意：AggressiveLazy 性格实际上未被使用

    u8 mainGene = m_mainGene;
    u8 hiddenGene = m_hiddenGene;

    // 隐性基因列表（非 Aggressive 的基因都是隐性的）
    constexpr u8 AGGRESSIVE = static_cast<u8>(Personality::Aggressive);
    constexpr u8 LAZY = static_cast<u8>(Personality::Lazy);

    // 如果主基因是好斗，直接返回好斗（显性）
    if (mainGene == AGGRESSIVE) {
        return Personality::Aggressive;
    }

    // 如果隐藏基因是好斗且主基因是懒惰，返回好斗（特殊规则）
    if (mainGene == LAZY && hiddenGene == AGGRESSIVE) {
        return Personality::Aggressive;
    }

    // 默认返回主基因
    return static_cast<Personality>(mainGene);
}

u8 PandaEntity::getOneOfGenesRandomly(math::IRandom& rng) const
{
    return rng.nextBoolean() ? m_mainGene : m_hiddenGene;
}

void PandaEntity::inheritGenesFromParents(PandaEntity* father, PandaEntity* mother)
{
    if (m_world == nullptr) {
        return;
    }

    math::IRandom& rng = m_world->getRandom();

    if (mother == nullptr) {
        // 只有父亲时，随机分配父亲的一个基因
        if (rng.nextBoolean()) {
            m_mainGene = father->getOneOfGenesRandomly(rng);
            m_hiddenGene = static_cast<u8>(rng.nextInt(0, 5)); // 随机隐藏基因
        } else {
            m_mainGene = static_cast<u8>(rng.nextInt(0, 5)); // 随机主基因
            m_hiddenGene = father->getOneOfGenesRandomly(rng);
        }
    } else {
        // 双亲都有时，随机从父母各取一个基因
        if (rng.nextBoolean()) {
            m_mainGene = father->getOneOfGenesRandomly(rng);
            m_hiddenGene = mother->getOneOfGenesRandomly(rng);
        } else {
            m_mainGene = mother->getOneOfGenesRandomly(rng);
            m_hiddenGene = father->getOneOfGenesRandomly(rng);
        }
    }

    // 1/32 概率发生变异
    if (rng.nextInt(32) == 0) {
        m_mainGene = static_cast<u8>(rng.nextInt(0, 5));
    }

    if (rng.nextInt(32) == 0) {
        m_hiddenGene = static_cast<u8>(rng.nextInt(0, 5));
    }

    // 更新表达性格
    updatePersonalityFromGenes();
}

void PandaEntity::updatePersonalityFromGenes()
{
    m_personality = calculateExpressedPersonality();
}

std::unique_ptr<AnimalEntity> PandaEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto baby = std::make_unique<PandaEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置
    baby->setPosition(x(), y(), z());

    // 先把父方世界挂给幼体，inheritGenesFromParents 需 m_world->getRandom()；
    // 否则 m_world==nullptr 时会提前返回，幼体保留构造时 randomizePersonality 的随机基因，
    // 基因遗传失效（对齐 MC 1.21.11 Panda.getBreedOffspring：create 传入 ServerLevel 使子代有世界）。
    baby->setWorld(world());

    // 遗传基因
    PandaEntity* parent = dynamic_cast<PandaEntity*>(&partner);
    baby->inheritGenesFromParents(this, parent);

    return baby;
}

void PandaEntity::tick()
{
    AnimalEntity::tick();

    // 担忧型熊猫在雷暴中会坐下（并停止进食）；雷暴结束后若未在进食则起身。
    // 其余动作（进食/打滚/躺下/打喷嚏）的状态机不受此影响。
    if (isWorried()) {
        if (world() != nullptr && world()->isThundering() && !isInWater()) {
            sit(true);
            m_eating = false;
        } else if (!isEating()) {
            sit(false);
        }
    }

    // 不满状态（对齐 vanilla Panda.tick:350-360）：盯住攻击目标、在第 29/14 tick
    // 播放"不能繁殖"音效，然后递减计时器。
    if (m_unhappyCounter > 0) {
        if (LivingEntity* target = attackTarget(); target != nullptr) {
            lookAt(*target, 90.0f, 90.0f);
        }
        if (m_unhappyCounter == 29 || m_unhappyCounter == 14) {
            playSound(SoundEvents::ENTITY_PANDA_CANT_BREED, 1.0f, 1.0f);
        }
        --m_unhappyCounter;
    }

    // 坐下时保持视线水平（vanilla Panda.tick 中 isSitting() 时 setXRot(0)）。
    if (isSitting()) {
        setRotation(yaw(), 0.0f);
    }

    // 更新打滚物理
    if (m_rolling) {
        _updateRoll();
    } else {
        m_rollTimer = 0;
    }

    // 更新打喷嚏计时器
    if (isSneezing() && m_sneezeTimer > 0) {
        m_sneezeTimer--;

        // 第1 tick播放预喷嚏音效
        if (m_sneezeTimer == 19) {
            playPreSneezeSound();
        }

        if (m_sneezeTimer <= 0) {
            setSneezing(false);
            // 打喷嚏完成，执行效果
            _onSneezeComplete();
        }
    }

    // 更新吃东西计时器
    if (m_eating && m_eatTimer > 0) {
        m_eatTimer--;
        if (m_eatTimer <= 0) {
            m_eating = false;
        }
    }

    // 更新躺着计时器
    if (m_lying && m_lyingTimer > 0) {
        m_lyingTimer--;
        if (m_lyingTimer <= 0) {
            m_lying = false;
        }
    }
}

void PandaEntity::registerGoals()
{
    // 调用父类方法注册基础目标（AgeableEntity 的 FollowParentGoal 等对幼体重要）。
    // 注意：AnimalEntity::registerGoals 不注册任何 goal（见 AnimalEntity.cpp:204-224 注释），
    // 每个动物子类须自己注册完整 AI 目标列表。此前 Panda 只注册了 PandaRollGoal，缺 BreedGoal
    // 等基础 goal，导致喂竹子 setInLove 后无 BreedGoal 驱动繁殖（繁殖链路断裂）。照搬 CowEntity
    // 范式补全基础动物 AI，并启用竹子 TemptGoal（原注释掉的）。
    AgeableEntity::registerGoals();

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 优先级 2: 繁殖（喂竹子进入爱心后 BreedGoal 驱动靠近配偶并 spawnBaby）。
    //   使用熊猫专属子类（找不到竹子时进入不满状态）。因该类是本地类，注册语句
    //   放在其定义之后（本函数末尾的目标选择器段落）。

    // 优先级 3: 近战攻击（好斗熊猫被攻击后反击）。对齐 vanilla Panda.registerGoals:264。
    //   vanilla PandaAttackGoal(this, 1.2F, true)：canUse = canPerformAction() && super.canUse()，
    //   攻击目标由 targetSelector 的 PandaHurtByTargetGoal 写入 attackTarget。MeleeAttackGoal
    //   负责寻路靠近与攻击（攻击力取 ATTACK_DAMAGE 属性，好斗熊猫=6.0）。
    //   此前好斗熊猫 ATTACK_DAMAGE=6.0 为死属性——无攻击 goal 驱动，被攻击不反击。
    //   注：vanilla TemptGoal 优先级为 4（此处为 3），GoalSelector 同 Move flag 互斥，按注册顺序
    //   Attack 先评估；功能等价于 vanilla 布局，未重排其余优先级以避免扰动既有测试。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::PandaAttackGoal>(this, 1.2, true));

    // 优先级 3: 食物诱惑（竹子）——对齐 vanilla Panda 持竹子吸引。scaredByMovement=false。
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                if (item == nullptr) return false;
                // Items::BAMBOO 可能在初始化期间为 nullptr
                if (Items::BAMBOO == nullptr) return false;
                return item == Items::BAMBOO;
            },
            false));

    // 优先级 4: 跟随父母（幼年熊猫）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 7: 坐下吃食物（对齐 vanilla Panda.registerGoals:281 的 PandaSitGoal，
    //   Flag.MOVE）。主手有食物或附近有可吃物品时坐下慢慢吃，结束时把食物丢回世界
    //   并进入冷却；懒散熊猫冷却更短。
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::PandaSitGoal>(this));

    // 优先级 9: 看向玩家（对齐 vanilla Panda.registerGoals:271-272，只看玩家、2% 概率）
    auto lookAtPlayer = std::make_unique<entity::ai::goal::PandaLookAtPlayerGoal>(this);
    m_lookAtPlayerGoal = lookAtPlayer.get();
    m_goalSelector.addGoal(9, std::move(lookAtPlayer));

    // 优先级 12: 随机看向（对齐 vanilla RandomLookAroundGoal；原为 7，为上面两个目标让位后顺延）
    m_goalSelector.addGoal(12, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 优先级 8: 打喷嚏（幼年熊猫随机打喷嚏，对齐 vanilla Panda.registerGoals:270）
    //   vanilla PandaSneezeGoal：isBaby && canPerformAction 时，isWeak 1/500 概率，否则 1/6000 概率，
    //   start 调 sneeze(true)。sneeze(true) 设 m_sneezeTimer=SNEEZE_DURATION，tick 递减驱动预喷嚏
    //   音效（timer==19，对齐 vanilla sneezeCounter==1）与 _onSneezeComplete（timer 到 0，对齐
    //   vanilla sneezeCounter>20 的 afterSneeze：掉粘液球+弱化周围实体+成年熊猫跳跃）。
    //   此前 _onSneezeComplete 已实现但无 goal 触发（死代码），本注册激活打喷嚏链路。
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::PandaSneezeGoal>(this));

    // 优先级 8: 仰躺（懒惰熊猫四脚朝天；对齐 vanilla PandaLieOnBackGoal）
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::PandaLieOnBackGoal>(this));

    // 优先级 6: 忧愁熊猫逃避玩家/敌对生物（对齐 vanilla PandaAvoidGoal 两条注册）
    m_goalSelector.addGoal(6,
        std::make_unique<entity::ai::goal::PandaAvoidGoal>(
            this, 8.0f, 2.0, 2.0, [](const LivingEntity* entity) -> bool {
                return dynamic_cast<const Player*>(entity) != nullptr;
            }));
    m_goalSelector.addGoal(6,
        std::make_unique<entity::ai::goal::PandaAvoidGoal>(
            this, 4.0f, 2.0, 2.0, [](const LivingEntity* entity) -> bool {
                return dynamic_cast<const MonsterEntity*>(entity) != nullptr;
            }));

    // 优先级 12: 打滚目标（顽皮熊猫或幼年熊猫）
    m_goalSelector.addGoal(12, std::make_unique<entity::ai::goal::PandaRollGoal>(this));

    // ========== targetSelector：攻击目标选取 ==========
    // 优先级 1: 被攻击后反击并呼叫同伴。对齐 vanilla Panda.registerGoals:277：
    //   targetSelector.addGoal(1, new Panda.PandaHurtByTargetGoal(this).setAlertOthers());
    //   - HurtByTargetGoal(this, alertAllies=true)：被攻击时取 getLastHurtBy() 作为反击目标，
    //     写入 attackTarget 供 PandaAttackGoal 消费。
    //   - setAlertOthers 谓词对齐 vanilla PandaHurtByTargetGoal.alertOther（Panda.java:856-860）：
    //       if (p_478126_ instanceof Panda && p_478126_.isAggressive()) p_478126_.setTarget(p_480501_);
    //     即只警醒好斗熊猫。alertOthers 已通过 entityType() 同类型过滤保证 instanceof Panda，
    //     谓词返回 !isAggressive() 排除非好斗熊猫（不警醒），等价于 vanilla 的 isAggressive() 门控。
    //   终止条件对齐 vanilla PandaHurtByTargetGoal.canContinueToUse（Panda.java:846-853）：
    //     if (!panda.gotBamboo && !panda.didBite) return super.canContinueToUse();
    //     else { panda.setTarget(null); return false; }
    //   gotBamboo 由"反击中收到食物"置位（Panda.mobInteract:615-618），didBite 由
    //   "非好斗熊猫咬到目标"置位（Panda.doHurtTarget:316-320），二者随目标清空归零。
    // 熊猫专用子类：反击中收到食物/咬中目标后立即放弃反击（vanilla PandaHurtByTargetGoal）。
    // 熊猫专属繁殖目标（对齐 vanilla Panda.PandaBreedGoal，Panda.java:786-829）：
    //   - canUse：基类条件不满足，或处于不满状态时拒绝；
    //     附近（3×8×8 的层叠扫描）找不到竹子方块时，若冷却已过则把不满计时器设为 32、
    //     冷却设为 600 tick，并让看向玩家目标锁定最近的玩家。
    //   - canFindBamboo：扫描蘑菇煲/草方块等之外唯一的熊猫食物来源——竹子方块。
    class PandaBreedGoal final : public entity::ai::goal::BreedGoal {
    public:
        PandaBreedGoal(PandaEntity* panda, f64 speed)
            : BreedGoal(panda, speed)
            , m_panda(panda)
        {}

        [[nodiscard]] bool shouldExecute() override
        {
            if (!BreedGoal::shouldExecute() || m_panda->unhappyCounter() != 0) {
                return false;
            }
            if (canFindBamboo()) {
                return true;
            }
            const i32 now = static_cast<i32>(m_panda->ticksExisted());
            if (m_unhappyCooldown <= now) {
                m_panda->setUnhappyCounter(32);
                m_unhappyCooldown = now + 600;
                if (!m_panda->isNoAi()) {
                    Player* player = EntityUtils::findClosestEntity<Player>(m_panda->world(),
                        m_panda->position(),
                        32.0f, // vanilla Panda.BREED_TARGETING 的追踪范围（同 FOLLOW_RANGE）
                        m_panda,
                        [](Player* candidate) -> bool {
                            return candidate != nullptr && candidate->isAlive() && !candidate->isSpectator();
                        });
                    if (m_panda->lookAtPlayerGoal() != nullptr) {
                        m_panda->lookAtPlayerGoal()->setLookTarget(player);
                    }
                }
            }
            return false;
        }

        /// 以自身方块位置为原点做 3×8×8 的层叠扫描寻找竹子（vanilla canFindBamboo）
        [[nodiscard]] bool canFindBamboo() const
        {
            IWorld* world = m_panda->world();
            if (world == nullptr) {
                return false;
            }
            const BlockPos origin = BlockPos(static_cast<i32>(std::floor(m_panda->x())),
                static_cast<i32>(std::floor(m_panda->y())),
                static_cast<i32>(std::floor(m_panda->z())));
            for (i32 i = 0; i < 3; ++i) {
                for (i32 j = 0; j < 8; ++j) {
                    // vanilla 用 k 在 [-j, j] 上按 0, 1, -1, 2, -2 ... 顺序逼近，这里等价覆盖全区间。
                    for (i32 k = -j; k <= j; ++k) {
                        for (i32 l = -j; l <= j; ++l) {
                            const BlockPos pos(origin.x + k, origin.y + i, origin.z + l);
                            const BlockState* state = world->getBlockState(pos);
                            if (state != nullptr && &state->getBlock() == VanillaBlocks::BAMBOO) {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

    private:
        PandaEntity* m_panda;
        i32 m_unhappyCooldown = 0;
    };

    class PandaHurtByTargetGoal final : public entity::ai::goal::HurtByTargetGoal {
    public:
        explicit PandaHurtByTargetGoal(PandaEntity* panda)
            // setTargetOnStart=true：本目标被放在目标选择器里，选中后需把攻击者写入
            // 熊猫的 attackTarget（vanilla TargetGoal.start → mob.setTarget(target)）。
            : HurtByTargetGoal(panda, true, TargetWriteMode::WriteOnStart)
            , m_panda(panda)
        {}

        [[nodiscard]] bool shouldContinueExecuting() override
        {
            if (!m_panda->gotBamboo() && !m_panda->didBite()) {
                return HurtByTargetGoal::shouldContinueExecuting();
            }
            m_panda->setAttackTarget(nullptr);
            return false;
        }

    private:
        PandaEntity* m_panda;
    };

    // 优先级 2: 繁殖。置于此处是因为 PandaBreedGoal 是本地类，须先定义后使用。
    m_goalSelector.addGoal(2, std::make_unique<PandaBreedGoal>(this, 1.0));

    auto hurtByTarget = std::make_unique<PandaHurtByTargetGoal>(this);
    hurtByTarget->setAlertOthers([](const LivingEntity* ally) -> bool {
        // 谓词返回 true = 不警醒该盟友。对齐 vanilla alertOther 的 isAggressive() 门控：
        // 非好斗熊猫不警醒。ally 经 entityType 过滤已是熊猫。
        const PandaEntity* pandaAlly = dynamic_cast<const PandaEntity*>(ally);
        if (pandaAlly == nullptr) {
            return true; // 非熊猫（理论不会出现，因 entityType 过滤）保守不警醒
        }
        return !pandaAlly->isAggressive();
    });
    m_targetSelector.addGoal(1, std::move(hurtByTarget));

    // 已实现的性格相关 goal：仰躺（懒惰，PandaLieOnBackGoal）、逃避玩家/敌对生物（忧愁，
    // PandaAvoidGoal）、打滚（顽皮/幼体，PandaRollGoal）、好斗反击（PandaAttackGoal +
    // PandaHurtByTargetGoal）、看到玩家（PandaLookAtPlayerGoal）、坐下吃食物（PandaSitGoal）。
}

void PandaEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 原版 Panda.createAttributes：攻击伤害无条件 6（好斗性格只额外加一次修饰符）。
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);

    // 熊猫的基础属性
    f32 maxHealth = 20.0f;

    // 虚弱熊猫生命值只有10
    if (isWeak()) {
        maxHealth = 10.0f;
    }

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, maxHealth);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.15); // 熊猫移动较慢

}

std::optional<ResourceLocation> PandaEntity::getAmbientSound() const
{
    // 根据性格返回不同音效
    if (isAggressive()) {
        return SoundEvents::ENTITY_PANDA_AGGRESSIVE_AMBIENT;
    }
    if (isWorried()) {
        return SoundEvents::ENTITY_PANDA_WORRIED_AMBIENT;
    }
    return SoundEvents::ENTITY_PANDA_AMBIENT;
}

std::optional<ResourceLocation> PandaEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_PANDA_HURT;
}

std::optional<ResourceLocation> PandaEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_PANDA_DEATH;
}

void PandaEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_PANDA_EAT, 1.0f, 1.0f);
}

void PandaEntity::sneeze(bool sneeze)
{
    // 对齐 vanilla Panda.sneeze:160：setFlag(2, sneeze)；!sneeze 时 setSneezeCounter(0)。
    // 状态位经 DATA_PANDA_FLAGS 同步，供客户端播放喷嚏动画。
    // Cubium 用标记位 + m_sneezeTimer 递减模型：开始时初始化 timer=SNEEZE_DURATION，
    // 由 tick() 递减驱动预喷嚏音效（timer==19）与 _onSneezeComplete（timer 到 0）。
    setSneezing(sneeze);
    if (sneeze) {
        m_sneezeTimer = SNEEZE_DURATION;
    } else {
        m_sneezeTimer = 0;
    }
}

bool PandaEntity::hurt(DamageSource& source, f32 amount)
{
    // 对齐 MC Java 1.21.11 Panda.hurtServer（Panda.java:543-546）：
    //   this.sit(false);
    //   return super.hurtServer(p_480184_, p_479799_, p_478083_);
    // 熊猫受击时无条件取消坐下状态（vanilla 不查 isInvulnerableTo，免疫也取消坐下），
    // 再走基类 hurt 处理实际伤害（基类内部查 isInvulnerableTo 门控）。
    sit(false);
    return AnimalEntity::hurt(source, amount);
}

bool PandaEntity::attackEntityAsMob(LivingEntity& target)
{
    // 对齐 vanilla Panda.doHurtTarget（Panda.java:316-320）：非好斗熊猫咬到目标时置位
    // didBite，使 PandaHurtByTargetGoal 在下一次 canContinueToUse 终止反击。
    if (!isAggressive()) {
        m_didBite = true;
    }
    return AnimalEntity::attackEntityAsMob(target);
}

void PandaEntity::playAttackSound(LivingEntity& target)
{
    // 对齐 vanilla Panda.playAttackSound：PANDA_BITE。
    (void)target;
    playBiteSound();
}

void PandaEntity::setAttackTarget(LivingEntity* target)
{
    // 对齐 vanilla Panda.tick:341-345：目标为空时清零 gotBamboo / didBite。
    if (target == nullptr) {
        m_gotBamboo = false;
        m_didBite = false;
    }
    AnimalEntity::setAttackTarget(target);
}

void PandaEntity::sit(bool sit)
{
    // vanilla Panda.sit 只写标志位；下坐动作（停止寻路、清除移动输入）由 tryToSit 负责，
    // 状态位本身由 AI（担忧雷暴 / 喂食 / PandaSitGoal）驱动。
    _setPandaFlag(PANDA_FLAG_SITTING, sit);
}

ActionResultType PandaEntity::interactMob(Player& player, Hand hand)
{
    ItemStack& stack = player.getHeldItem(hand);
    const bool isFood = isBreedingItem(stack);
    const bool wasInLove = isInLove();

    // 对齐 vanilla Panda.mobInteract:615-618：正在反击（有攻击目标）时收到食物，
    // 置 gotBamboo，使 PandaHurtByTargetGoal 下次 canContinueToUse 终止反击。
    if (isFood && attackTarget() != nullptr) {
        m_gotBamboo = true;
    }

    const ActionResultType result = AnimalEntity::interactMob(player, hand);

    // 食物已被 AnimalEntity 消费（进入求爱或加速成长）时不再追加坐下。
    if (result == ActionResultType::Success) {
        return result;
    }

    if (!isFood || isChild() || isInWater()) {
        return result;
    }

    // 成体喂食但未进入求爱：停下、坐下并进入进食状态。
    // wasInLove 为真说明正处求爱中，原版同样允许继续喂食并坐下进食。
    if (wasInLove || !canBreed()) {
        if (!player.abilities().creativeMode) {
            stack.shrink(1);
        }
        tryToSit();
        m_eating = true;
        return ActionResultType::Success;
    }
    return result;
}

void PandaEntity::tryToSit()
{
    // 对齐 vanilla Panda.tryToSit：水中不坐下，否则停止寻路并坐下。
    if (isInWater()) {
        return;
    }
    clearNavigation();
    sit(true);
}

void PandaEntity::playSneezeSound()
{
    playSound(SoundEvents::ENTITY_PANDA_SNEEZE, 1.0f, 1.0f);
}

void PandaEntity::playPreSneezeSound()
{
    playSound(SoundEvents::ENTITY_PANDA_PRE_SNEEZE, 1.0f, 1.0f);
}

void PandaEntity::playBiteSound()
{
    playSound(SoundEvents::ENTITY_PANDA_BITE, 1.0f, 1.0f);
}

void PandaEntity::_onSneezeComplete()
{
    if (m_world == nullptr) {
        return;
    }

    // 1. 播放喷嚏音效
    playSneezeSound();

    // 2. 生成喷嚏粒子
    // 粒子位置在熊猫头部前方
    f32 renderYawOffset = m_builtIn.rotation->m_rot.x; // 使用yaw作为朝向
    f32 yawRad = math::toRadians(renderYawOffset);
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    // 粒子位置：熊猫眼睛高度前方
    f32 particleX = static_cast<f32>(x()) - (width() + 1.0f) * 0.5f * sinYaw;
    f32 particleY = static_cast<f32>(y()) + eyeHeight() - 0.1f;
    f32 particleZ = static_cast<f32>(z()) + (width() + 1.0f) * 0.5f * cosYaw;

    // 使用熊猫当前运动速度作为粒子速度
    Vector3 vel = velocity();
    m_world->addParticle(
        particle::ParticleTypeId::Sneeze, Vector3(particleX, particleY, particleZ), Vector3(vel.x, 0.0f, vel.z));

    // 3. 让周围10格内的成年熊猫跳跃
    AxisAlignedBB searchBox = boundingBox().expand(10.0f, 10.0f, 10.0f);
    std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : nearbyEntities) {
        // 检查是否是熊猫
        auto* panda = dynamic_cast<PandaEntity*>(entity);
        if (panda != nullptr && !panda->isChild() && panda->onGround() && !panda->isInWater()) {
            // 成年熊猫跳起来
            panda->jump();
        }
    }

    // 4. 1/700概率掉落粘液球（需要游戏规则 doMobLoot）
    if (!m_world->isClientSide()) {
        const auto& gameRules = m_world->getGameRules();
        if (gameRules.getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT)) {
            math::IRandom& rng = getRandom();
            if (rng.nextInt(700) == 0) {
                // 使用 ItemDropHelper 在熊猫位置掉落粘液球
                ItemStack slimeBall(*Items::SLIME_BALL, 1);
                ItemDropHelper::spawnItemAtEntity(this, slimeBall, 0.5f, rng);
            }
        }
    }
}

void PandaEntity::_updateRoll()
{
    // 处理打滚物理
    m_rollTimer++;

    if (m_rollTimer > ROLL_DURATION) {
        // 打滚超过 32 ticks 后结束
        m_rolling = false;
        m_rollTimer = 0;
        return;
    }

    if (m_world != nullptr && !m_world->isClientSide()) {
        Vector3 vel = velocity();

        if (m_rollTimer == 1) {
            // 第1帧：初始化打滚方向和初速度
            f32 yawRad = math::toRadians(yaw());
            f32 speed = isChild() ? ROLL_SPEED_CHILD : ROLL_SPEED_ADULT;
            f32 sinYaw = std::sin(yawRad);
            f32 cosYaw = std::cos(yawRad);

            m_rollVelocity = Vector3(vel.x + (-sinYaw * speed), 0.0, vel.z + (cosYaw * speed));

            // 设置初始速度（包含跳跃）
            setVelocity(m_rollVelocity.x, ROLL_JUMP_VELOCITY, m_rollVelocity.z);
        } else if (m_rollTimer == 7 || m_rollTimer == 15 || m_rollTimer == 23) {
            // 第7、15、23帧：执行小跳
            f32 jumpVel = onGround() ? ROLL_JUMP_VELOCITY : static_cast<f32>(vel.y);
            setVelocity(0.0, jumpVel, 0.0);
        } else {
            // 其他帧：维持水平移动
            setVelocity(m_rollVelocity.x, vel.y, m_rollVelocity.z);
        }
    }
}

} // namespace mc
