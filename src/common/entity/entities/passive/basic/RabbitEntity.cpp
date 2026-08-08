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

#include "RabbitEntity.hpp"

#include <cmath>
#include <memory>

#include "common/core/Types.hpp"
#include "common/entity/ai/controller/RabbitJumpControl.hpp"
#include "common/entity/ai/controller/RabbitMoveControl.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/RaidGardenGoal.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

namespace mc {

// 本文件收敛了原 RabbitEntity 的 TODO：实现了兔子专属的 RabbitJumpControl /
// RabbitMoveControl 控制器、jumpDelayTicks/wasOnGround 着陆延迟状态机、
// updateAITasks()（对应 MC customServerAiStep）以及杀手兔变种的属性与 AI 目标切换。
// RaidGardenGoal（偷胡萝卜）+ CarrotBlock 年龄递减逻辑已实现，详见
// ai/goal/goals/special/RaidGardenGoal.{hpp,cpp}。
//
// 仍保留为 TODO 的项目：
// - getJumpPower() 重写：MC 中 Rabbit 重写了 getJumpPower() 根据移动速度和路径
//   调整跳跃高度（0.2/0.3/0.5）。项目当前 LivingEntity::jump() 为非虚函数且使用
//   m_jumpUpwardsMotion，重写跳跃力度需要更大的架构改动，暂留待未来处理。

RabbitEntity::RabbitEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 替换为兔子专属的跳跃/移动控制器（对应 MC Rabbit 构造函数中：
    //   this.jumpControl = new RabbitJumpControl(this);
    //   this.moveControl = new RabbitMoveControl(this);
    //   this.setSpeedModifier(0.0);
    // ）
    // 必须在 registerGoals()/registerAttributes() 之前完成，因为 AI 目标和
    // customServerAiStep（updateAITasks）会通过 MobEntity::jumpController()/
    // moveController() 访问这些控制器。
    m_jumpController = std::make_unique<entity::ai::controller::RabbitJumpControl>(this);
    m_moveController = std::make_unique<entity::ai::controller::RabbitMoveControl>(this);

    // 注册属性（必须在 setRabbitType 之前，因为 applyRabbitType 需要访问 ATTACK_DAMAGE）
    registerAttributes();

    // 注册 AI 目标
    registerGoals();

    // 设置皮肤类型（内部会调用 setRabbitType -> applyRabbitType）
    // 必须在 registerAttributes() 之后，因为 killer 变种需要修改 ATTACK_DAMAGE
    setRandomRabbitType();

    // 对应 MC Rabbit 构造函数末尾的 setSpeedModifier(0.0)
    if (auto* nav = navigator(); nav != nullptr) {
        nav->setSpeed(0.0);
    }
}

std::unique_ptr<Entity> RabbitEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<RabbitEntity>(0, registry);
}

void RabbitEntity::setRandomRabbitType()
{
    // 根据当前群系确定兔子类型
    // 参考 MC 1.21.11 Rabbit.getRandomRabbitVariant：杀手兔不再自然生成，
    // 自然生成的兔子类型完全由群系决定
    setRabbitType(getDefaultRabbitTypeForBiome());
}

void RabbitEntity::setRabbitType(RabbitType type)
{
    // 对应 MC 1.21.11 Rabbit.setVariant()：先应用变种特定属性和 AI 目标，
    // 再更新 m_rabbitType 字段
    applyRabbitType(type);
    m_rabbitType = type;
}

