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

#pragma once

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "../../item/core/ActionResult.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../sound/SoundCategory.hpp"
#include "../../util/AxisAlignedBB.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "../../util/text/ITextComponent.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../entities/projectile/ProjectileDeflection.hpp"
#include "EntityDataManager.hpp"
#include "EntityFlags.hpp"
#include "EntityPose.hpp"
#include "EntitySize.hpp"
#include "MoverType.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/ecs/components/BuiltInEntityComponents.hpp"
#include "common/entity/ecs/components/EntityFlagsComponent.hpp"
#include "common/entity/ecs/components/EntityStateComponent.hpp"
#include "common/entity/ecs/components/EnvironmentStateComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"
#include "common/entity/ecs/components/FreezeComponent.hpp"
#include "common/entity/ecs/components/PhysicsStateComponent.hpp"
#include "common/entity/ecs/components/PortalComponent.hpp"
#include "common/entity/ecs/context/EntityContext.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class PhysicsEngine;
class IWorld;
class EntityManager;
class BlockState;
class DamageSource;

namespace entity {

// 前向声明：EntityType 完整定义在 EntityType.hpp，Entity 仅持有其 const 指针
// （m_entityType 缓存）并按名查询，无需在此引入完整定义。
class EntityType;

} // namespace entity

namespace scoreboard {
class Team;
} // namespace scoreboard

namespace world::explosion {

// 前向声明：Entity::ignoreExplosion 入参，仅持有裸指针，无需完整定义
struct ExplosionImmunityContext;

} // namespace world::explosion

/**
 * @brief 实体推动反应类型
 *
 * 定义实体被活塞等推动时的反应行为。
 */
enum class PushReaction : u8 {
    Normal,  // 正常推动
    Destroy, // 被推动时销毁
    Ignore   // 忽略推动
};

// 引入 mc::entity::EntityPose 到 mc 命名空间以保持兼容
using EntityPose = entity::EntityPose;

// ============================================================================
// 实体标志位
// ============================================================================
// EntityFlags 枚举及位运算符已提取到独立头 EntityFlags.hpp（第四批 ECS 迁移），
// 供 EntityFlagsComponent 以值类型承载而不循环依赖本文件。参照 EquipmentSlot 提取先例。

// ============================================================================
// 实体基类
// ============================================================================

/**
 * @brief 实体基类
 *
 * 所有游戏实体（玩家、生物、物品等）的基类。
 * 提供位置、速度、旋转等基本属性，以及碰撞检测和物理模拟支持。
 *
 * 注意：
 * - 位置使用 Vector3 存储，但组件是 f64 精度
 * - 碰撞箱使用 AxisAlignedBB（f32 精度）
 * - 子类应重写 width()、height()、eyeHeight() 方法
 *
 * 数据参数（通过 EntityDataManager 同步）：
 * - FLAGS: 实体标志（燃烧、潜行等）
 * - AIR: 空气值
 * - CUSTOM_NAME: 自定义名称
 * - CUSTOM_NAME_VISIBLE: 名称可见性
 * - SILENT: 静音标志
 * - NO_GRAVITY: 无重力标志
 * - POSE: 姿态
 */
class Entity {
public:
    // ========== 静态数据参数 ==========
    // 子类应定义自己的数据参数

    /**
     * @brief 构造函数
     *
     * @param id 实体ID
     * @param world 世界指针（可为 nullptr，测试/反序列化场景）
     * @param registry ECS 实体注册表引用。构造时立即在此 registry 内 create 出 ECS
     *   实体并 attach 4 个高频组件（StateVector/Velocity/AABBShape/EntityRotation），
     *   缓存裸指针到 m_builtIn，使后续 position()/velocity() 等 getter 直接解引用。
     *
     *   【为何构造时即 attach】entt 的实体与组件绑定在创建它的 registry 上、不可跨
     *   registry 迁移，故构造时 registry 必须就位（对齐基岩版 Actor(ILevel&,
     *   EntityContext&) 构造签名透传 EntityContext 的设计）。首批所有实体类型在构造
     *   时挂同一套基础组件，差异化组件由后续批次在工厂 attachAll 中补充。
     */
    Entity(EntityInstanceId id, IWorld* world, ecs::EntityRegistry& registry);
    // 析构时销毁 ECS 实体（entt entity + 其全部组件）。
    // 【为何必须 destroy】EntityOwnerComponent 经 self-attach 持本 Entity 的非拥有裸指针，
    // 供 fireTick/portalTick 等经 view<..., EntityOwnerComponent> 反查 OOP 句柄。
    // 若析构不 destroy entt 实体，残留 entt 条目的 EntityOwnerComponent 将持悬垂 Entity*，
    // 系统遍历到时 isRemoved()/hurt() 等解引用即 UAF。destroy 后系统 view 不再遍历到本实体。
    // 安全性：m_entityContext 是非拥有视图，Entity 析构后无人应再经它访问（已建立契约）；
    // 成员析构顺序保证 ~Entity body 先执行（此时 m_entityContext 仍存活可取 entity id），
    // 随后才析构 m_entityContext 本身。
    virtual ~Entity();

    // 禁止拷贝
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // 禁止移动（因为 EntityDataManager 包含 std::mutex）
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 注册数据参数
     *
     * 子类应重写此方法来注册自己的数据参数。
     * 在构造函数中调用。
     */
    virtual void registerData();

    // ========== 基本属性 ==========

    [[nodiscard]] EntityInstanceId id() const { return m_id; }
    /**
     * @brief 设置实体ID
     *
     * 仅由EntityManager在分配ID时调用。
     * 不应该在其他地方使用。
     */
    void setId(EntityInstanceId id) { m_id = id; }
    [[nodiscard]] const std::string& uuid() const { return m_uuid; }
    void setUuid(const std::string& uuid) { m_uuid = uuid; }

    /**
     * @brief 获取实体类型标识符
     * @return 实体类型字符串（用于网络同步和渲染）
     */
    [[nodiscard]] virtual std::string getTypeId() const;

    /**
     * @brief 取本实体在 Java `minecraft:entity_type` 注册表中的 vanilla 1.21.11 id
     *
     * 用于 AddEntity（clientbound/minecraft:add_entity）的 entityTypeId 字段。entity_type
     * 注册表不在 Configuration 同步的 23 个注册表内，真 Java 客户端用内置 vanilla core 包
     * 注册表 id 解析，故服务端必须发 vanilla id（ EntityType.java register 顺序，字母序 157 条），
     * 而非项目内部 EntityType::id()（VanillaEntities.cpp registerType 顺序）。
     *
     * 默认实现按 entityType()->name() 查 JavaEntityTypeIdMap。船类（BoatEntity/ChestBoatEntity）
     * override：vanilla 无泛型 boat/chest_boat，按内部木种枚举拼出对应木种变体名
     * （oak_boat/mangrove_chest_boat/bamboo_raft 等）再查表。
     *
     * @return vanilla entity_type registry id；查不到返回 0（acacia_boat 兜底）
     */
    [[nodiscard]] virtual u32 getJavaEntityTypeId() const;

    /**
     * @brief 设置实体类型标识符
     *
     * 当实体由注册表工厂创建时，调用方应传入注册名（如 minecraft:pig）。
     * 同时失效缓存的 EntityType 指针，由 entityType() 在首次访问时懒查询。
     *
     * @note 存在 20+ 处在注册表就绪前调用本方法的路径（如 WitherEntity 构造期
     *       对 skull 子实体 setTypeId），故不在此立即查表，而由 entityType() 懒查询。
     */
    void setTypeId(const std::string& typeId)
    {
        m_typeId = typeId;
        m_entityType = nullptr; // 失效缓存，由 entityType() 懒查询重建
    }

    /**
     * @brief 获取实体类型的运行时指针（懒查询）
     *
     * @return 指向 EntityRegistry 内部 EntityType 对象的 const 指针；若 m_typeId
     *         为空或注册表未注册该类型，返回 nullptr。
     *
     * 指针稳定性：返回值指向 EntityRegistry::m_types（std::deque）内的对象，
     * 与 VanillaEntityTypeKeys::* 指针别名同源，在注册表未被 clear() 前地址稳定，
     * 可安全用于指针比较（热路径比 operator== 字符串比较更快）。
     *
     * 懒查询：首次调用时按 m_typeId 查注册表并缓存，后续直接返回缓存。应对
     * "先 setTypeId 后 registerAll" 的初始化顺序——此时首次查询可能返回 nullptr，
     * 待注册表就绪后下次查询会重新填充。
     *
     * 类型判等推荐用法：
     * @code
     * if (entity->entityType() == entity::VanillaEntityTypeKeys::PIG) {
     *     // 处理猪（指针比较）
     * }
     * @endcode
     */
    [[nodiscard]] const entity::EntityType* entityType() const;

    /**
     * @brief 获取实体的默认战利品表ID
     *
     * 默认实现从实体类型ID推导战利品表路径：
     * minecraft:pig -> minecraft:entities/pig
     * 子类可覆写此方法返回空字符串表示无战利品表（如投射物、区域效果云等），
     * 或返回自定义战利品表路径。
     *
     * @return 战利品表ID字符串，无战利品表时返回空字符串
     */
    [[nodiscard]] virtual std::string getLootTableId() const;

    /**
     * @brief 取本实体在 AddEntity 包 data 字段携带的实体特定数据
     *
     * 对齐 vanilla 1.21.11 ClientboundAddEntityPacket.entityData：一个 i32，
     * 语义随实体类型而异。vanilla FallingBlockEntity.getEntityData() 返回
     * `Block.BLOCK_STATE_REGISTRY.getId(this.getBlockState())`（即 BlockState 的
     * stateId），用于让客户端在 spawn 时刻即拿到下落方块的方块状态——vanilla
     * FallingBlock 的 BlockState 不走 SynchedEntityData，仅此一处下发。
     *
     * 默认实现返回 0（无实体特定 data）。FallingBlockEntity 等需要下发 spawn data
     * 的子类覆写本方法返回对应 stateId。
     *
     * @return AddEntity.data 字段值（i32，语义随实体类型）
     */
    [[nodiscard]] virtual i32 getSpawnData() const { return 0; }

    // ========== 世界访问 ==========

    [[nodiscard]] IWorld* world() { return m_world; }
    [[nodiscard]] const IWorld* world() const { return m_world; }
    void setWorld(IWorld* world) { m_world = world; }

    /**
     * @brief 绑定/解绑所属 EntityManager（用于空间索引实时维护）
     *
     * 由 `EntityManager::addEntity` 绑定 this、`removeEntity`/`_removeDeadEntitiesInternal`
     * 解绑 nullptr。`reapplyPosition()` 经此反向指针通知索引迁移，使 3D section 索引
     * 实时准确（同一 tick 内移动后查询读到新位置）。裸指针非拥有，EntityManager 与
     * Entity 同在 common 模块，不破坏 common 不依赖 server。
     */
    void setEntityManager(EntityManager* manager) noexcept { m_entityManager = manager; }
    [[nodiscard]] EntityManager* entityManager() const noexcept { return m_entityManager; }

    // ========== 数据管理 ==========

    [[nodiscard]] entity::EntityDataManager& dataManager() { return m_dataManager; }
    [[nodiscard]] const entity::EntityDataManager& dataManager() const { return m_dataManager; }

    // ========== 位置 ==========
    //
    // 位置/速度/旋转/AABB 四类高频数据已迁入 ECS 组件（StateVectorComponent 等），
    // 本类不再持有可写字段副本，getter/setter 直接读写 m_builtIn 缓存的组件裸指针
    // （零双写硬约束：避免同帧字段与组件两份可写副本不一致）。m_builtIn 在构造时
    // attach 组件后填充，随 Entity 生命周期稳定（首批4组件永不移除，指针稳定）。

    [[nodiscard]] Vector3 position() const { return m_builtIn.stateVector->m_pos; }
    [[nodiscard]] f32 x() const { return m_builtIn.stateVector->m_pos.x; }
    [[nodiscard]] f32 y() const { return m_builtIn.stateVector->m_pos.y; }
    [[nodiscard]] f32 z() const { return m_builtIn.stateVector->m_pos.z; }

    /**
     * @brief 获取实体高度按比例偏移后的 Y 坐标
     *
     * 对应 MC 1.21.11 Entity.getY(double partialY)。
     * 计算公式：position.y + height * partialY。
     *
     * 常用 partialY 值（参考 MC 原版调用约定）：
     * - 0.0：脚部 Y（等价于 y()）
     * - 1/3：胸部高度，用于弓箭/三叉戟/弩瞄准（AbstractSkeleton/Drowned/Illusioner/CrossbowItem）
     * - 0.5：身体几何中心，用于粒子/瞄准点（Blaze/Ghast/Guardian/EnderMan/Phantom）
     * - 0.8：接近头部，用于骑乘时瞄准（Breeze Shoot）或钓鱼钩吸附位置（FishingHook）
     * - 1.0：实体头顶（MushroomCow 掉落物、AbstractBoat 水位线）
     *
     * 注意：与 getEyeY() 不同——getEyeY 使用 eyeHeight 偏移，本方法使用 height 偏移，
     * 两者仅在实体姿态/尺寸固定时通过 eyeHeight/height 比例间接关联。
     *
     * @param partialY 高度比例（0.0 = 脚部，1.0 = 头顶）
     * @return 偏移后的 Y 坐标（f64 精度，与 MC 原版一致）
     */
    [[nodiscard]] f64 getY(f64 partialY) const
    {
        return static_cast<f64>(m_builtIn.stateVector->m_pos.y) + static_cast<f64>(height()) * partialY;
    }