void RabbitEntity::applyRabbitType(RabbitType newType)
{
    // 对应 MC 1.21.11 Rabbit.setVariant()：
    //   if (variant == EVIL) {
    //       getAttribute(ARMOR).setBaseValue(8.0);
    //       goalSelector.addGoal(4, new MeleeAttackGoal(this, 1.4, true));
    //       targetSelector.addGoal(1, new HurtByTargetGoal(this).setAlertOthers());
    //       targetSelector.addGoal(2, new NearestAttackableTargetGoal<>(this, Player.class, true));
    //       targetSelector.addGoal(2, new NearestAttackableTargetGoal<>(this, Wolf.class, true));
    //       getAttribute(ATTACK_DAMAGE).addOrUpdateTransientModifier(
    //           new AttributeModifier(EVIL_ATTACK_POWER_MODIFIER, 5.0, ADD_VALUE));
    //       if (!hasCustomName()) setCustomName(...killer_bunny...);
    //   } else {
    //       getAttribute(ATTACK_DAMAGE).removeModifier(EVIL_ATTACK_POWER_MODIFIER);
    //   }
    if (newType == RabbitType::Killer) {
        // 杀手兔护甲值 = 8
        attributes().setBaseValue(entity::attribute::Attributes::ARMOR, 8.0);

        // 注册近战攻击目标（速度 1.4，使用长期记忆）
        // 对应 MC goalSelector.addGoal(4, new MeleeAttackGoal(this, 1.4, true))
        m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.4, true));

        // 被攻击后反击，并警醒盟友（此处盟友为其他兔子，但 MC 原版 alertOthers
        // 会通知同类的 HurtByTargetGoal。项目实现中 setAlertOthers 的谓词决定哪些
        // 盟友被警醒，这里使用默认空谓词表示警醒所有同类）
        m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));

        // 主动攻击玩家
        m_targetSelector.addGoal(
            2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));

        // 主动攻击狼
        m_targetSelector.addGoal(
            2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<WolfEntity>>(this, true));

        // ATTACK_DAMAGE +5 修改器（对应 MC EVIL_ATTACK_POWER_MODIFIER）
        // 使用 addOrUpdateTransientModifier 语义：先移除同 ID 修改器再添加
        if (auto* inst = attributes().getInstance(entity::attribute::Attributes::ATTACK_DAMAGE); inst != nullptr) {
            inst->removeModifier(EVIL_ATTACK_POWER_MODIFIER_ID);
            inst->addModifier(entity::attribute::AttributeModifier(EVIL_ATTACK_POWER_MODIFIER_ID,
                "Killer rabbit attack power boost",
                5.0,
                entity::attribute::Operation::Addition));
        }
    } else {
        // 非杀手兔变种：移除 EVIL_ATTACK_POWER_MODIFIER（如果存在）
        if (auto* inst = attributes().getInstance(entity::attribute::Attributes::ATTACK_DAMAGE); inst != nullptr) {
            inst->removeModifier(EVIL_ATTACK_POWER_MODIFIER_ID);
        }
    }
}

RabbitEntity::RabbitType RabbitEntity::getDefaultRabbitTypeForBiome() const
{
    // 获取当前位置的群系
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return RabbitType::Brown;
    }

    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    const ChunkData* chunk = worldPtr->getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return RabbitType::Brown;
    }

    BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());

    // 参考 MC 1.21.11 Rabbit.getRandomRabbitVariant：
    // 使用一次随机调用来决定类型，与 MC 的随机种子消费方式一致
    math::Random& rng = getRandom();
    i32 i = rng.nextInt(100);

    // 雪地群系：生成白色/白色斑点兔子
    // 参考 MC BiomeTags.SPAWNS_WHITE_RABBITS
    if (biomeId == Biomes::SnowyPlains || biomeId == Biomes::SnowyMountains || biomeId == Biomes::IceSpikes ||
        biomeId == Biomes::FrozenOcean || biomeId == Biomes::DeepFrozenOcean || biomeId == Biomes::FrozenRiver ||
        biomeId == Biomes::SnowyBeach || biomeId == Biomes::SnowyTaiga || biomeId == Biomes::SnowyTaigaHills ||
        biomeId == Biomes::SnowyTaigaMountains || biomeId == Biomes::FrozenPeaks || biomeId == Biomes::JaggedPeaks ||
        biomeId == Biomes::SnowySlopes || biomeId == Biomes::Grove) {
        return i < 80 ? RabbitType::White : RabbitType::WhiteSpotted;
    }

    // 沙漠群系：生成金色兔子
    // 参考 MC BiomeTags.SPAWNS_GOLD_RABBITS
    if (biomeId == Biomes::Desert || biomeId == Biomes::DesertHills || biomeId == Biomes::DesertLakes) {
        return RabbitType::Gold;
    }

    // 其他群系：棕色/椒盐色/黑色
    if (i < 50) {
        return RabbitType::Brown;
    }
    if (i < 90) {
        return RabbitType::SaltAndPepper;
    }
    return RabbitType::Black;
}

bool RabbitEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 兔子用胡萝卜、金胡萝卜、蒲公英繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;

    // 检查胡萝卜和金胡萝卜
    if (item == Items::CARROT || item == Items::GOLDEN_CARROT) {
        return true;
    }

    // 检查蒲公英（方块物品）
    // DANDELION 是方块，需要通过 BlockItemRegistry 获取对应的物品
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    if (block != nullptr && block == VanillaBlocks::DANDELION) {
        return true;
    }

    return false;
}

std::unique_ptr<AnimalEntity> RabbitEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = m_world->entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto baby = std::make_unique<RabbitEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 类型继承逻辑：5% 概率根据群系随机生成类型，95% 从父母继承
    // 参考 MC 1.21.11 Rabbit.getBreedOffspring
    math::Random& rng = getRandom();
    RabbitType babyType;

    if (rng.nextInt(20) == 0) {
        // 5% 概率：根据父母所在位置的群系生成类型
        // 注意：此时 baby 尚未设置 world，因此使用父级的位置和群系
        babyType = getDefaultRabbitTypeForBiome();
    } else {
        // 95% 概率：从父母继承
        // 50% 概率继承自己，50% 概率继承配偶
        if (rng.nextBoolean()) {
            babyType = m_rabbitType;
        } else {
            // 尝试从配偶获取类型
            RabbitEntity* partnerRabbit = dynamic_cast<RabbitEntity*>(&partner);
            if (partnerRabbit != nullptr) {
                babyType = partnerRabbit->getRabbitType();
            } else {
                babyType = m_rabbitType;
            }
        }
    }
    baby->setRabbitType(babyType);

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

void RabbitEntity::setJumping(bool jumping)
{
    // 对应 MC 1.21.11 Rabbit.setJumping()：
    //   super.setJumping(jumping);
    //   if (jumping) { playSound(getJumpSound(), getSoundVolume(), ...); }
    LivingEntity::setJumping(jumping);

    if (!jumping) {
        return;
    }

    // 播放跳跃音效（对应 MC Rabbit.getJumpSound() = SoundEvents.RABBIT_JUMP）
    auto soundEvent = makeSoundEventId("jump");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random& random = getRandom();
    playSound(*soundEvent, getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
}

void RabbitEntity::startJumping()
{
    // 对应 MC 1.21.11 Rabbit.startJumping()：
    //   setJumping(true); jumpDuration = 10; jumpTicks = 0;
    //
    // 幂等保护：若动画已在进行中（m_rabbitJumpDuration != 0），则跳过重置，
    // 避免反复归零导致动画永远无法推进，也避免向客户端重复广播 RabbitJump 状态码。
    if (m_rabbitJumpDuration != 0) {
        return;
    }

    // 先设置 LivingEntity 的跳跃标志（会触发 jump 音效播放）
    // 注意：此处调用本类重写的 setJumping(true)，它会先调用基类再播音效
    setJumping(true);

    m_rabbitJumpDuration = 10;
    m_rabbitJumpTicks = 0;

    // 对应 MC 1.21.11 Rabbit.jumpFromGround() 中的 broadcastEntityEvent(this, (byte)1)
    // 项目架构下 LivingEntity::jump() 非虚函数无法重写，故在动画启动时即广播，
    // 让客户端同步启动 jumpDuration 计时器以计算 jumpRotation。
    // 注意：MC 中广播发生在 jumpFromGround() 内（物理跳跃时刻），此处略早一个 tick，
    // 但客户端位置插值会平滑过渡，视觉上无差异。
    if (auto* worldPtr = world(); worldPtr != nullptr) {
        worldPtr->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::RabbitJump));
    }
}

f32 RabbitEntity::getJumpCompletion(f32 partialTick) const
{
    // 对应 MC 1.21.11 Rabbit.getJumpCompletion(float)：
    //   jumpDuration == 0 ? 0.0F : (jumpTicks + partialTick) / jumpDuration
    if (m_rabbitJumpDuration == 0) {
        return 0.0f;
    }
    return (static_cast<f32>(m_rabbitJumpTicks) + partialTick) / static_cast<f32>(m_rabbitJumpDuration);
}