    /**
     * @brief 获取眼睛高度对应的 Y 坐标
     *
     * 对应 MC 1.21.11 Entity.getEyeY()，计算公式：position.y + eyeHeight。
     * 用于瞄准、视线碰撞（canSee）、弹射物发射位置等需要"眼睛位置"的场景。
     *
     * 项目内原有多处 `y() + eyeHeight()` 内联代码（Entity::canSee、Player::getEyePosition、
     * LlamaEntity、DrownedEntity、WitchEntity、NetherEntities、IllagerEntities 等），
     * 已统一重构为本方法调用以提升复用性。
     *
     * @return 眼睛 Y 坐标（f64 精度）
     */
    [[nodiscard]] f64 getEyeY() const
    {
        return static_cast<f64>(m_builtIn.stateVector->m_pos.y) + static_cast<f64>(eyeHeight());
    }

    /**
     * @brief 获取实体所站立的方块位置（对应 MC Entity.getOnPos()）
     *
     * 返回实体脚下方块的位置，即 Y 坐标向下取整后再减1。
     * 用于方块交互、音效/粒子播放位置等场景。
     *
     * @return 实体脚下方块的 BlockPos
     */
    [[nodiscard]] BlockPos onPos() const
    {
        return BlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)) - 1,
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    }

    // 前一帧位置（用于插值）
    [[nodiscard]] Vector3 prevPosition() const { return m_builtIn.stateVector->m_posPrev; }
    [[nodiscard]] f32 prevX() const { return m_builtIn.stateVector->m_posPrev.x; }
    [[nodiscard]] f32 prevY() const { return m_builtIn.stateVector->m_posPrev.y; }
    [[nodiscard]] f32 prevZ() const { return m_builtIn.stateVector->m_posPrev.z; }

    /**
     * @brief 计算到另一个实体的距离
     * @param other 另一个实体
     * @return 距离（非平方）
     */
    [[nodiscard]] f32 distanceTo(const Entity& other) const
    {
        return m_builtIn.stateVector->m_pos.distance(other.m_builtIn.stateVector->m_pos);
    }

    /**
     * @brief 计算到另一个实体的距离平方
     * @param other 另一个实体
     * @return 距离的平方（避免开方运算，适合比较）
     */
    [[nodiscard]] f32 distanceSqTo(const Entity& other) const
    {
        return m_builtIn.stateVector->m_pos.distanceSquared(other.m_builtIn.stateVector->m_pos);
    }

    /**
     * @brief 计算到指定位置的距离平方
     * @param px 目标X坐标
     * @param py 目标Y坐标
     * @param pz 目标Z坐标
     * @return 距离的平方
     */
    [[nodiscard]] f32 distanceSqTo(f32 px, f32 py, f32 pz) const
    {
        f32 dx = px - m_builtIn.stateVector->m_pos.x;
        f32 dy = py - m_builtIn.stateVector->m_pos.y;
        f32 dz = pz - m_builtIn.stateVector->m_pos.z;
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算到指定位置的水平距离平方（忽略Y轴）
     */
    [[nodiscard]] f32 distanceHorizontalSqTo(f32 px, f32 pz) const
    {
        f32 dx = px - m_builtIn.stateVector->m_pos.x;
        f32 dz = pz - m_builtIn.stateVector->m_pos.z;
        return dx * dx + dz * dz;
    }

    // ========== 旋转 ==========

    [[nodiscard]] f32 yaw() const { return m_builtIn.rotation->m_rot.x; }
    [[nodiscard]] f32 pitch() const { return m_builtIn.rotation->m_rot.y; }
    [[nodiscard]] f32 prevYaw() const { return m_builtIn.rotation->m_rotPrev.x; }
    [[nodiscard]] f32 prevPitch() const { return m_builtIn.rotation->m_rotPrev.y; }

    /// 设置偏航角（不更新 prevYaw，用于偏转等场景）
    void setYaw(f32 yaw) { m_builtIn.rotation->m_rot.x = yaw; }

    /// 设置上一tick偏航角
    void setPrevYaw(f32 prevYaw) { m_builtIn.rotation->m_rotPrev.x = prevYaw; }

    // ========== 速度 ==========

    [[nodiscard]] Vector3 velocity() const { return m_builtIn.velocity->m_velocity; }
    [[nodiscard]] f32 velocityX() const { return m_builtIn.velocity->m_velocity.x; }
    [[nodiscard]] f32 velocityY() const { return m_builtIn.velocity->m_velocity.y; }
    [[nodiscard]] f32 velocityZ() const { return m_builtIn.velocity->m_velocity.z; }

    // ========== 状态 ==========

    [[nodiscard]] bool onGround() const { return m_builtIn.physicsState->m_onGround; }
    [[nodiscard]] bool isRemoved() const { return m_removed; }
    // pose/flags 已迁入 ecs::EntityStateComponent/EntityFlagsComponent（真相源），经组件查询读取。
    [[nodiscard]] EntityPose pose() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_pose;
    }
    [[nodiscard]] EntityFlags flags() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityFlagsComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_flags;
    }

    /**
     * @brief 是否拥有某 ECS 组件（透传 EntityContext::hasComponent）
     *
     * 供 Entity 继承体系外的调用方（AI goal/sensor、BlockEntity 等）做 tag/
     * capability 类型标记查询，替代 dynamic_cast<接口*>。const 安全。批次5 子批
     * 5.1 起 tag component 查询的统一外部入口。
     */
    template <class T>
    [[nodiscard]] bool hasComponent() const
    {
        return m_entityContext->hasComponent<T>();
    }

    /**
     * @brief 查询组件指针（透传 EntityContext::tryGetComponent）
     *
     * 供 Entity 继承体系外的调用方（序列化器、AI goal/sensor、BlockEntity 等）做
     * capability/tag 组件读写，替代 dynamic_cast<接口*>。const 安全。批次6 子目标1
     * 起组件序列化器经它直写组件内部字段（如 HealthComponent.m_healthSynced）。
     *
     * 注意：业务逻辑仍走 getter/setter（保持 DataParameter 同步副作用），仅在序列化
     * 等需直访组件内部的横切关注点使用本方法。
     */
    template <class T>
    [[nodiscard]] T* tryGetComponent()
    {
        return m_entityContext->tryGetComponent<T>();
    }
    template <class T>
    [[nodiscard]] const T* tryGetComponent() const
    {
        return m_entityContext->tryGetComponent<T>();
    }

    /**
     * @brief 获取本实体所属的 ECS registry（构造时透传绑定，存于 EntityContext）
     *
     * ECS 迁移后实体构造透传 ecs::EntityRegistry& 并 attach 组件，EntityContext 持有该
     * registry 引用。本方法返回该引用，供需要创建子实体/同级实体（如 spawnBaby、
     * spawnChild、summon）的场景获取 registry，避免硬依赖 m_world。
     *
     * 背景：原 spawnBaby 等通过 m_world->entityRegistry() 获取 registry，但单元测试中
     * 构造的裸实体未 setWorld（m_world==nullptr）会解引用空指针崩溃。实体构造时已绑定
     * registry，自给自足更合理。生产场景下实体总在 world 内，本方法返回的 registry 与
     * m_world->entityRegistry() 一致；测试场景下实体未 setWorld 也能正确返回构造时传入
     * 的 registry。
     *
     * 返回引用，调用方须确保本实体生命周期内 registry 仍有效（实体与 registry 同生命周期）。
     */
    [[nodiscard]] ecs::EntityRegistry& ecsRegistry() { return m_entityContext->registry(); }
    [[nodiscard]] const ecs::EntityRegistry& ecsRegistry() const { return m_entityContext->registry(); }

    [[nodiscard]] virtual bool isChild() const { return false; }

    // ========== 声音 ==========

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] virtual sound::SoundCategory getSoundCategory() const { return sound::SoundCategory::Neutral; }

    /**
     * @brief 播放实体声音
     * @param soundEventId 声音事件ID
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    void playSound(const ResourceLocation& soundEventId, f32 volume, f32 pitch) const;

    /**
     * @brief 获取溅水声音
     *
     * 子类可覆盖以提供特定溅水音效。
     *
     * @return 溅水声音事件
     */
    [[nodiscard]] virtual ResourceLocation getSplashSound() const;

    /**
     * @brief 获取高速溅水声音
     *
     * 当实体高速入水时播放此音效。
     * 子类可覆盖以提供特定高速溅水音效。
     *
     * @return 高速溅水声音事件
     */
    [[nodiscard]] virtual ResourceLocation getHighspeedSplashSound() const;

    /**
     * @brief 执行水花溅射效果
     *
     * 当实体入水时播放水花音效并生成气泡和水溅粒子。
     *
     * 实现细节：
     * - 根据实体速度计算溅水强度 f1
     * - f1 < 0.25 时播放普通溅水声，否则播放高速溅水声
     * - 音量使用 f1，音调使用 1.0 + (rand - rand) * 0.4
     * - 生成 (1 + width * 20) 个气泡粒子和水溅粒子
     * - 粒子位置在实体包围盒内随机，Y 坐标固定在水面上方
     *
     * Player 类覆盖此方法以检查观察者模式。
     */
    virtual void doWaterSplashEffect();

    // ========== 设置属性 ==========

    void setPosition(f32 x, f32 y, f32 z);
    void setPosition(const Vector3& pos) { setPosition(pos.x, pos.y, pos.z); }

    /**
     * @brief 将当前位置与旋转记录为下一次渲染插值的起点
     */
    void snapshotInterpolationState();

    void setRotation(f32 yaw, f32 pitch);
    void setVelocity(f32 x, f32 y, f32 z);
    void setVelocity(const Vector3& vel) { setVelocity(vel.x, vel.y, vel.z); }

    /**
     * @brief 设置身体偏航角（默认空实现，LivingEntity 重写以持久化字段）
     *
     * 对齐 MC 1.21.11 Entity#setYBodyRot：基类为空操作，子类（LivingEntity）
     * 重写后写入 yBodyRot 字段。结构模板放置实体、实体从 NBT 加载等场景
     * 需要让身体朝向跟随结构旋转，调用此方法可统一处理任意实体类型，
     * 无需调用方做 dynamic_cast<LivingEntity*> 判断。
     *
     * @param yaw 身体偏航角（度）
     */
    virtual void setYBodyRot(f32 yaw) { (void)yaw; }

    /**
     * @brief 设置头部偏航角（默认空实现，LivingEntity 重写以持久化字段）
     *
     * 对齐 MC 1.21.11 Entity#setYHeadRot：基类为空操作，子类（LivingEntity）
     * 重写后写入 yHeadRot 字段。AI LookController、结构模板放置等场景使用。
     *
     * @param yaw 头部偏航角（度）
     */
    virtual void setYHeadRot(f32 yaw) { (void)yaw; }

    /**
     * @brief 添加速度增量
     * @param dx X方向增量
     * @param dy Y方向增量
     * @param dz Z方向增量
     */
    void addVelocity(f32 dx, f32 dy, f32 dz)
    {
        m_builtIn.velocity->m_velocity.x += dx;
        m_builtIn.velocity->m_velocity.y += dy;
        m_builtIn.velocity->m_velocity.z += dz;
    }

    /**
     * @brief 添加速度增量
     * @param delta 速度增量向量
     */
    void addVelocity(const Vector3& delta) { addVelocity(delta.x, delta.y, delta.z); }

    /**
     * @brief 缩放速度向量
     * @param factor 缩放因子
     *
     * 用于阻力计算。
     */
    void scaleVelocity(f32 factor)
    {
        m_builtIn.velocity->m_velocity.x *= factor;
        m_builtIn.velocity->m_velocity.y *= factor;
        m_builtIn.velocity->m_velocity.z *= factor;
    }

    /**
     * @brief 根据偏航角计算相对移动并添加到速度
     *
     * 根据实体的偏航角，将相对移动向量转换为绝对移动向量，
     * 并乘以移动因子后添加到当前速度。
     *
     * @param factor 移动因子（速度倍率）
     * @param strafe 左右移动（负值向左，正值向右）
     * @param vertical 垂直移动（负值向下，正值向上）
     * @param forward 前后移动（负值向后，正值向前）
     */
    void moveRelative(f32 factor, f32 strafe, f32 vertical, f32 forward);

    /**
     * @brief 设置是否在地面上
     *
     * 当实体落地时（onGround从false变为true），
     * 会清空攀爬位置追踪。
     *
     * @param onGround 是否在地面上
     */
    void setOnGround(bool onGround)
    {
        if (onGround && !m_builtIn.physicsState->m_onGround) {
            // 落地时清空攀爬位置
            m_lastClimbPos = std::nullopt;
        }
        m_builtIn.physicsState->m_onGround = onGround;
    }
    void setPose(EntityPose pose);
    void setFlags(EntityFlags flags);

    // 标志操作
    void addFlag(EntityFlags flag);
    void removeFlag(EntityFlags flag);
    [[nodiscard]] bool hasFlag(EntityFlags flag) const { return mc::hasFlag(flags(), flag); }

    /**
     * @brief 检查是否正在鞘翅飞行
     *
     * 通过检查 FallFlying 标志位（第7位）来判断。
     *
     * @return 如果正在鞘翅飞行返回 true
     */
    [[nodiscard]] bool isElytraFlying() const { return hasFlag(EntityFlags::FallFlying); }

    /**
     * @brief 设置实体游泳状态
     *
     * 通过设置 Swimming 标志位（第4位）来同步客户端与服务端的游泳状态。
     * 该标志位会通过 DATA_FLAGS_PARAM 自动同步到客户端。
     *
     * @param swimming 是否正在游泳
     */
    void setSwimming(bool swimming)
    {
        if (swimming) {
            addFlag(EntityFlags::Swimming);
        } else {
            removeFlag(EntityFlags::Swimming);
        }
    }

    /**
     * @brief 检查实体是否正在游泳
     *
     * 通过检查 Swimming 标志位（第4位）来判断。
     *
     * @return 如果正在游泳返回 true
     */
    [[nodiscard]] bool isSwimming() const { return hasFlag(EntityFlags::Swimming); }

    /**
     * @brief 检查实体是否在视觉上表现为游泳姿态
     *
     * 基类实现：当实体姿态为 Swimming 时返回 true。
     * LivingEntity 重写此方法，额外考虑 FallFlying 姿态（与鞘翅飞行视觉重叠）。
     * DrownedEntity 重写此方法，要求 isSwimming() 为 true 且未骑乘其他实体。
     *
     * 该方法用于驱动 swimAmount 的渐入渐出（updateSwimAmount）。
     *
     * @return 如果视觉上表现为游泳姿态返回 true
     */
    [[nodiscard]] virtual bool isVisuallySwimming() const;

    /**
     * @brief 检查实体是否发光
     *
     * 发光效果来源：
     * 1. 发光药水效果 (StatusEffectType::GLOWING)
     * 2. 发光鱿鱼实体类型
     * 3. 团队发光规则
     *
     * 客户端检查数据参数中的 Glowing 标志位，
     * 服务端检查 m_glowing 字段。
     *
     * @return 如果实体发光返回 true
     */
    [[nodiscard]] bool isGlowing() const;

    /**
     * @brief 设置实体发光状态
     *
     * 在服务端设置 m_glowing 字段并同步 Glowing 标志位。
     *
     * @param glowing 是否发光
     */
    void setGlowing(bool glowing);

    /**
     * @brief 获取实体所属队伍
     *
     * 基类默认返回 nullptr。
     * ServerPlayer 子类重写此方法，通过服务器记分板获取玩家所在队伍。
     * TameableEntity 子类重写此方法，继承主人的队伍。
     *
     * @return 队伍指针，如果实体不在任何队伍返回 nullptr
     */
    [[nodiscard]] virtual scoreboard::Team* getTeam() { return nullptr; }
    [[nodiscard]] virtual const scoreboard::Team* getTeam() const { return nullptr; }

    /**
     * @brief 检查实体是否在指定队伍中
     *
     * @param team 要检查的队伍
     * @return true 如果实体属于该队伍
     */
    [[nodiscard]] bool isOnScoreboardTeam(const scoreboard::Team* team) const;

    /**
     * @brief 检查两个实体是否为盟友关系
     *
     * 双向检查：this 认为 other 是盟友，或 other 认为 this 是盟友。
     * 子类可重写 considersEntityAsAlly 来自定义盟友判定逻辑。
     *
     * @param other 另一个实体
     * @return true 如果两个实体互为盟友
     */
    [[nodiscard]] bool isAlliedTo(const Entity& other) const;

    /**
     * @brief 检查此实体是否将指定队伍视为盟友
     *
     * 默认实现通过 getTeam() 判断是否同一队伍。
     * 子类可重写此方法以支持更复杂的盟友关系（如驯服动物继承主人的队伍）。
     *
     * @param team 要检查的队伍
     * @return true 如果此实体属于该队伍或与该队伍为盟友关系
     */
    [[nodiscard]] virtual bool isAlliedTo(const scoreboard::Team* team) const;

    /**
     * @brief 检查此实体是否将指定实体视为盟友
     *
     * 默认实现调用 isAlliedTo(other.getTeam())。
     * 子类可重写此方法以支持更复杂的盟友关系。
     *
     * @param other 另一个实体
     * @return true 如果此实体将 other 视为盟友
     */
    [[nodiscard]] virtual bool considersEntityAsAlly(const Entity& other) const;

    /**
     * @brief 检查两个实体是否在同一队伍
     *
     * 旧版 API，仅检查 this 是否属于 other 的队伍。
     * 新代码应优先使用 isAlliedTo() 进行双向检查。
     *
     * @param other 另一个实体
     * @return true 如果两个实体在同一队伍
     */
    [[nodiscard]] bool isOnSameTeam(const Entity& other) const;

    // ========== 尺寸 ==========

    /**
     * @brief 获取实体宽度
     * @return 实体宽度（方块单位）
     */
    [[nodiscard]] virtual f32 width() const { return 0.6f; }

    /**
     * @brief 获取实体高度
     * @return 实体高度（方块单位）
     */
    [[nodiscard]] virtual f32 height() const { return 1.8f; }

    /**
     * @brief 获取眼睛高度
     * @return 眼睛距离脚底的高度（方块单位）
     */
    [[nodiscard]] virtual f32 eyeHeight() const { return 1.62f; }

    /**
     * @brief 获取指定姿态下的实体尺寸
     * @param pose 目标姿态
     * @return 对应姿态的尺寸信息
     */
    [[nodiscard]] virtual entity::EntitySize getDimensions(EntityPose pose) const;

    /**
     * @brief 获取当前缓存的实体尺寸
     * @return 当前尺寸
     */
    [[nodiscard]] entity::EntitySize currentDimensions() const { return m_dimensions; }

    /**
     * @brief 刷新当前尺寸和碰撞箱
     *
     * 当姿态或其他会影响尺寸的状态变化时调用。
     */
    void refreshDimensions();

    /**
     * @brief 获取步进高度
     * @return 实体可以自动步进的最大高度（默认为0，LivingEntity默认为0.6）
     */
    [[nodiscard]] virtual f32 stepHeight() const { return m_stepHeight; }

    /**
     * @brief 设置步进高度
     *
     * 用于设置实体可以自动步进的最大高度：
     * - 玩家/大多数生物：0.6（可走上台阶）
     * - 马、铁傀儡、末影人等：1.0（可走上完整方块）
     * - 盔甲架：0.0（无法步进）
     *
     * @param height 步进高度
     */
    void setStepHeight(f32 height) { m_stepHeight = height; }

    // ========== 碰撞箱 ==========

    /**
     * @brief 获取实体碰撞箱
     * @return 基于当前位置的AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB boundingBox() const
    {
        if (!m_dimensionsInitialized) {
            const_cast<Entity*>(this)->refreshDimensions();
        }
        return m_builtIn.aabbShape->m_aabb;
    }

    /**
     * @brief 实体是否可被碰撞或射线命中
     *
     * 对应 MC Java 的 isPickable()。决定实体是否拥有可交互的碰撞箱。
     */
    [[nodiscard]] virtual bool canBeCollidedWith() const { return true; }

    /**
     * @brief this 是否会与指定实体发生碰撞
     *
     * 对应 MC Java 的 Entity.canCollideWith(Entity)：
     *   return other.canBeCollidedWith(this) && !isPassengerOfSameVehicle(other);
     *
     * 即：candidate 必须可被碰撞，且不能与 this 同处一个骑乘链
     * （载具不会与其乘客、共享同一载具的乘客之间互相碰撞）。
     *
     * 注意：MC Java 的 getEntityCollisions 在传入 entity 时使用
     * `EntitySelector.NO_SPECTATORS.and(entity::canCollideWith)` 作为过滤谓词，
     * 因此物理碰撞检测中的"实体碰撞"会调用此方法。
     *
     * @param other 候选实体
     * @return 若 this 应与 other 进行碰撞检测返回 true
     */
    [[nodiscard]] virtual bool canCollideWith(const Entity& other) const
    {
        return other.canBeCollidedWith() && !isRidingSameEntity(other);
    }

    /**
     * @brief 实体是否可被弹射物命中
     *
     * 对应 MC Java 的 canBeHitByProjectile()。
     * 默认实现为 isAlive() && canBeCollidedWith()（MC Java 中为 isAlive() && isPickable()，
     * 本项目中 canBeCollidedWith() 对应 isPickable() 的语义）。
     *
     * 子类可重写此方法以改变弹射物命中行为：
     * - Player 重写为 !isSpectator() && Entity::canBeHitByProjectile()
     * - Interaction 实体重写为返回 false（本项目中尚未实现该实体）
     *
     * 与 canBeCollidedWith() 的区别：
     * - canBeCollidedWith() 关注碰撞箱是否可交互（物理碰撞、射线命中）
     * - canBeHitByProjectile() 关注弹射物是否可命中（综合判断存活状态和旁观者模式等）
     */
    [[nodiscard]] virtual bool canBeHitByProjectile() const { return isAlive() && canBeCollidedWith(); }

    /**
     * @brief 获取此实体对指定弹射物的偏转类型
     *
     * 对应 MC Java 的 Entity.deflection(Projectile)。
     * 当弹射物命中此实体时，在调用 onEntityHit 之前先检查此方法。
     * 如果返回非 None 的偏转类型，弹射物将被偏转而不是命中实体。
     *
     * 默认实现检查实体类型是否属于 #minecraft:deflects_projectiles 标签：
     * - 属于标签时返回 Reverse（反向偏转）
     * - 不属于标签时返回 None（不偏转）
     *
     * 子类可重写此方法以自定义偏转行为：
     * - BreezeEntity 重写以排除风弹的偏转
     *
     * @param projectile 正在命中的弹射物
     * @return 偏转类型
     */
    [[nodiscard]] virtual ProjectileDeflection deflection(const entity::ProjectileEntity& projectile) const;

    /**
     * @brief 命中检测时额外扩张的碰撞边界
     */
    [[nodiscard]] virtual f32 getCollisionBorderSize() const { return 0.0f; }

    // ========== 更新 ==========

    virtual void tick();
    virtual void update();

    // ========== 移动 ==========

    /**
     * @brief 直接移动实体（无碰撞检测）
     * @param dx, dy, dz 移动增量
     */
    void move(f32 dx, f32 dy, f32 dz);

    /**
     * @brief 使用指定移动类型移动实体
     *
     * 用于区分不同来源的移动（活塞推动、玩家推动、自身移动等）。
     *
     * @param type 移动类型
     * @param delta 移动增量
     */
    void move(entity::MoverType type, const Vector3& delta);

    /**
     * @brief 旋转实体
     * @param deltaYaw 偏航角增量（度）
     * @param deltaPitch 俯仰角增量（度）
     */
    void rotate(f32 deltaYaw, f32 deltaPitch);

    // ========== 推动反应 ==========

    /**
     * @brief 获取实体的推动反应类型
     *
     * 子类可重写以返回不同的推动反应。
     *
     * @return 推动反应类型
     */
    [[nodiscard]] virtual PushReaction getPushReaction() const { return PushReaction::Normal; }

    // ========== 物理 ==========

    /**
     * @brief 设置物理引擎（已废弃，优先使用 World 的物理引擎）
     * @param engine 物理引擎指针（不拥有所有权）
     * @deprecated 使用 World::physicsEngine() 替代
     */
    void setPhysicsEngine(PhysicsEngine* engine) { m_physicsEngine = engine; }

    /**
     * @brief 获取物理引擎
     *
     * 优先返回 World 的物理引擎，如果没有则返回显式设置的物理引擎。
     */
    [[nodiscard]] PhysicsEngine* physicsEngine();
    [[nodiscard]] const PhysicsEngine* physicsEngine() const;

    /**
     * @brief 带碰撞检测的移动
     *
     * 使用物理引擎进行带碰撞检测的移动。
     * 会更新 m_onGround、m_collidedHorizontally、m_collidedVertically 状态。
     *
     * @param dx, dy, dz 期望移动增量
     * @return 实际移动增量
     */
    Vector3 moveWithCollision(f32 dx, f32 dy, f32 dz);

    /**
     * @brief 应用物理效果（重力、阻力）
     *
     * 更新速度：应用重力和空气阻力。
     * 如果在地面，重置Y速度为0。
     *
     * @param deltaTime 时间增量（秒）
     */
    void applyPhysics(f32 deltaTime);

    /**
     * @brief 检测是否在地面上
     *
     * 通过检测实体下方是否有方块碰撞来判断。
     * 如果 World 存在，使用 World 的碰撞检测；
     * 否则使用物理引擎（如果有）。
     */
    void checkOnGround();

    // ========== 碰撞状态 ==========

    [[nodiscard]] bool collidedHorizontally() const { return m_builtIn.physicsState->m_collidedHorizontally; }
    [[nodiscard]] bool collidedVertically() const { return m_builtIn.physicsState->m_collidedVertically; }
    [[nodiscard]] f32 fallDistance() const { return m_builtIn.physicsState->m_fallDistance; }

    /**
     * @brief 设置摔落距离
     * @param distance 摔落距离
     */
    void setFallDistance(f32 distance) { m_builtIn.physicsState->m_fallDistance = distance; }

    // ========== 移除 ==========

    /**
     * @brief 移除实体
     *
     * 标记实体为已移除状态。子类可以重写此方法在移除前执行额外逻辑
     * （例如史莱姆分裂）。
     * 对应 MC Java 的 Entity.remove(RemovalReason.KILLED)。
     */
    virtual void remove() { m_removed = true; }

    /**
     * @brief 静默丢弃实体
     *
     * 与 remove() 不同，discard() 不触发任何掉落物、经验或其他死亡相关逻辑，
     * 仅将实体标记为已移除。适用于实体需要立即消失但不应产生副作用的场景，
     * 例如末影龙战斗状态扫描中发现无传送门的孤龙时将其丢弃。
     * 对应 MC Java 的 Entity.discard()。
     */
    virtual void discard() { m_removed = true; }

    /**
     * @brief 由 /kill 命令调用
     *
     * 默认实现直接调用 remove()。
     * LivingEntity 重写此方法使用虚空伤害杀死实体。
     */
    virtual void onKillCommand() { remove(); }

    // ========== 维度 ==========

    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    // ========== 传送门 ==========

    /**
     * @brief 获取传送冷却时间
     * @return 剩余冷却时间（tick），0 表示可以传送
     */
    [[nodiscard]] i32 portalCooldown() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>();
        return c != nullptr ? c->m_portalCooldown : 0;
    }

    /**
     * @brief 设置传送冷却时间
     * @param cooldown 冷却时间（tick）
     */
    void setPortalCooldown(i32 cooldown)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>()) {
            c->m_portalCooldown = cooldown;
        }
    }

    /**
     * @brief 检查是否可以传送
     * @return 如果冷却时间为 0 则返回 true
     */
    [[nodiscard]] bool canTeleport() const { return portalCooldown() <= 0; }

    /**
     * @brief 获取在传送门中的累计时间
     * @return 累计时间（tick）
     */
    [[nodiscard]] i32 portalTime() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>();
        return c != nullptr ? c->m_portalTime : 0;
    }

    /**
     * @brief 设置在传送门中的累计时间
     * @param time 累计时间（tick）
     */
    void setPortalTime(i32 time)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>()) {
            c->m_portalTime = time;
        }
    }

    /**
     * @brief 重置传送门计时
     */
    void resetPortalTime() { setPortalTime(0); }

    /**
     * @brief 检查是否在传送门中
     */
    [[nodiscard]] bool isInPortal() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>();
        return c != nullptr ? c->m_inPortal : false;
    }

    /**
     * @brief 设置是否在传送门中
     */
    void setInPortal(bool inPortal)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>()) {
            c->m_inPortal = inPortal;
        }
    }

    /**
     * @brief 获取在传送门中停留所需的最大时间
     *
     * 玩家需要 80 tick (4秒) 在传送门中才能传送。
     * 其他实体只需要 1 tick（因为基类返回 0，递增后立即满足条件）。
     *
     * @return 传送所需的最大时间（tick）
     */
    [[nodiscard]] virtual i32 getMaxInPortalTime() const { return 0; }

    /**
     * @brief 当传送门触发时调用
     *
     * 当实体在传送门中停留足够时间后触发。
     * 子类（如 ServerPlayer）可重写此方法以实现实际的维度切换逻辑。
     *
     * 传送门 tick 逻辑（计时递进/冷却递减/inPortal 重置）已迁入 ecs::sys::portalTick
     * （PostEntityTick 阶段），本方法仅由 system 在达阈值时调用，执行重置 + 触发冷却。
     *
     * @return true 如果传送成功
     */
    virtual bool onPortalTriggered();

    /**
     * @brief 跨维度传送（基类默认实现返回 false，不传送）。
     *
     * ServerPlayer override 调真实 changeDimension 实现。common 层方块（如 EndPortalBlock）
     * 经此虚派发触发跨维度传送，无需 common->server 头文件依赖。
     *
     * @param targetDim 目标维度 ID
     * @return true 如果传送成功；基类默认 false（非玩家实体当前不支持跨维度）
     *
     * @todo 非玩家实体的跨维度传送（vanilla 末地门对矿车/掉落物等也传送）暂未实现，
     *       基类返回 false。后续如需支持，应在 ServerWorld 层实现通用实体迁移。
     */
    virtual bool changeDimension(DimensionId targetDim)
    {
        (void)targetDim;
        return false;
    }

    /**
     * @brief 设置实体所在的传送门方块位置
     *
     * 当实体进入传送门方块时调用。
     *
     * @param pos 传送门方块位置
     */
    void setPortalPos(const BlockPos& pos)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>()) {
            c->m_portalPos = pos;
        }
    }

    /**
     * @brief 获取实体所在的传送门方块位置
     */
    [[nodiscard]] const BlockPos& portalPos() const
    {
        // 返回引用：组件存在时返回组件内字段引用，不存在时返回静态默认值。
        const auto* c = m_entityContext->tryGetComponent<ecs::PortalComponent>();
        return c != nullptr ? c->m_portalPos : ecs::PortalComponent::s_defaultPos;
    }

    /**
     * @brief 触发传送冷却
     *
     * 传送后设置冷却时间，防止立即再次传送。
     * 默认冷却时间为 300 tick (15秒)。
     */
    void triggerPortalCooldown() { setPortalCooldown(getPortalCooldown()); }

    /**
     * @brief 获取默认传送冷却时间
     */
    [[nodiscard]] virtual i32 getPortalCooldown() const { return 300; }

    /**
     * @brief 检查实体是否不是Boss
     *
     * Boss实体（末影龙、凋灵）不能使用传送门。
     * 默认返回 true，Boss实体覆盖返回 false。
     *
     * @return 如果实体不是Boss返回 true
     */
    [[nodiscard]] virtual bool isNonBoss() const { return true; }

    // ========== 随机传送 ==========

    /**
     * @brief 尝试传送到指定位置
     *
     * 安全传送流程：
     * 1. 检查目标区块是否已加载
     * 2. 从目标位置向下查找固体地面
     * 3. 检查目标位置是否有碰撞
     * 4. 检查目标位置是否在液体中
     * 5. 如果检查失败，恢复原位置
     *
     * @param x 目标 X 坐标
     * @param y 目标 Y 坐标
     * @param z 目标 Z 坐标
     * @param playEffects 是否播放传送效果（粒子、音效）
     * @return 如果传送成功返回 true
     */
    bool attemptTeleport(f64 x, f64 y, f64 z, bool playEffects = true);

    /**
     * @brief 随机传送到附近位置
     *
     * 在指定范围内随机选择位置尝试传送：
     * - 水平方向：以实体为中心，±range 格范围
     * - 垂直方向：以实体为中心，±range 格范围
     *
     * 最多尝试 16 次，找到第一个有效位置后立即传送。
     * 如果所有尝试都失败，保持原位置不变。
     *
     * @param range 随机范围（水平和垂直）
     * @param playEffects 是否播放传送效果
     * @param avoidFluid 是否避免液体（水、岩浆）
     * @return 如果传送成功返回 true
     */
    bool randomTeleport(f64 range, bool playEffects = true, bool avoidFluid = true);

    /**
     * @brief 查找安全的传送位置
     *
     * 从目标位置向下查找第一个安全的站立位置。
     * 安全位置定义：
     * - 有固体方块作为脚底
     * - 脚底上方两格无碰撞
     * - 不在液体中
     *
     * @param x 目标 X 坐标
     * @param y 目标 Y 坐标（起始搜索高度）
     * @param z 目标 Z 坐标
     * @param avoidFluid 是否避免液体
     * @return 安全位置，如果找不到返回 nullopt
     */
    [[nodiscard]] std::optional<Vector3d> findSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid = true) const;

    /**
     * @brief 检查位置是否可以安全站立
     *
     * 检查实体在该位置是否会有碰撞和液体问题。
     *
     * @param x 目标 X 坐标
     * @param y 目标 Y 坐标（脚底高度）
     * @param z 目标 Z 坐标
     * @param avoidFluid 是否检查液体
     * @return 如果位置安全返回 true
     */
    [[nodiscard]] bool isSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid = true) const;

    // ========== 存活时间 ==========

    [[nodiscard]] u32 ticksExisted() const { return m_ticksExisted; }

    // ========== 随机数 ==========

    /**
     * @brief 获取实体的随机数生成器
     *
     * 每个实体拥有独立的持久化随机数生成器，在构造时以唯一种子初始化。
     * 多次调用返回同一对象的引用，保证随机序列的连续性。
     *
     * @return 实体随机数生成器的引用
     */
    [[nodiscard]] math::Random& getRandom() { return m_random; }
    [[nodiscard]] math::Random& getRandom() const { return m_random; }

    // ========== 存活状态 ==========

    /**
     * @brief 检查实体是否存活
     * @return 如果实体未被移除且未死亡则返回 true
     */
    [[nodiscard]] virtual bool isAlive() const { return !m_removed; }

    // ========== 玩家碰撞 ==========

    /**
     * @brief 当玩家与此实体碰撞时调用
     *
     * 用于处理玩家拾取物品、经验球、箭矢等。
     * 子类可重写以实现特定的碰撞行为。
     *
     * @param player 与此实体碰撞的玩家
     */
    virtual void onCollideWithPlayer(class Player& player)
    {
        // 默认实现：无操作
        (void)player;
    }

    // ========== 玩家交互 ==========

    /**
     * @brief 处理玩家初始交互
     *
     * 当玩家右键点击实体时首先调用此方法。
     * 子类可重写此方法处理特定的交互行为（如骑乘、打开容器等）。
     *
     * 基类默认实现返回 Pass，表示不处理交互。
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    virtual ActionResultType processInitialInteract(class Player& player, Hand hand);

    /**
     * @brief 处理玩家指定位置的交互
     *
     * 当玩家右键点击实体的特定位置时调用。
     * 基类默认调用 processInitialInteract。
     * 子类可重写此方法处理基于点击位置的交互（如盔甲架装备槽）。
     *
     * hitPosition 是相对于实体坐标的局部坐标（0 到 实体尺寸 的范围），
     * 可用于确定玩家点击的是实体的哪个部位。
     *
     * @param player 与此实体交互的玩家
     * @param hitPosition 点击位置（相对于实体坐标系）
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    virtual ActionResultType applyPlayerInteraction(class Player& player, const Vector3& hitPosition, Hand hand);

    // ========== 环境检测 ==========

    /**
     * @brief 检查实体是否在水中
     *
     * 读 EnvironmentStateComponent.inWater（由 ecs::sys::environmentSensing 每帧重写）。
     * 需要世界引用才能正常工作——system 遍历碰撞箱内方块流体计算浸入状态。
     */
    [[nodiscard]] virtual bool isInWater() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->inWater : false;
    }

    /**
     * @brief 检查实体是否在岩浆中
     *
     * 读 EnvironmentStateComponent.inLava（由 ecs::sys::environmentSensing 每帧重写）。
     */
    [[nodiscard]] virtual bool isInLava() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->inLava : false;
    }

    /**
     * @brief 检查实体是否在雨中
     *
     * 需要满足：世界正在下雨 + 实体位置可以看到天空 + 生物群系允许降水
     *
     * @return 如果实体在雨中返回 true
     */
    [[nodiscard]] bool isInRain() const;

    /**
     * @brief 检查实体是否湿润
     *
     * 用于三叉戟激流附魔、末影人躲避等逻辑
     *
     * @return 如果实体在水中或雨中返回 true
     */
    [[nodiscard]] bool isWet() const { return isInWater() || isInRain(); }

    /**
     * @brief 检查眼睛是否在水下
     *
     * 用于判断是否可以游泳、是否消耗氧气等
     *
     * @return 如果眼睛位置在水方块中返回 true
     */
    [[nodiscard]] bool areEyesInWater() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->eyesInWater : false;
    }

    /**
     * @brief 检查眼睛是否在岩浆中
     */
    [[nodiscard]] bool areEyesInLava() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->eyesInLava : false;
    }

    /**
     * @brief 检查是否可以游泳
     *
     * 用于判断游泳姿态切换
     *
     * @return 如果可以游泳返回 true
     */
    [[nodiscard]] bool canSwim() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? (c->eyesInWater && c->inWater) : false;
    }

    /**
     * @brief 检查实体是否可以在液体中生成
     *
     * 对应 MC 原版 Entity.canSpawnInLiquids()。
     * 大多数实体返回 false（不能在液体中生成），
     * 溺尸等水生实体重写返回 true。
     *
     * @return 如果可以在液体中生成返回 true
     */
    [[nodiscard]] virtual bool canSpawnInLiquids() const { return false; }

    /**
     * @brief 获取实体眼睛位置的亮度
     *
     * 用于判断怪物是否在阳光下燃烧等。
     *
     * @return 亮度值 (0.0 - 1.0)，如果世界不存在返回 0.0
     */
    [[nodiscard]] f32 getBrightness() const;

    /**
     * @brief 检查实体是否在梯子或藤蔓上
     *
     * 检查实体所在位置的方块是否为可攀爬方块。
     * 可攀爬方块包括：梯子、藤蔓、脚手架。
     *
     * @return 如果实体在可攀爬方块上返回 true
     */
    [[nodiscard]] virtual bool isOnLadder() const;

    /**
     * @brief 获取最后攀爬位置
     *
     * 当实体在攀爬方块（梯子、藤蔓、脚手架等）上时，
     * 记录攀爬位置。用于摔落死亡消息的生成。
     *
     * @return 攀爬位置，如果没有攀爬则返回空
     */
    [[nodiscard]] const std::optional<BlockPos>& getLastClimbPos() const { return m_lastClimbPos; }

    /**
     * @brief 设置最后攀爬位置
     * @param pos 攀爬位置
     */
    void setLastClimbPos(const BlockPos& pos) { m_lastClimbPos = pos; }

    /**
     * @brief 清空最后攀爬位置
     *
     * 在实体落地时调用。
     */
    void clearLastClimbPos() { m_lastClimbPos = std::nullopt; }

    /**
     * @brief 获取实体浸入流体的最大高度
     * @return 流体高度（max(waterHeight, lavaHeight)），如果不在流体中返回 0
     */
    [[nodiscard]] virtual f32 getFluidHeight() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->fluidHeight : 0.0f;
    }

    /**
     * @brief 获取水浸入高度
     * @return 水浸入高度（0.0 起）
     */
    [[nodiscard]] f32 waterHeight() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->waterHeight : 0.0f;
    }

    /**
     * @brief 获取岩浆浸入高度
     * @return 岩浆浸入高度（0.0 起）
     */
    [[nodiscard]] f32 lavaHeight() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EnvironmentStateComponent>();
        return c != nullptr ? c->lavaHeight : 0.0f;
    }

    /**
     * @brief 检查是否能看见另一个实体
     *
     * 通过射线检测判断视线是否被方块阻挡。
     *
     * @param other 目标实体
     * @return 如果视线未被阻挡返回true
     */
    [[nodiscard]] virtual bool canSee(const Entity& other) const;

    /**
     * @brief 检查实体是否着火
     *
     * 火焰免疫的实体永远不会被认为着火。
     */
    [[nodiscard]] bool isOnFire() const { return !isImmuneToFire() && getRemainingFireTicks() > 0; }

    /**
     * @brief 获取剩余着火时间（tick）
     *
     * 正值表示燃烧剩余时间，负值表示火焰免疫期倒计时。
     * 对应 MC Java 的 getRemainingFireTicks()。
     */
    [[nodiscard]] i32 getRemainingFireTicks() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::FireComponent>();
        return c != nullptr ? c->m_fire : 0;
    }

    /**
     * @brief 设置剩余着火时间
     *
     * 直接设置火焰计时器值，不做任何检查。
     * 正值表示燃烧剩余时间，负值表示火焰免疫期倒计时。
     * 对应 MC Java 的 setRemainingFireTicks(int)。
     *
     * @param ticks 火焰计时器值
     */
    void setRemainingFireTicks(i32 ticks)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::FireComponent>()) {
            c->m_fire = ticks;
        }
    }

    /**
     * @brief 获取着火时间（tick）
     * @deprecated 使用 getRemainingFireTicks() 替代
     */
    [[nodiscard]] i32 fire() const { return getRemainingFireTicks(); }

    /**
     * @brief 获取火焰计时器
     * @deprecated 使用 getRemainingFireTicks() 替代
     */
    [[nodiscard]] i32 getFireTimer() const { return getRemainingFireTicks(); }

    /**
     * @brief 点燃实体指定秒数
     *
     * 将秒数转换为 tick 数（1 秒 = 20 tick），然后调用 igniteForTicks()。
     * 仅在新燃烧时间大于当前剩余时间时才会更新，不会覆盖免疫期。
     * 同时清除冰冻状态。
     *
     * @param seconds 燃烧时间（秒）
     */
    void igniteForSeconds(f32 seconds) { igniteForTicks(static_cast<i32>(seconds * 20.0f)); }

    /**
     * @brief 点燃实体指定 tick 数
     *
     * 仅在新燃烧时间大于当前剩余时间时才会更新。
     * 如果当前处于火焰免疫期（m_fire < 0），只有新值大于当前负值时才会覆盖。
     * 同时清除冰冻状态。
     *
     * @param ticks 燃烧时间（tick）
     */
    void igniteForTicks(i32 ticks)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::FireComponent>()) {
            if (c->m_fire < ticks) {
                c->m_fire = ticks;
            }
        }
        clearFreeze();
    }

    /**
     * @brief 设置着火时间
     *
     * @deprecated 使用 igniteForTicks() 或 igniteForSeconds() 替代
     *
     * 仅在新燃烧时间大于当前剩余时间时才会更新。
     * 如果当前处于火焰免疫期（m_fire < 0），只有新值大于当前负值时才会覆盖。
     *
     * @param ticks 燃烧时间（tick）
     */
    void setFire(i32 ticks)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::FireComponent>()) {
            if (c->m_fire < ticks) {
                c->m_fire = ticks;
            }
        }
    }

    /**
     * @brief 强制设置火焰计时器
     *
     * 直接设置火焰计时器值，不检查当前值。
     * 用于增加/减少火焰时间，包括设置为负值（表示火焰免疫期）。
     * Player 重写此方法以限制创造模式下的燃烧时间。
     *
     * @param ticks 火焰计时器值
     */
    virtual void forceFireTicks(i32 ticks)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::FireComponent>()) {
            c->m_fire = ticks;
        }
    }

    /**
     * @brief 清除冰冻状态
     *
     * 当实体被点燃时调用，清除冰冻效果。
     * 基类实现将冰冻计时器重置为 0。
     * LivingEntity 重写此方法以额外移除冰冻减速修饰符。
     */
    virtual void clearFreeze() { setTicksFrozen(0); }

    /**
     * @brief 获取冰冻计时器值
     *
     * 返回实体当前累积的冰冻 tick 数。
     * 当实体处于细雪方块中时每 tick +1，不在细雪中时每 tick -2。
     * 值达到 getTicksRequiredToFreeze() 时实体完全冰冻。
     *
     * @return 冰冻计时器值
     */
    [[nodiscard]] i32 getTicksFrozen() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::FreezeComponent>();
        return c != nullptr ? c->m_ticksFrozen : 0;
    }

    /**
     * @brief 设置冰冻计时器值
     *
     * 同时同步到数据管理器用于客户端同步。
     *
     * @param ticks 冰冻计时器值
     */
    void setTicksFrozen(i32 ticks)
    {
        // FreezeComponent 为真相源，DATA_TICKS_FROZEN_PARAM 退为同步镜像。
        if (auto* c = m_entityContext->tryGetComponent<ecs::FreezeComponent>()) {
            c->m_ticksFrozen = ticks;
        }
        m_dataManager.set(DATA_TICKS_FROZEN_PARAM, ticks);
    }

    /**
     * @brief 获取完全冰冻所需的 tick 数
     *
     * 默认值为 140 tick（7 秒）。
     *
     * @return 完全冰冻所需的 tick 数
     */
    [[nodiscard]] virtual i32 getTicksRequiredToFreeze() const { return BASE_TICKS_REQUIRED_TO_FREEZE; }

    /**
     * @brief 获取冰冻百分比（0.0 ~ 1.0）
     *
     * 用于渲染冰冻动画效果和计算冰冻减速修饰符。
     *
     * @return 冰冻百分比
     */
    [[nodiscard]] f32 getPercentFrozen() const
    {
        const i32 required = getTicksRequiredToFreeze();
        return static_cast<f32>(std::min(getTicksFrozen(), required)) / static_cast<f32>(required);
    }

    /**
     * @brief 检查实体是否完全冰冻
     *
     * 当冰冻计时器达到 getTicksRequiredToFreeze() 时返回 true。
     * 完全冰冻的实体每 40 tick 受到 1.0 冰冻伤害。
     *
     * @return 是否完全冰冻
     */
    [[nodiscard]] bool isFullyFrozen() const { return getTicksFrozen() >= getTicksRequiredToFreeze(); }

    /**
     * @brief 检查实体是否正在冰冻中
     *
     * 当冰冻计时器 > 0 时返回 true。
     *
     * @return 是否正在冰冻
     */
    [[nodiscard]] bool isFreezing() const { return getTicksFrozen() > 0; }

    /**
     * @brief 检查实体是否可以冰冻
     *
     * 检查实体类型是否不在冰冻免疫标签中。
     * LivingEntity 重写此方法以额外检查皮革护甲。
     *
     * @return 是否可以冰冻
     */
    [[nodiscard]] virtual bool canFreeze() const;

    /**
     * @brief 设置实体是否处于细雪中
     *
     * 每帧在 baseTick() 开始时重置为 false，
     * 由细雪方块的 onEntityCollision() 设置为 true。
     *
     * @param inPowderSnow 是否在细雪中
     */
    void setIsInPowderSnow(bool inPowderSnow)
    {
        if (auto* c = m_entityContext->tryGetComponent<ecs::FreezeComponent>()) {
            c->m_isInPowderSnow = inPowderSnow;
        }
    }

    /**
     * @brief 检查实体是否处于细雪中
     *
     * @return 是否在细雪中
     */
    [[nodiscard]] bool isInPowderSnow() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::FreezeComponent>();
        return c != nullptr && c->m_isInPowderSnow;
    }

    /** @brief 冰冻所需的基础 tick 数（140 tick = 7 秒） */
    static constexpr i32 BASE_TICKS_REQUIRED_TO_FREEZE = 140;

    /** @brief 完全冰冻时伤害频率（40 tick = 2 秒） */
    static constexpr i32 FREEZE_HURT_FREQUENCY = 40;

    /**
     * @brief 获取火焰免疫期时长（tick）
     *
     * 返回实体在火焰熄灭后获得的短暂免疫期（负值火焰计时器的绝对值）。
     * 基类返回 0（无免疫期），Player 重写返回 20（1 秒免疫期）。
     *
     * @return 免疫期 tick 数
     */
    [[nodiscard]] virtual i32 getFireImmuneTicks() const { return 0; }

    /**
     * @brief 检查是否免疫火焰
     *
     * 默认实现查询实体类型的火焰免疫标志。
     * 子类可重写以提供运行时可变的免疫状态。
     *
     * @return 如果免疫火焰返回 true
     */
    [[nodiscard]] virtual bool isImmuneToFire() const;

    /**
     * @brief 岩浆点燃实体
     *
     * 将实体点燃 15 秒（300 ticks）。如果实体免疫火焰则不点燃。
     * 在岩浆方块碰撞时调用。
     */
    void lavaIgnite();

    /**
     * @brief 对实体造成岩浆伤害并播放灼烧音效
     *
     * 对非火焰免疫的实体造成 4.0 点岩浆伤害。
     * 如果伤害成功且 shouldPlayLavaHurtSound() 返回 true 且实体未静音，
     * 播放 GENERIC_BURN 音效。
     */
    void lavaHurt();

    /**
     * @brief 判断是否应播放岩浆受伤音效
     *
     * 基类实现始终返回 true。子类可重写以限制音效播放频率。
     * 例如 ItemEntity 重写此方法，仅在生命值归零或每 10 tick 播放一次音效，
     * 避免物品在岩浆中每 tick 都播放音效造成噪音。
     *
     * @return 如果应播放岩浆受伤音效返回 true
     */
    [[nodiscard]] virtual bool shouldPlayLavaHurtSound() const { return true; }

    /**
     * @brief 清除火焰（将火焰计时器设为不超过 0）
     *
     * 对应 MC Java 的 clearFire()。
     * 如果当前火焰计时器为正数，设为 0；如果已为负数（火焰免疫期），保持不变。
     */
    void clearFire();

    /**
     * @brief 熄灭火焰并播放灭火音效
     *
     * 对应 MC Java 的 extinguishFire()。
     * 如果实体正在燃烧，先播放灭火音效，然后调用 clearFire()。
     */
    void extinguishFire();

    /**
     * @brief 播放实体灭火音效
     *
     * 在实体火焰被水或雨熄灭时播放 GENERIC_EXTINGUISH_FIRE 音效。
     * 仅在服务端调用，音效会广播给附近玩家。
     */
    void playExtinguishSound();

    /**
     * @brief 设置火焰免疫期倒计时
     *
     * 当实体的火焰被熄灭时（例如离开火方块、进入水中、被雨淋），
     * 调用此方法设置一个短暂的免疫期，防止实体立即被重新点燃。
     * 免疫期长度由 getFireImmuneTicks() 决定。
     * 基类 Entity 返回 0（不设置免疫期），Player 返回 20 tick（1 秒）。
     */
    void setFireImmunityCooldown();

    // ========== 空气管理 ==========

    /**
     * @brief 获取空气值
     */
    [[nodiscard]] i32 air() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_air;
    }

    /**
     * @brief 设置空气值
     */
    void setAir(i32 air);

    /**
     * @brief 获取最大空气值
     */
    [[nodiscard]] virtual i32 maxAir() const { return 300; }

    // ========== 受伤标记 ==========

    /**
     * @brief 标记实体已受伤（需要同步速度到客户端）
     *
     * 在实体受到带冲击力的伤害或被施加击退时调用，设置 m_hurtMarked = true。
     * 服务端在同步实体速度后将其重置为 false。
     * 对应 MC Java 的 Entity.hurtMarked 字段。
     * 在 MC Java 中，此标记用于两个目的：
     * 1. 在 ServerEntity.sendDirtyEntityData() 中触发速度同步包（ClientboundSetEntityMotionPacket）
     * 2. 在 AI 目标中检测实体是否处于刚被击退的状态（如 TradeWithPlayerGoal）
     */
    void markHurt() { m_hurtMarked = true; }

    /**
     * @brief 检查实体是否被标记为已受伤
     *
     * @return 如果实体需要速度同步返回 true
     */
    [[nodiscard]] bool isHurtMarked() const { return m_hurtMarked; }

    /**
     * @brief 清除受伤标记
     *
     * 在服务端发送速度同步包后调用，将 m_hurtMarked 重置为 false。
     * 对应 MC Java 的 ServerEntity.sendDirtyEntityData() 中 hurtMarked = false。
     */
    void clearHurtMarked() { m_hurtMarked = false; }

    // ========== 无敌 ==========

    /**
     * @brief 检查是否无敌
     */
    [[nodiscard]] bool isInvulnerable() const { return m_invulnerable; }

    /**
     * @brief 设置无敌状态
     */
    void setInvulnerable(bool invulnerable) { m_invulnerable = invulnerable; }

    /**
     * @brief 判断该实体是否应忽略（不受）某次爆炸影响
     *
     * 默认返回 false（受爆炸影响）。掉落物/盔甲架/悬挂实体/载具等"方块类实体"
     * 依据上下文重写此方法：在 mobGriefing 关闭或爆炸不破坏方块等条件下跳过爆炸。
     *
     * 上下文字段语义见 ExplosionImmunityContext。
     *
     * @param ctx 爆炸免疫判定上下文
     * @return true 表示该实体忽略此次爆炸（不受伤、不受击退）
     */
    [[nodiscard]] virtual bool ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const
    {
        MC_UNUSED(ctx);
        return false;
    }

    /**
     * @brief 检查实体是否可以在指定位置与方块交互
     *
     * 用于冒险模式下的交互权限判断。默认实现返回 true（允许交互）。
     * Player 类重写此方法，在旁观模式下禁止交互，在冒险模式下检查
     * 手持物品的 CanPlaceOn NBT 标签来判断是否允许放置。
     * ProjectileEntity 重写此方法，委托给发射者的 mayInteract 或检查 MOB_GRIEFING 游戏规则。
     *
     * @param world 世界引用
     * @param pos 目标方块位置
     * @return 如果允许交互返回 true
     */
    [[nodiscard]] virtual bool mayInteract(IWorld& world, const BlockPos& pos) const
    {
        (void)world;
        (void)pos;
        return true;
    }

    /**
     * @brief 受伤入口方法
     *
     * 处理实体受伤的通用逻辑。基类实现检查无敌状态。
     * 子类（如 LivingEntity）应重写此方法以实现具体的受伤逻辑。
     *
     * @param source 伤害来源
     * @param amount 伤害量
     * @return 是否成功造成伤害
     */
    virtual bool hurt(DamageSource& source, f32 amount);

    /**
     * @brief 检查是否对特定伤害类型免疫
     *
     * @param source 伤害来源
     * @return 如果免疫该伤害类型返回 true
     */
    [[nodiscard]] virtual bool isInvulnerableTo(DamageSource& source) const;

    // ========== 自定义名称 ==========

    /**
     * @brief 获取自定义名称组件
     * @return 自定义名称组件指针，如果没有返回 nullptr
     */
    [[nodiscard]] const text::ITextComponent* getCustomNameComponent() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_customName.get();
    }

    /**
     * @brief 获取自定义名称的纯文本
     * @return 自定义名称纯文本，如果没有返回空字符串
     */
    [[nodiscard]] std::string customNameText() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_customName ? c->m_customName->getUnformattedText() : std::string();
    }

    /**
     * @brief 检查是否有自定义名称
     * @return 如果有自定义名称返回true
     */
    [[nodiscard]] bool hasCustomName() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_customName != nullptr;
    }

    /**
     * @brief 获取显示名称
     *
     * 返回自定义名称组件，如果没有则返回默认名称。
     *
     * @return 显示名称组件
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getDisplayName() const;

    /**
     * @brief 设置自定义名称组件
     * @param name 名称组件（所有权转移）
     */
    void setCustomNameComponent(std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 设置自定义名称（纯文本，向后兼容）
     * @param name 名称字符串
     */
    void setCustomName(const std::string& name);

    /**
     * @brief 检查自定义名称是否可见
     */
    [[nodiscard]] bool isCustomNameVisible() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_customNameVisible;
    }

    /**
     * @brief 设置自定义名称可见性
     */
    void setCustomNameVisible(bool visible);

    // ========== 静音 ==========

    /**
     * @brief 检查是否静音
     */
    [[nodiscard]] bool isSilent() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_silent;
    }

    /**
     * @brief 设置静音状态
     */
    void setSilent(bool silent);

    // ========== 重力 ==========

    /**
     * @brief 检查是否受重力影响
     */
    [[nodiscard]] bool hasNoGravity() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::EntityStateComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_noGravity;
    }

    /**
     * @brief 设置是否受重力影响
     */
    void setNoGravity(bool noGravity);

    // ========== 实体标签 ==========

    /**
     * @brief 获取实体的所有标签
     *
     * 标签用于命令系统和数据包谓词。
     *
     * @return 标签集合的常量引用
     */
    [[nodiscard]] const std::set<std::string>& getTags() const { return m_tags; }

    /**
     * @brief 添加标签
     *
     * 每个实体最多可以有 1024 个标签。
     *
     * @param tag 标签名称
     * @return 如果成功添加返回 true（标签不存在且未达到上限）
     */
    bool addTag(const std::string& tag);

    /**
     * @brief 移除标签
     *
     * @param tag 标签名称
     * @return 如果成功移除返回 true（标签存在）
     */
    bool removeTag(const std::string& tag);

    /**
     * @brief 检查是否拥有指定标签
     *
     * @param tag 标签名称
     * @return 如果拥有该标签返回 true
     */
    [[nodiscard]] bool hasTag(const std::string& tag) const;

    /**
     * @brief 获取标签数量
     *
     * @return 当前标签数量
     */
    [[nodiscard]] size_t getTagCount() const { return m_tags.size(); }

    /**
     * @brief 清空所有标签
     */
    void clearTags() { m_tags.clear(); }

    // ========== 运动速度乘数（甜浆果丛等减速效果） ==========

    /**
     * @brief 设置运动速度乘数
     *
     * 用于甜浆果丛、蜘蛛网等减速效果。
     * 每帧在实体移动前，速度会乘以这个乘数。
     * 退出减速区域时自动清除。
     *
     * @param multiplier 速度乘数 (x, y, z 分量)
     */
    void setMotionMultiplier(const Vector3& multiplier)
    {
        m_motionMultiplier = multiplier;
        m_hasMotionMultiplier = true;
    }

    /**
     * @brief 清除运动速度乘数
     *
     * 当实体退出减速方块时调用。
     */
    void clearMotionMultiplier()
    {
        m_motionMultiplier = Vector3(1.0f, 1.0f, 1.0f);
        m_hasMotionMultiplier = false;
    }

    /**
     * @brief 检查是否有运动速度乘数
     */
    [[nodiscard]] bool hasMotionMultiplier() const { return m_hasMotionMultiplier; }

    /**
     * @brief 获取运动速度乘数
     */
    [[nodiscard]] const Vector3& motionMultiplier() const { return m_motionMultiplier; }

    // ========== 摔落伤害 ==========

    /**
     * @brief 处理摔落伤害（使用默认伤害来源）
     * @param distance 摔落距离
     * @param damageMultiplier 伤害倍率
     *
     * 使用默认的摔落伤害来源（DamageSources::fall()）。
     * 内部委托给 causeFallDamage(distance, damageMultiplier, DamageSources::fall())。
     * 如需自定义伤害来源，使用 causeFallDamage()。
     *
     * 注意：Entity 基类的 causeFallDamage 仅传播给乘客，不施加伤害。
     * LivingEntity 重写 causeFallDamage 以实际计算和施加摔落伤害。
     * handleFallDamage 主要用于 LivingEntity::handleFallDamage 中将默认伤害来源
     * 转换为 causeFallDamage 调用的便捷方法。
     */
    virtual void handleFallDamage(f32 distance, f32 damageMultiplier);

    /**
     * @brief 使用自定义伤害来源处理摔落伤害
     * @param distance 摔落距离
     * @param damageMultiplier 伤害倍率
     * @param source 伤害来源
     *
     * Block::onFallenUpon 默认实现调用此方法施加摔落伤害。
     * 方块可以重写 onFallenUpon 以自定义摔落行为：
     * - 石笋方块：调用 causeFallDamage 并传入 DamageSources::stalagmite()，不调用父类（替代普通摔落伤害）
     * - 耕地方块：先执行踩踏逻辑，再调用父类 onFallenUpon（保留普通摔落伤害）
     * - 海龟蛋方块：先执行踩破逻辑，再调用父类 onFallenUpon（保留普通摔落伤害）
     *
     * 基类实现仅调用 propagateFallToPassengers 将摔落伤害传播给所有乘客（不对自身施加伤害）。
     * LivingEntity 重写此方法：先调用 Entity::causeFallDamage（传播给乘客），
     * 然后使用自定义伤害来源对自身计算伤害。
     * 参考: MC Entity.causeFallDamage → propagateFallToPassengers
     */
    virtual void causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source);

    /**
     * @brief 将摔落伤害传播给所有乘客
     * @param distance 摔落距离
     * @param damageMultiplier 伤害倍率
     * @param source 伤害来源
     *
     * 当载具受到摔落伤害时，所有乘客也受到相同的摔落伤害。
     * 参考: MC Entity.propagateFallToPassengers
     */
    void propagateFallToPassengers(f32 distance, f32 damageMultiplier, const DamageSource& source);

    /**
     * @brief 更新摔落距离
     * 在移动时调用，跟踪摔落距离以便着地时计算伤害。
     * 着地时调用 Block::onFallenUpon，由方块决定摔落伤害类型和大小。
     */
    void updateFallDistance();