void RabbitEntity::aiStep()
{
    // 先调用父类 aiStep()，处理物理移动、自动跳跃等基础逻辑
    // 注意：父类 aiStep() 中可能在 m_isJumping && onGround && m_jumpTicks==0 时调用 jump()，
    // jump() 仅设置垂直速度，不会触发跳跃动画状态机；动画由 setJumping(true) 路径启动。
    LivingEntity::aiStep();

    // 对应 MC 1.21.11 Rabbit.aiStep()：
    //   if (this.jumpTicks != this.jumpDuration) { this.jumpTicks++; }
    //   else if (this.jumpDuration != 0) {
    //       this.jumpTicks = 0; this.jumpDuration = 0; this.setJumping(false);
    //   }
    if (m_rabbitJumpTicks != m_rabbitJumpDuration) {
        ++m_rabbitJumpTicks;
    } else if (m_rabbitJumpDuration != 0) {
        m_rabbitJumpTicks = 0;
        m_rabbitJumpDuration = 0;
        LivingEntity::setJumping(false); // 直接调用基类避免再次播音效/广播
    }
}

void RabbitEntity::updateAITasks()
{
    // 对应 MC 1.21.11 Rabbit.customServerAiStep(ServerLevel)：
    //   if (jumpDelayTicks > 0) jumpDelayTicks--;
    //   if (moreCarrotTicks > 0) moreCarrotTicks -= random.nextInt(3);
    //   if (onGround()) {
    //       if (!wasOnGround) { setJumping(false); checkLandingDelay(); }
    //       if (variant == EVIL && jumpDelayTicks == 0) { ...killer rabbit attack... }
    //       if (!jumpControl.wantJump()) {
    //           if (moveControl.hasWanted() && jumpDelayTicks == 0) {
    //               ...face direction and startJumping...
    //           }
    //       } else if (!jumpControl.canJump()) { enableJumpControl(); }
    //   }
    //   wasOnGround = onGround();

    // 1. 递减着陆延迟
    if (m_jumpDelayTicks > 0) {
        --m_jumpDelayTicks;
    }

    // 2. 随机递减 moreCarrotTicks（对应 MC moreCarrotTicks -= random.nextInt(3)）
    if (m_moreCarrotTicks > 0) {
        math::Random& rng = getRandom();
        m_moreCarrotTicks -= rng.nextInt(3);
        if (m_moreCarrotTicks < 0) {
            m_moreCarrotTicks = 0;
        }
    }

    // 3. 着陆检测与跳跃控制（仅在地面时执行）
    if (onGround()) {
        // 着陆瞬间：从空中到地面的过渡
        if (!m_wasOnGround) {
            // 对应 MC：setJumping(false); checkLandingDelay();
            // 直接调用基类 setJumping(false) 避免触发本类的音效逻辑
            LivingEntity::setJumping(false);
            checkLandingDelay();
        }

        // 杀手兔的主动跳跃攻击
        // 对应 MC：if (variant == EVIL && jumpDelayTicks == 0) {
        //     LivingEntity target = getTarget();
        //     if (target != null && distanceToSqr(target) < 16.0) {
        //         facePoint(target.x, target.z);
        //         moveControl.setWantedPosition(target.x, target.y, target.z, moveControl.getSpeedModifier());
        //         startJumping();
        //         wasOnGround = true;
        //     }
        // }
        if (isKillerRabbit() && m_jumpDelayTicks == 0) {
            LivingEntity* target = attackTarget();
            if (target != nullptr) {
                // distanceToSqr < 16.0 (4 格距离平方)
                f32 distSq = distanceSqTo(*target);
                if (distSq < 16.0f) {
                    facePoint(target->x(), target->z());
                    if (auto* moveCtrl = moveController(); moveCtrl != nullptr) {
                        moveCtrl->setMoveTo(target->x(), target->y(), target->z(), moveCtrl->speed());
                    }
                    startJumping();
                    m_wasOnGround = true;
                    // 跳过后续的普通跳跃逻辑（杀手兔已主动跳跃）
                    m_wasOnGround = onGround();
                    return;
                }
            }
        }

        // 普通跳跃逻辑
        // 对应 MC：RabbitJumpControl rabbitJumpCtrl = (RabbitJumpControl)jumpControl;
        //   if (!rabbitJumpCtrl.wantJump()) {
        //       if (moveControl.hasWanted() && jumpDelayTicks == 0) {
        //           Path path = navigation.getPath();
        //           Vec3 vec3 = new Vec3(moveControl.getWantedX(), ...);
        //           if (path != null && !path.isDone()) vec3 = path.getNextEntityPos(this);
        //           facePoint(vec3.x, vec3.z);
        //           startJumping();
        //       }
        //   } else if (!rabbitJumpCtrl.canJump()) { enableJumpControl(); }
        auto* jumpCtrl = jumpController();
        auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
        if (rabbitJumpCtrl != nullptr && !rabbitJumpCtrl->wantJump()) {
            // 检查是否有移动目标且着陆延迟已结束
            auto* moveCtrl = moveController();
            if (moveCtrl != nullptr && moveCtrl->isUpdating() && m_jumpDelayTicks == 0) {
                // 获取目标位置：优先使用路径的下一节点，否则使用移动控制器的目标
                f64 targetX = moveCtrl->getX();
                f64 targetZ = moveCtrl->getZ();

                auto* nav = navigator();
                if (nav != nullptr) {
                    const auto* path = nav->getPath();
                    if (path != nullptr && !path->isFinished()) {
                        // 对应 MC path.getNextEntityPos(this)
                        Vector3d nextPos = path->getPosition(this);
                        targetX = nextPos.x;
                        targetZ = nextPos.z;
                    }
                }

                facePoint(targetX, targetZ);
                startJumping();
            }
        } else if (rabbitJumpCtrl != nullptr && !rabbitJumpCtrl->canJump()) {
            // 跳跃控制器被禁用（着陆延迟期间），重新启用
            enableJumpControl();
        }
    }

    // 4. 更新 wasOnGround
    m_wasOnGround = onGround();
}

void RabbitEntity::enableJumpControl()
{
    // 对应 MC Rabbit.enableJumpControl()：
    //   ((RabbitJumpControl)jumpControl).setCanJump(true);
    auto* jumpCtrl = jumpController();
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    if (rabbitJumpCtrl != nullptr) {
        rabbitJumpCtrl->setCanJump(true);
    }
}

void RabbitEntity::disableJumpControl()
{
    // 对应 MC Rabbit.disableJumpControl()：
    //   ((RabbitJumpControl)jumpControl).setCanJump(false);
    auto* jumpCtrl = jumpController();
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    if (rabbitJumpCtrl != nullptr) {
        rabbitJumpCtrl->setCanJump(false);
    }
}

void RabbitEntity::setLandingDelay()
{
    // 对应 MC Rabbit.setLandingDelay()：
    //   if (moveControl.getSpeedModifier() < 2.2) jumpDelayTicks = 10;
    //   else jumpDelayTicks = 1;
    f64 speedModifier = 0.0;
    if (auto* moveCtrl = moveController(); moveCtrl != nullptr) {
        speedModifier = moveCtrl->speed();
    }
    if (speedModifier < 2.2) {
        m_jumpDelayTicks = 10;
    } else {
        m_jumpDelayTicks = 1;
    }
}

void RabbitEntity::checkLandingDelay()
{
    // 对应 MC Rabbit.checkLandingDelay()：
    //   setLandingDelay(); disableJumpControl();
    setLandingDelay();
    disableJumpControl();
}

void RabbitEntity::facePoint(f64 targetX, f64 targetZ)
{
    // 对应 MC Rabbit.facePoint(double, double)：
    //   setYRot((float)(Mth.atan2(targetZ - getZ(), targetX - getX()) * 180.0 / PI) - 90.0F);
    f32 targetYaw = static_cast<f32>(std::atan2(targetZ - z(), targetX - x()) * math::RAD_TO_DEG - 90.0);
    setYaw(math::wrapDegrees(targetYaw));
}