private:
    /**
     * @brief 着地时触发踩上方块的 onFallenUpon 回调
     *
     * 调用实体脚下方块的 onFallenUpon 方法。
     * Block::onFallenUpon 默认实现会调用 entity.causeFallDamage() 施加普通摔落伤害。
     */
    void _handleLandingOnBlock();

public:
    // ========== 闪电击中 ==========

    /**
     * @brief 当实体被闪电击中时调用
     *
     * 子类可以重写此方法来处理被闪电击中的特殊效果：
     * - 哞菇：红色 -> 棕色
     * - 苦力怕：变成高压苦力怕
     * - 村民：变成女巫
     * - 猪：变成僵尸猪灵
     *
     * 注意：闪电伤害由 LightningBoltEntity 单独处理，
     * 此方法用于处理特殊变形效果。
     */
    virtual void onStruckByLightning() {}

    /**
     * @brief 被爆炸击中时调用
     *
     * 在爆炸对实体施加击退和伤害之后调用，允许实体对爆炸做出额外响应。
     * 默认实现为空操作。
     *
     * Player 重写此方法以设置冲量上下文（impulse context），
     * 当爆炸由风弹引起时启用坠落伤害免疫。
     *
     * @param cause 引起爆炸的实体，可能为 nullptr（如床爆炸等无来源爆炸）
     */
    virtual void onExplosionHit(Entity* cause) { (void)cause; }

    // ========== 乘客/骑乘系统 ==========

    /**
     * @brief 获取乘客列表
     * @return 乘客实体ID列表
     */
    [[nodiscard]] const std::vector<EntityInstanceId>& getPassengers() const { return m_passengers; }

    /**
     * @brief 检查是否有乘客
     */
    [[nodiscard]] bool hasPassengers() const { return !m_passengers.empty(); }

    /**
     * @brief 检查是否被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const { return hasPassengers(); }

    /**
     * @brief 获取所骑乘的车辆
     * @return 车辆实体ID，如果没有则返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId getVehicle() const { return m_vehicle; }

    /**
     * @brief 检查是否正在骑乘
     */
    [[nodiscard]] bool isRiding() const { return m_vehicle != INVALID_ENTITY_ID; }

    /**
     * @brief 检查指定实体是否是乘客
     * @param entityId 实体ID
     */
    [[nodiscard]] bool isPassenger(EntityInstanceId entityId) const;

    /**
     * @brief 添加乘客到本载具
     *
     * 仅操作乘客列表，不进行循环检测。
     * 循环检测由 startRiding() 负责。
     *
     * 前置条件：passenger.getVehicle() 必须等于 this.id()，
     * 即 passenger 必须已经通过 setVehicle() 关联到此载具。
     * 这与 MC Java 的行为一致：startRiding 先设置 vehicle 字段，再调用 addPassenger。
     * 如果 passenger.getVehicle() != this.id()，则触发断言并返回 false，
     * 对齐 MC Java 的 IllegalStateException 行为。
     *
     * 不应直接调用此方法，应使用 startRiding()。
     *
     * @param passenger 乘客实体
     * @return 是否成功添加
     */
    bool addPassenger(Entity& passenger);

    /**
     * @brief 移除乘客
     *
     * 从乘客列表中移除指定实体。调用者应先清空 passenger 的 vehicle 引用
     * （通过 stopRiding/dismount），然后再调用此方法。
     * 对齐 MC Java：如果 passenger.getVehicle() 仍指向 this，
     * 说明调用顺序错误，会触发断言。
     *
     * @param passenger 乘客实体
     */
    void removePassenger(Entity& passenger);

    /**
     * @brief 开始骑乘载具
     *
     * 1. 循环检测（从载具沿 vehicle 链向上遍历）
     * 2. 检查 couldAcceptPassenger / canBeRidden / canAddPassenger
     * 3. 如已在骑乘则先停止
     * 4. 设置 m_vehicle = vehicle.id()（先于 addPassenger）
     * 5. 调用 vehicle.addPassenger(*this)
     *
     * @param vehicle 载具实体
     * @return 是否成功骑乘
     */
    bool startRiding(Entity& vehicle);

    /**
     * @brief 停止骑乘
     */
    void stopRiding();

    /**
     * @brief 下马（核心下车逻辑）
     *
     * 内部方法，被 stopRiding() 调用
     */
    void dismount();

    /**
     * @brief 移除所有乘客
     *
     * 从后向前遍历乘客列表并调用每个乘客的 stopRiding()
     */
    void removePassengers();

    /**
     * @brief 检查是否可以被指定实体骑乘
     * @param vehicle 载具实体
     * @return 如果可以骑乘返回true
     *
     * 默认检查：不在潜行状态 + 骑乘冷却为0
     */
    [[nodiscard]] virtual bool canBeRidden(const Entity& vehicle) const;

    /**
     * @brief 检查是否可以在水中骑乘
     * @return 如果可以在水中骑乘返回true
     *
     * 默认返回true，LivingEntity重写返回false
     */
    [[nodiscard]] virtual bool canBeRiddenInWater() const { return true; }

    /**
     * @brief 检查此载具实体是否在水中强制乘客下坐骑
     * @return 如果乘客在水中会被强制下坐骑返回true
     *
     * MC Java 中此方法委托给 EntityType 标签检查：
     * return this.getType().is(EntityTypeTags.DISMOUNTS_UNDERWATER)
     * 马、猪、骆驼等陆地骑乘实体返回true，船返回false。
     * 当乘客的眼睛位置在水中且所骑乘的载具返回true时，
     * LivingEntity::updateAirSupply() 会调用 stopRiding() 强制下坐骑。
     */
    [[nodiscard]] virtual bool dismountsUnderwater() const;

    /**
     * @brief 检查是否与指定实体骑乘同一载具
     * @param other 其他实体
     * @return 如果骑乘同一载具返回true
     */
    [[nodiscard]] bool isRidingSameEntity(const Entity& other) const;

    /**
     * @brief 获取最底层的骑乘实体
     * @return 最底层载具的指针，如果没有骑乘返回nullptr
     *
     * 沿着骑乘链向下遍历直到找到最底层的载具
     */
    [[nodiscard]] Entity* getLowestRidingEntity();
    [[nodiscard]] const Entity* getLowestRidingEntity() const;

    /**
     * @brief 检查是否与指定实体存在骑乘关系（双向）
     *
     * 向下搜索：other 是否是 this 的间接乘客（other 骑乘 this 或 this 的乘客）
     * 向上搜索：other 是否是 this 的间接载具（this 骑乘 other 或 other 是 this 的载具链的一部分）
     *
     * @param other 目标实体
     * @return 如果存在骑乘关系返回true
     */
    [[nodiscard]] bool isRidingOrBeingRiddenBy(const Entity& other) const;

    /**
     * @brief 分离所有乘客和载具
     *
     * 同时移除所有乘客并下车
     */
    void detach();

    /**
     * @brief 是否有待挂载的乘客 NBT
     *
     * 反序列化阶段（EntityDeserializer::deserialize）遇到 Passengers 标签时，
     * 不会立即 spawn 乘客（因为主实体尚未 spawn，id 仍为 0，此时 spawn 乘客会导致
     * 乘客的 m_vehicle 被记为 0，后续主实体 spawn 时骑乘关系失效）。
     * 而是把 Passengers 列表暂存到 m_pendingPassengersNbt，等主实体被 spawnEntity
     * 注入世界、拿到真实 id 后，由 EntityDeserializer::attachPassengers 递归处理。
     *
     * @return 是否有待挂载的乘客 NBT
     */
    [[nodiscard]] bool hasPendingPassengersNbt() const { return !m_pendingPassengersNbt.empty(); }

    /**
     * @brief 取走待挂载的乘客 NBT 列表
     *
     * 调用方（通常是 EntityDeserializer::attachPassengers）取走后，本实体的
     * m_pendingPassengersNbt 会被清空。多次调用第二次起返回空列表。
     *
     * @return 待挂载的乘客 NBT 列表（按原 Passengers 列表顺序）
     */
    std::vector<nbt::tags::compound_tag> takePendingPassengersNbt() { return std::move(m_pendingPassengersNbt); }

    /**
     * @brief 追加一条待挂载乘客 NBT（内部方法）
     *
     * 仅由 EntityDeserializer::deserialize 调用，用于把 Passengers 列表中的
     * 单个乘客 NBT 暂存到 m_pendingPassengersNbt。后续由 attachPassengers 取走处理。
     * 下划线前缀表明这是内部 API，外部业务代码不应调用。
     */
    void _appendPendingPassengerNbt(const nbt::tags::compound_tag& passengerTag)
    {
        m_pendingPassengersNbt.push_back(passengerTag);
    }

    /**
     * @brief 获取第一个乘客
     * @return 第一个乘客的实体ID，如果没有则返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId getFirstPassenger() const
    {
        return m_passengers.empty() ? INVALID_ENTITY_ID : m_passengers.front();
    }

    /**
     * @brief 获取控制乘客（通常是第一个乘客）
     * @return 控制乘客的实体ID，如果没有则返回 INVALID_ENTITY_ID
     *
     * 子类可重写此方法以返回不同的控制乘客
     */
    [[nodiscard]] virtual EntityInstanceId getControllingPassenger() const { return getFirstPassenger(); }

    /**
     * @brief 检查是否可以由乘客控制方向
     * @return 如果可以被乘客控制返回true
     */
    [[nodiscard]] virtual bool canBeSteered() const { return false; }

    /**
     * @brief 获取骑乘时的乘客数量限制
     * @return 最大乘客数量
     */
    [[nodiscard]] virtual i32 getMaxPassengers() const { return 1; }

    /**
     * @brief 检查此实体是否根本可以接受乘客
     *
     * 对应 MC Java 的 Entity.couldAcceptPassenger()。
     * 这是骑乘检查的"硬门槛"——在 canAddPassenger 之前检查，
     * 如果返回 false，则无论如何都无法骑乘此实体。
     *
     * 默认返回 true（可以接受乘客）。
     * 不祥物品生成器（OminousItemSpawner）等不应被骑乘的实体重写返回 false。
     *
     * @return 如果此实体可以接受乘客返回 true
     */
    [[nodiscard]] virtual bool couldAcceptPassenger() const { return true; }

    /**
     * @brief 检查此实体是否可以添加指定乘客
     *
     * 对应 MC Java 的 Entity.canAddPassenger(Entity)。
     * 这是骑乘检查的"软门槛"——在 couldAcceptPassenger 之后检查，
     * 但可以通过强制骑乘绕过。
     *
     * 默认实现：检查当前乘客数量是否小于最大乘客数。
     * MC Java 原版默认为 passengers.isEmpty()（仅限单乘客），
     * 但本项目中 getMaxPassengers() 机制已完善，故默认实现使用
     * passengers.size() < getMaxPassengers() 以自动适配多乘客载具。
     * 需要额外条件（如船检查是否在水下）的子类应重写此方法。
     * 不允许任何乘客的实体应重写 couldAcceptPassenger() 返回 false。
     *
     * @param passenger 待添加的乘客实体
     * @return 如果可以添加指定乘客返回 true
     */
    [[nodiscard]] virtual bool canAddPassenger(const Entity& passenger) const
    {
        (void)passenger;
        return static_cast<i32>(m_passengers.size()) < getMaxPassengers();
    }

    /**
     * @brief 获取载具骑乘高度偏移
     * @return 载具顶部到乘客底部的距离
     *
     * 默认返回实体高度的75%
     */
    [[nodiscard]] virtual f64 getMountedYOffset() const;

    /**
     * @brief 获取乘客Y偏移
     * @return 乘客相对于载具骑乘点的Y偏移
     *
     * Entity默认返回0，Player重写返回-0.35
     */
    [[nodiscard]] virtual f64 getYOffset() const { return 0.0; }

    /**
     * @brief 获取骑乘位置（世界坐标）
     * @return 骑乘位置的世界坐标
     */
    [[nodiscard]] virtual Vector3 getRidingPosition() const;

    /**
     * @brief 更新乘客位置
     *
     * 每帧调用以更新所有乘客的位置
     */
    void updatePassengers();

    /**
     * @brief 更新骑乘实体的状态
     *
     * 当作为乘客时调用，更新位置和旋转
     */
    virtual void updateRidden();

    /**
     * @brief 将载具的朝向应用到乘客
     * @param passenger 乘客实体
     *
     * 用于船等需要同步旋转的载具
     */
    virtual void applyOrientationToEntity(Entity& passenger);

    /**
     * @brief 检查乘客是否可以控制方向
     * @return 如果乘客可以控制载具方向返回true
     */
    [[nodiscard]] bool canPassengerSteer() const;

    /**
     * @brief 获取骑乘冷却时间
     * @return 剩余冷却时间（tick），0表示可以骑乘
     */
    [[nodiscard]] i32 rideCooldown() const { return m_rideCooldown; }

    /**
     * @brief 检查是否可以骑乘（冷却为0）
     */
    [[nodiscard]] bool canRide() const { return m_rideCooldown <= 0; }

    /**
     * @brief 检查实体是否可以更新
     * @return 如果实体可以更新返回true
     *
     * 用于控制乘客是否执行tick更新
     */
    [[nodiscard]] virtual bool canUpdate() const { return true; }

    /**
     * @brief 更新单个乘客的位置
     * @param passenger 乘客实体
     *
     * 子类可重写此方法以自定义乘客位置计算
     */
    virtual void updatePassengerPosition(Entity& passenger);