sound::SoundCategory RabbitEntity::getSoundCategory() const
{
    return isKillerRabbit() ? sound::SoundCategory::Hostile : sound::SoundCategory::Neutral;
}

void RabbitEntity::playAttackSound(LivingEntity& /*target*/)
{
    if (!isKillerRabbit()) {
        return;
    }

    auto soundEvent = makeSoundEventId("attack");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random& random = getRandom();
    playSound(*soundEvent, 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
}

void RabbitEntity::registerGoals()
{
    // 调用父类方法
    AgeableEntity::registerGoals();

    // 兔子有特殊的 AI 行为（逃跑更快）
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（兔子逃跑速度更快）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 2.2));

    // 优先级 2: 逃离玩家（8格，速度2.2）- 杀手兔不逃离
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            8.0f, // avoidDistance - 检测玩家的距离
            2.2,  // farSpeed - 远距离逃跑速度
            2.2,  // nearSpeed - 近距离逃跑速度
            [this](const LivingEntity* entity) -> bool {
                // 杀手兔不逃离
                if (isKillerRabbit()) return false;
                // 检查是否是玩家
                return dynamic_cast<const Player*>(entity) != nullptr;
            }));

    // 优先级 2: 逃离狼（10格，速度2.2）- 杀手兔不逃离
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            10.0f, // avoidDistance - 检测狼的距离
            2.2,   // farSpeed
            2.2,   // nearSpeed
            [this](const LivingEntity* entity) -> bool {
                if (isKillerRabbit()) return false;
                return entity->entityType() == entity::VanillaEntityTypeKeys::WOLF;
            }));

    // 优先级 2: 逃离怪物（4格，速度2.2）- 杀手兔不逃离
    m_goalSelector.addGoal(2,
        new entity::ai::goal::AvoidEntityGoal(this,
            4.0f, // avoidDistance - 检测怪物的距离
            2.2,  // farSpeed
            2.2,  // nearSpeed
            [this](const LivingEntity* entity) -> bool {
                if (isKillerRabbit()) return false;
                // 检查是否是敌对生物
                return dynamic_cast<const MonsterEntity*>(entity) != nullptr;
            }));

    // 优先级 3: 繁殖
    m_goalSelector.addGoal(3, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 4: 食物诱惑（胡萝卜、金胡萝卜、蒲公英）
    m_goalSelector.addGoal(4,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                if (item == nullptr) return false;

                // 胡萝卜和金胡萝卜
                if (item == Items::CARROT || item == Items::GOLDEN_CARROT) {
                    return true;
                }

                // 蒲公英（方块物品）
                const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
                if (block != nullptr && block == VanillaBlocks::DANDELION) {
                    return true;
                }

                return false;
            },
            false)); // scaredByMovement = false

    // 优先级 5: 偷胡萝卜（对应 MC 1.21.11 Rabbit.RaidGardenGoal）
    // 兔子饥饿（moreCarrotTicks<=0）时寻找成熟胡萝卜并啃食，受 MOB_GRIEFING 规则限制。
    // 与 FollowParentGoal 共用优先级 5，二者均占用 Move+Jump 标志，GoalSelector 保证互斥。
    m_goalSelector.addGoal(5, new entity::ai::goal::RaidGardenGoal(this));

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 6: 随机漫步
    m_goalSelector.addGoal(6, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));
}

void RabbitEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 对应 MC 1.21.11 Rabbit.createAttributes()：
    //   Animal.createAnimalAttributes()
    //       .add(MAX_HEALTH, 3.0).add(MOVEMENT_SPEED, 0.3F).add(ATTACK_DAMAGE, 3.0)
    // 兔子需要 ATTACK_DAMAGE 属性以支持杀手兔变种的攻击（+5 修改器）。
    // AnimalEntity 基类不注册 ATTACK_DAMAGE（仅 MonsterEntity 注册），此处显式注册。
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    // 基础攻击伤害 3.0（对应 MC DEFAULT_ATTACK_POWER = 3）
    // 杀手兔变种在 applyRabbitType() 中添加 +5 修改器
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
}

} // namespace mc