protected:
    /**
     * @brief 定位乘客（内部方法）
     * @param passenger 乘客实体
     */
    void positionRider(Entity& passenger);

public:
    // ========== 更新 ==========

    /**
     * @brief 基础 tick 更新
     */
    virtual void baseTick();

    /**
     * @brief 将数据参数同步回实体字段
     *
     * 客户端接收元数据包后调用，用于把 DataManager 中的值写回实体成员。
     */
    virtual void syncMetadataFromDataManager();

    // ========== NBT 序列化 ==========

    /**
     * @brief 将实体完整序列化到 NBT
     *
     * 写入位置、运动、旋转、UUID 等基类数据，然后调用 addAdditionalSaveData()。
     *
     * @param tag NBT 复合标签（输出参数）
     */
    void writeToNBT(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 反序列化实体
     *
     * 读取位置、运动、旋转、UUID 等基类数据，然后调用 readAdditionalSaveData()。
     *
     * @param tag NBT 复合标签
     * @return 成功或错误
     */
    Result<void> readFromNBT(const nbt::tags::compound_tag& tag);

    // ========== 方块交互 ==========

    /**
     * @brief 检查实体是否正在潜行
     *
     * 默认返回 false。Player 类重写此方法返回实际的潜行状态。
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] virtual bool isSneaking() const { return false; }

    /**
     * @brief 检查实体是否处于旁观者模式
     *
     * 基类默认返回 false。Player 类重写此方法返回实际的旁观者模式状态。
     * 旁观者模式下的实体不可被弹射物命中、不可冰冻、不可被推挤等。
     * 参考: net.minecraft.world.entity.Entity.isSpectator()
     *
     * @return 如果实体处于旁观者模式返回 true
     */
    [[nodiscard]] virtual bool isSpectator() const { return false; }

    /**
     * @brief 检查实体是否小心行走（潜行状态）
     *
     * 小心行走的实体不会触发 onEntityWalk 回调。
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] virtual bool isSteppingCarefully() const { return isSneaking(); }

    /**
     * @brief 检查实体是否阻尼振动
     *
     * 阻尼振动的实体不会触发振动信号。
     * 监守者始终阻尼振动，掉落的羊毛物品也阻尼振动。
     * 参考: net.minecraft.world.entity.Entity.dampensVibrations()
     *
     * @return 如果实体阻尼振动返回true
     */
    [[nodiscard]] virtual bool dampensVibrations() const { return false; }

    /**
     * @brief 检查实体是否可以触发行走事件
     *
     * 某些实体（如盔甲架、船等）不会触发行走相关事件。
     *
     * @return 默认返回true
     */
    [[nodiscard]] virtual bool canTriggerWalking() const { return true; }

    /**
     * @brief 检查实体是否不触发压力板/绊线
     *
     * 对应 MC Java 的 Entity.isIgnoringBlockTriggers()。
     * 某些实体不会触发压力板和绊线。
     * 默认返回 false（会触发）。
     *
     * 重写返回 true 的实体：蝙蝠、盔甲架（标记模式）、
     * 不祥物品生成器等。
     *
     * 注意：物品实体和投射物不重写此方法——在 MC 原版中，木质/测重
     * 压力板可以检测所有实体（包括物品），而石质压力板通过
     * LivingEntity 类型过滤自动排除非生物实体。
     *
     * @return 如果实体不触发压力板返回true
     */
    [[nodiscard]] virtual bool doesEntityNotTriggerPressurePlate() const { return false; }

    /**
     * @brief 获取实体的比较器输出信号强度
     *
     * 某些实体可以输出模拟红石信号（0-15），供比较器读取。
     * 例如：箱子矿车、漏斗矿车（基于容器填充率）、命令方块矿车（基于成功次数）、物品展示框（基于旋转）。
     * 默认返回 0（无信号）。
     *
     * @return 比较器信号强度 0-15
     */
    [[nodiscard]] virtual i32 getComparatorOutput() const { return 0; }

    /**
     * @brief 播放脚步声
     *
     * 当实体在方块上行走时调用。子类可重写以自定义脚步声。
     *
     * 默认实现使用脚下方块的声音类型播放脚步声，并检查紫水晶共振音效。
     *
     * @param pos 方块位置
     * @param blockState 方块状态
     */
    virtual void playStepSound(const BlockPos& pos, const BlockState* blockState);

    /**
     * @brief 获取主脚步声方块位置
     *
     * 检查脚上方块是否属于 INSIDE_STEP_SOUND_BLOCKS 或 COMBINATION_STEP_SOUND_BLOCKS，
     * 如果是则返回上方方块位置（脚步声应以上方方块为准），
     * 否则返回脚下方块位置。
     *
     * @param pos 脚下方块位置
     * @return 应播放脚步声的方块位置
     */
    [[nodiscard]] BlockPos getPrimaryStepSoundBlockPos(const BlockPos& pos) const;

    /**
     * @brief 播放组合脚步声
     *
     * 同时播放上方方块的正常步声和下方方块的沉闷步声。
     * 用于踩在 COMBINATION_STEP_SOUND_BLOCKS 方块上时（如地毯、雪层等）。
     *
     * @param aboveState 上方方块状态（组合步声方块）
     * @param belowState 下方方块状态
     */
    void playCombinationStepSounds(const BlockState& aboveState, const BlockState& belowState);

    /**
     * @brief 播放沉闷脚步声
     *
     * 以极低音量播放步声，用于组合脚步声中下方方块的音效。
     * 音量 = soundType.volume * 0.05，音调 = soundType.pitch * 0.8
     *
     * @param blockState 方块状态
     */
    void playMuffledStepSound(const BlockState& blockState);

    /**
     * @brief 检查是否应播放紫水晶步声音效
     *
     * 当脚下方块属于 CRYSTAL_SOUND_BLOCKS 且距离上次播放已过 20 tick 冷却时，
     * 应播放紫水晶共振音效。
     *
     * @param blockState 方块状态
     * @return 如果应播放紫水晶音效返回 true
     */
    [[nodiscard]] bool shouldPlayAmethystStepSound(const BlockState& blockState) const;

    /**
     * @brief 播放紫水晶共振铃声
     *
     * 使用惰性衰减模型更新紫水晶声音强度：
     * 1. 用 0.997^elapsed_ticks 补偿自上次播放以来的衰减
     * 2. 累加 0.07 并钳制到 [0, 1]
     * 3. 以强度计算音量和音调，播放 block.amethyst_block.chime
     * 4. 记录当前 tick 为 lastCrystalSoundPlayTick
     */
    void playAmethystStepSound();

    /**
     * @brief 检查实体是否无视碰撞
     *
     * 无视碰撞的实体可以穿过方块，不会触发碰撞检测。
     *
     * @return 如果实体无视碰撞返回true
     */
    [[nodiscard]] bool noClip() const { return m_noClip; }

    /**
     * @brief 设置实体是否无视碰撞
     * @param noClip 是否无视碰撞
     */
    void setNoClip(bool noClip) { m_noClip = noClip; }

    /**
     * @brief 执行方块碰撞回调（无参数版本）
     *
     * 遍历实体碰撞箱覆盖的所有方块，调用方块的 onEntityCollision 方法。
     * 用于在 tick() 中手动触发方块碰撞检测，例如船、矿车等不触发行走的实体。
     */
    void doBlockCollisions();

    /**
     * @brief 执行方块碰撞回调（移动后版本）
     *
     * 在实体移动后调用，处理与方块的交互：
     * - onLanded: 垂直碰撞后着地
     * - onEntityWalk: 在地面上行走
     * - onInsideBlock: 实体进入方块碰撞箱
     *
     * @param actualMovement 实际移动向量
     * @param desiredMovement 期望移动向量
     */
    void doBlockCollisionsAfterMove(const Vector3& actualMovement, const Vector3& desiredMovement);

    /**
     * @brief 当实体进入方块碰撞箱时调用
     *
     * 每帧遍历实体碰撞箱覆盖的所有方块时调用。
     * ServerPlayer 重写此方法触发 EnterBlockTrigger 成就。
     *
     * @param blockState 方块状态
     */
    virtual void onInsideBlock(const BlockState& blockState)
    {
        // 基类空实现，子类可重写
        (void)blockState;
    }

    // toString，用于调试，所有实体统一
    [[nodiscard]] std::string toString() const;

protected:
    /**
     * @brief 根据实体类型和后缀构造声音事件ID
     * @param suffix 声音后缀（例如 ambient、hurt、death）
     * @return 声音事件ID，无效类型返回空
     */
    [[nodiscard]] std::optional<ResourceLocation> makeSoundEventId(std::string_view suffix) const;

    // 对象级内存追踪守卫：绑定本对象地址，ctor 发 alloc、dtor 发 free。Entity 不可移动
    // （EntityDataManager 含 std::mutex），故无需 move 重绑定，ctor 初始化列表绑定 this
    // 即可，一次插桩覆盖所有派生类（LivingEntity/MobEntity/Player 等）。仅 MC_ENABLE_MEMORY
    // && MC_ENABLE_TRACY 时发事件，其余分支空操作。
    ::mc::profiler::TracyObjectTracker<"Entity"> m_memTrack;

    EntityInstanceId m_id;
    std::string m_uuid;   // UUID 字符串
    std::string m_typeId; // 资源标识符（如 minecraft:pig）
    // 缓存的 EntityType 指针，由 entityType() 懒查询填充。mutable 以支持
    // const 方法内的懒查询。指向 EntityRegistry::m_types 内对象，地址稳定。
    // 声明于 m_typeId 之后，与 m_memTrack 布局约束兼容（m_memTrack 须居数据成员前）。
    mutable const entity::EntityType* m_entityType = nullptr;

    // ========== ECS 桥接 ==========
    //
    // m_entityContext：ECS 实体的非拥有视图三元组（registry 引用 + entt registry
    // 引用 + EntityId），由构造时 registry.create() 建立。通过它可查询任意组件
    // （首批4高频组件已由 m_builtIn 缓存裸指针加速，其余组件走 context 查询）。
    //
    // m_builtIn：4 高频组件（StateVector/Velocity/AABBShape/EntityRotation）的裸
    // 指针缓存，构造 attach 后填充。position()/velocity() 等 getter 直接解引用，
    // 避免每次走 entt 查询。指针稳定性契约见 BuiltInEntityComponents.hpp。
    //
    // 原 m_position/m_prevPosition/m_velocity/m_yaw/m_pitch/m_prevYaw/m_prevPitch/
    // m_boundingBox 8 个字段已删除，数据迁入上述组件（零双写硬约束）。
    std::unique_ptr<ecs::EntityContext> m_entityContext;
    ecs::BuiltInEntityComponents m_builtIn{};

    mutable math::Random m_random; ///< 实体随机数生成器，构造时初始化

    // m_onGround 已迁入 PhysicsStateComponent（见 m_builtIn.physicsState->m_onGround）。
    bool m_removed = false;
    bool m_noClip = false;  // 是否无视碰撞（用于三叉戟返回等）
    bool m_glowing = false; // 发光状态（服务端使用）
    // m_pose / m_flags 已迁入 ecs::EntityStateComponent / EntityFlagsComponent（真相源），
    // DATA_POSE_PARAM / DATA_FLAGS_PARAM 退为同步镜像。经 pose()/flags()/setPose()/setFlags()
    // 等读写（见 m_entityContext->tryGetComponent）。
    entity::EntitySize m_dimensions = entity::EntitySize::flexible(0.6f, 1.8f);
    // m_boundingBox 已迁入 AABBShapeComponent（见 m_builtIn.aabbShape->m_aabb）。
    bool m_dimensionsInitialized = false;

    // 物理相关
    PhysicsEngine* m_physicsEngine = nullptr;
    // m_collidedHorizontally/m_collidedVertically/m_fallDistance 已迁入 PhysicsStateComponent
    // （见 m_builtIn.physicsState）。
    f32 m_stepHeight = 0.0f; // 步进高度，默认0.0f，LivingEntity设置为0.6f

    DimensionId m_dimension = 0;
    u32 m_ticksExisted = 0;

    // 传送门相关字段（m_portalCooldown/m_portalTime/m_inPortal/m_portalPos）已迁入
    // ecs::PortalComponent，经 m_entityContext->tryGetComponent 读写。

    // 世界引用
    IWorld* m_world = nullptr;

    // 所属 EntityManager 反向指针（非拥有）：reapplyPosition 经它通知空间索引迁移。
    // nullptr 表示未托管于任何 EntityManager（测试裸实体或 addEntity 前）。
    EntityManager* m_entityManager = nullptr;

    // 数据管理器
    entity::EntityDataManager m_dataManager;

    // 静态数据参数（id 由继承链分配器按 registerData 调用顺序分配，对齐 vanilla 1.21.11
    // Entity 的 defineId 顺序：FLAGS(0)/AIR(1)/CUSTOM_NAME_VISIBLE(2)/CUSTOM_NAME(3)/
    // SILENT(4)/NO_GRAVITY(5)/POSE(6)/TICKS_FROZEN(7)）。
    // CUSTOM_NAME 为 OptionalComponent（serializerId 6），POSE 为 Pose（serializerId 20）。
    static entity::DataParameter<i8> DATA_FLAGS_PARAM;
    static entity::DataParameter<i32> DATA_AIR_PARAM;
    static entity::DataParameter<bool> DATA_CUSTOM_NAME_VISIBLE_PARAM;
    static entity::DataParameter<entity::OptionalComponentValue> DATA_CUSTOM_NAME_PARAM;
    static entity::DataParameter<bool> DATA_SILENT_PARAM;
    static entity::DataParameter<bool> DATA_NO_GRAVITY_PARAM;
    static entity::DataParameter<entity::PoseValue> DATA_POSE_PARAM;
    static entity::DataParameter<i32> DATA_TICKS_FROZEN_PARAM;

    /**
     * @brief 本类的继承链标识（复刻 vanilla ClassTreeIdRegistry）
     *
     * Entity 是继承链根，parent 为 nullptr。子类 classInfo 的 parent 指向其直接
     * 父类的 classInfo()，运行时解引用构建继承链。EntityDataManager 据此沿父链
     * 分配 synched-data 字段 id。
     *
     * 返回静态对象的 const 引用（地址稳定），供 parent 指针存储。
     */
    static const entity::EntityClassInfo& classInfo();

    // 环境状态（inWater/inLava/eyesInWater/eyesInLava/waterHeight/lavaHeight/fluidHeight 七字段）
    // 已迁入 ecs::EnvironmentStateComponent，经 m_entityContext->tryGetComponent 读写，
    // 由 ecs::sys::environmentSensing（SystemPhase::EnvironmentSense 阶段）每帧重写。

    // 冰冻状态
    // m_ticksFrozen / m_isInPowderSnow 已迁入 ecs::FreezeComponent（真相源），
    // DATA_TICKS_FROZEN_PARAM 退为同步镜像。经 getTicksFrozen/setTicksFrozen/
    // isInPowderSnow/setIsInPowderSnow 读写（见 m_entityContext->tryGetComponent）。

    // 攀爬追踪（用于摔落死亡消息）
    std::optional<BlockPos> m_lastClimbPos; // 最后攀爬位置

    // 空气值
    // m_air 已迁入 ecs::EntityStateComponent（真相源），DATA_AIR_PARAM 退为同步镜像。
    // 经 air()/setAir() 读写（见 m_entityContext->tryGetComponent）。

    // 无敌
    bool m_invulnerable = false;

    // 受伤标记（服务端：设为 true 表示需要同步速度到客户端）
    // 对应 MC Java 的 Entity.hurtMarked 字段
    bool m_hurtMarked = false;

    // 自定义名称
    // m_customName / m_customNameVisible 已迁入 ecs::EntityStateComponent（真相源），
    // DATA_CUSTOM_NAME_PARAM / DATA_CUSTOM_NAME_VISIBLE_PARAM 退为同步镜像。
    // 经 getCustomNameComponent()/setCustomName()/setCustomNameVisible() 等读写。

    // 静音
    // m_silent 已迁入 ecs::EntityStateComponent（真相源），DATA_SILENT_PARAM 退为同步镜像。
    // 经 isSilent()/setSilent() 读写。

    // 重力
    // m_noGravity 已迁入 ecs::EntityStateComponent（真相源），DATA_NO_GRAVITY_PARAM 退为同步镜像。
    // 经 hasNoGravity()/setNoGravity() 读写。

    // 实体标签（最多1024个标签）
    std::set<std::string> m_tags;

    // 运动速度乘数（用于甜浆果丛等减速效果）
    Vector3 m_motionMultiplier = Vector3(1.0f, 1.0f, 1.0f);
    bool m_hasMotionMultiplier = false;

    // 紫水晶步声共振
    f32 m_crystalSoundIntensity = 0.0f; ///< 紫水晶共振铃声音量强度 [0, 1]
    i32 m_lastCrystalSoundPlayTick = 0; ///< 上次播放紫水晶铃声的游戏刻

    // 乘客/骑乘系统
    std::vector<EntityInstanceId> m_passengers;     // 乘客列表
    EntityInstanceId m_vehicle = INVALID_ENTITY_ID; // 正在骑乘的车辆
    i32 m_rideCooldown = 0;                         // 骑乘冷却（tick），用于防止快速上下骑乘

    // 反序列化阶段暂存的 Passengers NBT 列表。主实体被 spawnEntity 注入世界、拿到真实 id 后，
    // 由 EntityDeserializer::attachPassengers 递归 spawn 乘客并 startRiding，保证乘客的 m_vehicle
    // 指向主实体的真实 id。详见 hasPendingPassengersNbt() 注释。
    std::vector<nbt::tags::compound_tag> m_pendingPassengersNbt;

    /**
     * @brief 设置车辆（内部方法）
     */
    void setVehicle(EntityInstanceId vehicle) { m_vehicle = vehicle; }

    /**
     * @brief 序列化子类特有数据
     *
     * 子类重写此方法添加自己的持久化数据，必须调用基类实现。
     *
     * @param tag NBT 复合标签（输出参数）
     */
    virtual void addAdditionalSaveData(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 反序列化子类特有数据
     *
     * 子类重写此方法读取自己的持久化数据，必须调用基类实现。
     *
     * @param tag NBT 复合标签
     * @return 成功或错误
     */
    virtual Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag);

    /**
     * @brief 重新应用当前位置到碰撞箱
     */
    void reapplyPosition();
};

} // namespace mc
