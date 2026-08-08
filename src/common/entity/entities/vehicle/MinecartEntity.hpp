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

#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "../../../world/blockentity/core/SimpleInventory.hpp"
#include "../../../world/blockentity/spawner/SpawnerLogic.hpp"
#include "../../../world/blockentity/transport/IHopper.hpp"
#include "../../core/Entity.hpp"
#include "../../core/EntityDataManager.hpp"
#include "../../damage/DamageSource.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;
class Block;
class IWorld;

namespace entity {

// 使用 blocks 命名空间中的 RailShape
using blocks::RailShape;

/**
 * @brief 矿车实体基类
 *
 * 可在铁轨上行驶的车辆。
 * 有多种变体：普通、箱、漏斗、熔炉、TNT、命令方块矿车。
 */
class AbstractMinecartEntity : public Entity {
public:
    /**
     * @brief 矿车类型
     */
    enum class Type : u8 {
        Rideable = 0,    // 普通矿车
        Chest = 1,       // 箱子矿车
        Furnace = 2,     // 熔炉矿车
        TNT = 3,         // TNT矿车
        Spawner = 4,     // 刷怪笼矿车
        Hopper = 5,      // 漏斗矿车
        CommandBlock = 6 // 命令方块矿车
    };

    /**
     * @brief 构造函数
     * @param type 矿车类型
     * @param id 实体ID
     */
    AbstractMinecartEntity(Type type, EntityInstanceId id, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数（用于工厂创建）
     * @param type 矿车类型
     * @param registry ECS 实体注册表
     */
    explicit AbstractMinecartEntity(Type type, ecs::EntityRegistry& registry);

    ~AbstractMinecartEntity() override = default;

    // 禁止拷贝
    AbstractMinecartEntity(const AbstractMinecartEntity&) = delete;
    AbstractMinecartEntity& operator=(const AbstractMinecartEntity&) = delete;

    // 允许移动
    AbstractMinecartEntity(AbstractMinecartEntity&&) = delete;
    AbstractMinecartEntity& operator=(AbstractMinecartEntity&&) = delete;

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 处理伤害
     * @note 矿车不继承LivingEntity，所以不重写hurt方法
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 检查是否可以被碰撞
     */
    [[nodiscard]] bool canBeCollidedWith() const override { return isAlive(); }

    /**
     * @brief 检查是否可以被推动
     */
    [[nodiscard]] bool canBePushed() const { return m_canBePushed; }

protected:
    /**
     * @brief 注册数据参数
     */
    void registerData() override;

public:
    /**
     * @brief 矿车宽度
     */
    [[nodiscard]] f32 width() const override { return 0.98f; }

    /**
     * @brief 矿车高度
     */
    [[nodiscard]] f32 height() const override { return 0.7f; }

    /**
     * @brief 矿车眼睛高度（乘客坐在矿车水平面上）
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.0f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    // ========== 矿车类型 ==========

    /**
     * @brief 获取矿车类型
     */
    [[nodiscard]] Type getMinecartType() const { return m_type; }

    // ========== 速度和物理 ==========

    /**
     * @brief 获取最大速度
     */
    [[nodiscard]] virtual f32 getMaxSpeed() const { return DEFAULT_MAX_SPEED; }

    /**
     * @brief 获取铁轨上的最大速度
     *
     * 速度 = max_minecart_speed 游戏规则值 / 20.0，水中减半。
     * 默认 max_minecart_speed = 8，即 0.4 方块/刻。
     */
    [[nodiscard]] virtual f32 getMaxSpeedWithRail() const;

    /**
     * @brief 获取空中最大横向速度
     */
    [[nodiscard]] virtual f32 getMaxSpeedAirLateral() const { return m_maxSpeedAirLateral; }

    /**
     * @brief 获取空中最大纵向速度
     */
    [[nodiscard]] virtual f32 getMaxSpeedAirVertical() const { return m_maxSpeedAirVertical; }

    /**
     * @brief 获取空气阻力
     */
    [[nodiscard]] virtual f32 getDragAir() const { return m_dragAir; }

    /**
     * @brief 设置最大速度
     */
    void setMaxSpeed(f32 speed) { m_maxSpeed = speed; }

    // ========== 铁轨相关 ==========

    /**
     * @brief 检查是否在铁轨上
     */
    [[nodiscard]] bool isOnRail() const { return m_onRail; }

    /**
     * @brief 检查当前位置是否在铁轨上
     * @param pos 方块位置
     */
    [[nodiscard]] bool isOnRailAt(const BlockPos& pos) const;

    /**
     * @brief 获取当前铁轨形状
     */
    [[nodiscard]] RailShape getRailShape() const { return m_railShape; }

    /**
     * @brief 获取当前铁轨位置
     */
    [[nodiscard]] const BlockPos& getRailPosition() const { return m_railPos; }

    // ========== 损坏和动画 ==========

    /**
     * @brief 获取矿车的损坏值
     */
    [[nodiscard]] i32 getDamage() const { return m_damage; }

    /**
     * @brief 设置损坏值
     */
    void setDamage(i32 damage) { m_damage = damage; }

    /**
     * @brief 获取摇晃幅度
     */
    [[nodiscard]] i32 getRollingAmplitude() const { return m_rollingAmplitude; }

    /**
     * @brief 获取摇晃方向
     */
    [[nodiscard]] i32 getRollingDirection() const { return m_rollingDirection; }

    /**
     * @brief 是否翻转
     *
     * 矿车在铁轨方向变化超过 90° 时会翻转，渲染器据此调整朝向。
     */
    [[nodiscard]] bool isFlipped() const { return m_flipped; }

    // ========== 数据参数访问器（供客户端渲染器读取同步状态） ==========

    // 对齐 vanilla 1.21.11 AbstractMinecart/VehicleEntity 的 5 个 wire 字段（id 8-12）：
    //   HURT(8,i32) / HURTDIR(9,i32) / DAMAGE(10,f32)
    //   CUSTOM_DISPLAY_BLOCK(11,Optional<BlockState>) / DISPLAY_OFFSET(12,i32)
    // 旧实现 rolling_amp/rolling_dir/show_block 为项目自创 ghost 字段(vanilla 无),已删;
    // display_tile 旧 i32 类型与 vanilla Optional<BlockState> 不符致真客户端 field11 崩,已改类型。
    [[nodiscard]] static entity::DataParameter<i32>& getHurtParam() { return DATA_HURT_PARAM; }
    [[nodiscard]] static entity::DataParameter<i32>& getHurtDirParam() { return DATA_HURTDIR_PARAM; }
    [[nodiscard]] static entity::DataParameter<f32>& getDamageParam() { return DATA_DAMAGE_PARAM; }
    [[nodiscard]] static entity::DataParameter<entity::OptionalBlockStateValue>& getCustomDisplayBlockParam()
    {
        return DATA_CUSTOM_DISPLAY_BLOCK_PARAM;
    }
    [[nodiscard]] static entity::DataParameter<i32>& getDisplayOffsetParam() { return DATA_DISPLAY_OFFSET_PARAM; }

    // ========== 乘客和碰撞 ==========

    /**
     * @brief 乘客乘坐高度偏移
     */
    [[nodiscard]] f64 getMountedYOffset() const override { return 0.0; }

    /**
     * @brief 应用力到矿车上（被推动时）
     * @param x X方向力
     * @param z Z方向力
     */
    void applyForce(f32 x, f32 z);

    // ========== 虚方法 ==========

    /**
     * @brief 掉落矿车物品
     * @param source 造成矿车破坏的伤害来源，可能为 nullptr（如非伤害导致）
     */
    virtual void dropItem(DamageSource* source = nullptr);

    /**
     * @brief 激活矿车（如熔炉矿车）
     */
    virtual void activate();

    /**
     * @brief 检查矿车是否已激活
     */
    [[nodiscard]] virtual bool isActivated() const { return false; }

    /**
     * @brief 激活铁轨通过回调
     * @param x 铁轨X坐标
     * @param y 铁轨Y坐标
     * @param z 铁轨Z坐标
     * @param powered 是否充能
     */
    virtual void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered);

    /**
     * @brief 应用摩擦力
     */
    virtual void applyDrag();

protected:
    /**
     * @brief 检查当前位置的铁轨并更新状态
     */
    void _checkRailState();

    /**
     * @brief 沿铁轨移动
     * @param pos 铁轨位置
     */
    void _moveAlongTrack(const BlockPos& pos);

    /**
     * @brief 脱轨移动
     */
    void _moveDerailedMinecart();

    /**
     * @brief 计算铁轨方向向量
     * @param shape 铁轨形状
     * @return 方向向量对 (起点方向, 终点方向)
     */
    [[nodiscard]] std::pair<Vector3, Vector3> _getRailDirectionVectors(RailShape shape) const;

    /**
     * @brief 获取铁轨上的贴靠位置
     * @param x 当前X
     * @param y 当前Y
     * @param z 当前Z
     * @return 贴靠后的位置
     */
    [[nodiscard]] Vector3 _getPosOnRail(f64 x, f64 y, f64 z) const;

    /**
     * @brief 获取斜坡调整值
     */
    [[nodiscard]] static constexpr f64 getSlopeAdjustment() { return SLOPE_ADJUSTMENT; }

    /**
     * @brief 处理实体碰撞
     */
    void _handleEntityCollisions();

    /**
     * @brief 处理矿车间碰撞
     */
    void _handleMinecartCollisions();

    /**
     * @brief 更新摇晃动画
     */
    void _updateRollingAnimation();

    /**
     * @brief 更新矿车朝向
     */
    void _updateRotation();

    // ========== 铁轨类型检测 ==========

    /**
     * @brief 检查是否为动力铁轨
     */
    [[nodiscard]] bool _isPoweredRail(const BlockPos& pos) const;

    /**
     * @brief 检查是否为探测铁轨
     */
    [[nodiscard]] bool _isDetectorRail(const BlockPos& pos) const;

    /**
     * @brief 检查是否为激活铁轨
     */
    [[nodiscard]] bool _isActivatorRail(const BlockPos& pos) const;

    /**
     * @brief 检查铁轨是否充能
     */
    [[nodiscard]] bool _isRailPowered(const BlockPos& pos);

    /**
     * @brief 是否应该执行铁轨特殊功能
     */
    [[nodiscard]] bool shouldDoRailFunctions() const { return true; }

    /**
     * @brief 在铁轨上移动矿车
     * @param pos 铁轨位置
     */
    void _moveMinecartOnRail(const BlockPos& pos);

    /**
     * @brief 检查位置是否为完整方块
     */
    [[nodiscard]] bool _isNormalBlockAt(const BlockPos& pos) const;

private:
    // 静态数据参数（通过 EntityDataManager::createKey 自动分配唯一 ID）
    // 对齐 vanilla 1.21.11：HURT/HURTDIR/DAMAGE 来自 VehicleEntity,CUSTOM_DISPLAY_BLOCK/DISPLAY_OFFSET 来自
    // AbstractMinecart。 项目无 VehicleEntity 中间层,5 字段全部在 AbstractMinecart 注册,wire id 8-12 连续。
    static entity::DataParameter<i32> DATA_HURT_PARAM;
    static entity::DataParameter<i32> DATA_HURTDIR_PARAM;
    static entity::DataParameter<f32> DATA_DAMAGE_PARAM;
    static entity::DataParameter<entity::OptionalBlockStateValue> DATA_CUSTOM_DISPLAY_BLOCK_PARAM;
    static entity::DataParameter<i32> DATA_DISPLAY_OFFSET_PARAM;

protected:
    /// 本类继承链标识（parent = Entity::classInfo()）。见 Entity::classInfo()。
    static const EntityClassInfo& classInfo();

private:
    // 矿车类型
    Type m_type;

    // 铁轨状态
    bool m_onRail = false;
    BlockPos m_railPos;
    RailShape m_railShape = RailShape::NorthSouth;
    bool m_flipped = false; // 是否翻转

    // 速度
    f32 m_maxSpeed = DEFAULT_MAX_SPEED;
    f32 m_maxSpeedAirLateral = DEFAULT_MAX_SPEED_AIR_LATERAL;
    f32 m_maxSpeedAirVertical = DEFAULT_MAX_SPEED_AIR_VERTICAL;
    f32 m_dragAir = DEFAULT_AIR_DRAG;

    // 损坏和动画
    i32 m_damage = 0;
    i32 m_rollingAmplitude = 0;
    i32 m_rollingDirection = 1;

    // 显示方块（vanilla 1.21.11 走 wire DATA_CUSTOM_DISPLAY_BLOCK_PARAM 同步,项目尚未接业务）
    // TODO: 待实现矿车内显示方块业务(熔炉/刷怪笼/命令方块矿车等),通过 DATA_CUSTOM_DISPLAY_BLOCK_PARAM
    //       (OptionalBlockStateValue) 同步到客户端,并扩展渲染器消费。当前成员声明保留供未来接入。
    i32 m_displayTile = 0;       // 方块状态ID（待接入 wire）
    i32 m_displayTileOffset = 6; // 显示偏移（待接入 wire）
    bool m_showBlock = false;    // 是否显示方块（待接入 wire,对应 Optional present）

    // 推动力（熔炉矿车用）
    f32 m_pushX = 0.0f;
    f32 m_pushZ = 0.0f;

    // 可推动状态
    bool m_canBePushed = true;

    // 常量
    static constexpr f32 DEFAULT_MAX_SPEED = 0.4f;               // 最大铁轨速度
    static constexpr f32 DEFAULT_MAX_SPEED_AIR_LATERAL = 0.4f;   // 空中最大横向速度
    static constexpr f32 DEFAULT_MAX_SPEED_AIR_VERTICAL = -1.0f; // 空中最大纵向速度（-1表示禁用）
    static constexpr f32 DEFAULT_AIR_DRAG = 0.95f;               // 空气阻力
    static constexpr f64 SLOPE_ADJUSTMENT = 0.0078125;           // 斜坡调整值 (1/128)
    static constexpr f32 POWERED_RAIL_BOOST = 0.06f;             // 动力铁轨加速
    static constexpr f32 UNPOWERED_RAIL_THRESHOLD = 0.03f;       // 无动力铁轨停止阈值
    static constexpr f32 OCCUPIED_DRAG = 0.997f;                 // 有乘客摩擦
    static constexpr f32 EMPTY_DRAG = 0.96f;                     // 无乘客摩擦
    static constexpr f32 DAMAGE_THRESHOLD = 40.0f;               // 摧毁阈值
};

/**
 * @brief 普通矿车
 */
class RideableMinecartEntity : public AbstractMinecartEntity {
public:
    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    RideableMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : AbstractMinecartEntity(Type::Rideable, id, registry)
    {}

    /**
     * @brief 激活铁轨通过时弹出乘客
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;
};

/**
 * @brief 箱子矿车
 */
class ChestMinecartEntity : public AbstractMinecartEntity {
public:
    static constexpr i32 INVENTORY_SIZE = 27; // 3行 x 9列

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    ChestMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    /**
     * @brief 箱子矿车有额外的摩擦力
     */
    void applyDrag() override;

    /**
     * @brief 掉落物品时同时掉落库存内容
     * @param source 造成矿车破坏的伤害来源，可能为 nullptr
     */
    void dropItem(DamageSource* source = nullptr) override;

    // ========== Entity 接口重写 ==========

    /**
     * @brief 获取比较器输出信号强度
     *
     * 基于库存填充率计算信号强度（0-15）。
     */
    [[nodiscard]] i32 getComparatorOutput() const override;

    // ========== IInventory 代理方法 ==========

    /**
     * @brief 获取库存大小
     */
    [[nodiscard]] i32 getContainerSize() const;

    /**
     * @brief 检查库存是否为空
     */
    [[nodiscard]] bool isInventoryEmpty() const;

    /**
     * @brief 获取指定槽位的物品
     */
    [[nodiscard]] ItemStack getInventoryItem(i32 slot) const;

    /**
     * @brief 设置指定槽位的物品
     */
    void setInventoryItem(i32 slot, const ItemStack& stack);

    /**
     * @brief 从槽位移除指定数量的物品
     */
    ItemStack removeInventoryItem(i32 slot, i32 count);

    /**
     * @brief 清空库存
     */
    void clearInventory();

    /**
     * @brief 获取库存指针
     */
    [[nodiscard]] IInventory* getInventory();

private:
    /// 27格库存（与箱子相同）
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;
};

/**
 * @brief 熔炉矿车
 *
 * 熔炉矿车可以：
 * - 燃烧燃料获得动力
 * - 自动向行驶方向推进
 * - 被玩家推动改变方向
 */
class FurnaceMinecartEntity : public AbstractMinecartEntity {
public:
    /// 燃料上限（32000 ticks）
    static constexpr i32 MAX_FUEL = 32000;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    FurnaceMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : AbstractMinecartEntity(Type::Furnace, id, registry)
    {}

    void tick() override;

    [[nodiscard]] bool isActivated() const override { return m_fuel > 0; }

    /**
     * @brief 熔炉矿车速度较慢
     */
    [[nodiscard]] f32 getMaxSpeed() const override { return 0.2f; }

    /**
     * @brief 添加燃料
     * @param ticks 燃烧时间（tick）
     */
    void addFuel(i32 ticks);

    /**
     * @brief 获取剩余燃料
     */
    [[nodiscard]] i32 getFuel() const { return m_fuel; }

    /**
     * @brief 设置推动方向
     * @param x X方向推力
     * @param z Z方向推力
     */
    void setPushDirection(f32 x, f32 z)
    {
        m_pushX = x;
        m_pushZ = z;
    }

    /**
     * @brief 熔炉矿车摩擦力计算
     */
    void applyDrag() override;

    /**
     * @brief 激活熔炉矿车（玩家右键交互时调用）
     */
    void activate() override;

    /**
     * @brief 激活铁轨通过时改变方向
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;

    /**
     * @brief 更新推动方向
     */
    void updatePushDirection();

    /**
     * @brief 掉落物品时掉落熔炉
     * @param source 造成矿车破坏的伤害来源，可能为 nullptr
     */
    void dropItem(DamageSource* source = nullptr) override;

private:
    i32 m_fuel = 0;
    f32 m_pushX = 0.0f;
    f32 m_pushZ = 0.0f;
};

/**
 * @brief TNT矿车
 *
 * TNT矿车可以：
 * - 被激活铁轨点燃
 * - 被爆炸点燃
 * - 被火焰/熔岩点燃
 * - 被燃烧的箭矢点燃
 * - 碰撞时根据速度造成不同威力的爆炸
 * - 摔落时爆炸
 */
class TNTMinecartEntity : public AbstractMinecartEntity {
public:
    /// 默认引信时间（80 tick = 4秒）
    static constexpr i32 DEFAULT_FUSE = 80;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    TNTMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : AbstractMinecartEntity(Type::TNT, id, registry)
    {}

    void tick() override;

    /**
     * @brief 点燃TNT
     * @param fuseTicks 引信时间（tick），默认80
     */
    void prime(i32 fuseTicks = DEFAULT_FUSE) { m_fuse = fuseTicks; }

    /**
     * @brief 是否已点燃
     */
    [[nodiscard]] bool isPrimed() const { return m_fuse > -1; }

    /**
     * @brief 获取剩余引信时间
     *
     * 返回剩余引信 tick 数；-1 表示未点燃。
     * 渲染器据此判断 TNT 闪烁叠加层是否生效及计算闪烁缩放因子。
     */
    [[nodiscard]] i32 fuse() const { return m_fuse; }

    /**
     * @brief 激活铁轨点燃TNT
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;

    /**
     * @brief 处理投射物命中
     * @param source 伤害来源
     * @param amount 伤害量
     * @return 是否处理了该伤害
     */
    bool onProjectileHit(DamageSource& source, f32 amount);

    /**
     * @brief 重写伤害处理
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 重写掉落物品逻辑
     * 火焰/爆炸伤害时点燃而非爆炸掉落
     * @param source 造成矿车破坏的伤害来源，可能为 nullptr
     */
    void dropItem(DamageSource* source = nullptr) override;

private:
    /**
     * @brief 爆炸
     * @param speedFactor 速度因子，影响爆炸威力
     * @param damageSource 自定义伤害来源，用于记录爆炸归因（可能为 nullptr）
     *
     * 内部调用 IWorld::createExplosionWithSource()，后者会自行 clone damageSource，
     * 因此调用者只需传入原始指针，无需 clone。
     */
    void _explode(f32 speedFactor, const DamageSource* damageSource = nullptr);

    /**
     * @brief 检查火焰接触并点燃
     */
    void _checkFireIgnition();

    /**
     * @brief 点燃TNT（播放声音等）
     * @param source 点燃来源的伤害信息（可能为 nullptr，如激活铁轨点燃）
     */
    void _ignite(const DamageSource* source = nullptr);

    /**
     * @brief 判断伤害源是否能够点燃TNT矿车
     *
     * 以下类型的伤害源可以点燃TNT：
     * - 直接实体是着火的投射物
     * - 伤害类型为火焰（IS_FIRE）
     * - 伤害类型为爆炸（IS_EXPLOSION）
     *
     * @param source 伤害来源
     * @return 是否能点燃TNT
     */
    [[nodiscard]] static bool _damageSourceIgnitesTnt(const DamageSource& source);

    i32 m_fuse = -1; ///< -1 表示未点燃

    /// 引爆来源（首次点燃时设置，之后不再覆盖）
    /// 对应 MC Java 的 ignitionSource 字段，用于爆炸伤害归因
    std::unique_ptr<DamageSource> m_ignitionSource;
};

/**
 * @brief 漏斗矿车
 *
 * 漏斗矿车可以：
 * - 从上方吸取物品实体
 * - 向下方容器传输物品
 * - 被红石信号禁用
 */
class HopperMinecartEntity : public AbstractMinecartEntity, public blockentity::IHopper {
public:
    static constexpr i32 INVENTORY_SIZE = 5;    ///< 漏斗矿车有5格库存
    static constexpr i32 TRANSFER_COOLDOWN = 4; ///< 传输冷却（tick）

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    HopperMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~HopperMinecartEntity() override = default;

    void tick() override;

    // ========== IHopper 接口实现 ==========

    [[nodiscard]] IWorld* getWorld() override { return Entity::world(); }
    [[nodiscard]] const IWorld* getWorld() const override { return Entity::world(); }
    [[nodiscard]] f64 getXPos() const override { return x(); }
    [[nodiscard]] f64 getYPos() const override { return y(); }
    [[nodiscard]] f64 getZPos() const override { return z(); }
    [[nodiscard]] BlockPos getHopperPos() const override
    {
        return BlockPos(static_cast<BlockCoord>(std::floor(x())),
            static_cast<BlockCoord>(std::floor(y())),
            static_cast<BlockCoord>(std::floor(z())));
    }
    [[nodiscard]] Direction getOutputDirection() const override { return Direction::Down; }

    /**
     * @brief 漏斗矿车不与网格对齐
     * MC Java 中 HopperMinecart.isGridAligned() 返回 false，
     * 因此漏斗矿车不会因为上方方块的碰撞形状而跳过物品吸取。
     */
    [[nodiscard]] bool isGridAligned() const override { return false; }

    /**
     * @brief 获取漏斗矿车的物品背包
     * @return 背包指针
     *
     * 实现 IHopper::getHopperInventory()，返回矿车内部背包。
     */
    [[nodiscard]] IInventory* getHopperInventory() override { return getInventory(); }

    // ========== IInventory 代理方法 ==========

    [[nodiscard]] i32 getContainerSize() const;
    [[nodiscard]] bool isInventoryEmpty() const;
    [[nodiscard]] ItemStack getInventoryItem(i32 slot) const;
    void setInventoryItem(i32 slot, const ItemStack& stack);
    ItemStack removeInventoryItem(i32 slot, i32 count);
    void clearInventory();
    [[nodiscard]] IInventory* getInventory();

    // ========== Entity 接口重写 ==========

    /**
     * @brief 漏斗矿车被摧毁时掉落物品
     *
     * 掉落容器内容物（受 doEntityDrops 游戏规则控制），
     * 然后调用父类方法掉落矿车物品（同样受 doEntityDrops 控制）。
     */
    void dropItem(DamageSource* source = nullptr) override;

    /**
     * @brief 获取比较器输出信号强度
     *
     * 基于库存填充率计算信号强度（0-15）。
     */
    [[nodiscard]] i32 getComparatorOutput() const override;

    // ========== 漏斗功能 ==========

    /**
     * @brief 是否可以吸取物品
     */
    [[nodiscard]] bool canSuckItems() const { return m_suckCooldown <= 0 && !m_disabled; }

    /**
     * @brief 是否被红石禁用
     */
    [[nodiscard]] bool isDisabled() const { return m_disabled; }

    /**
     * @brief 设置禁用状态（红石控制）
     */
    void setDisabled(bool disabled) { m_disabled = disabled; }

    /**
     * @brief 激活铁轨通过回调
     * 激活铁轨充能时禁用漏斗
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;

private:
    /**
     * @brief 从上方吸取物品
     */
    void _suckItems();

    /**
     * @brief 向下方容器传输物品
     */
    void _transferItemsOut();

    std::unique_ptr<blockentity::SimpleInventory> m_inventory;
    i32 m_suckCooldown = 0;
    bool m_disabled = false; ///< 红石禁用状态
};

/**
 * @brief 命令方块矿车
 */
class CommandBlockMinecartEntity : public AbstractMinecartEntity {
public:
    CommandBlockMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : AbstractMinecartEntity(Type::CommandBlock, id, registry)
    {}

    void tick() override;

    /**
     * @brief 设置命令
     */
    void setCommand(const std::string& command) { m_command = command; }

    /**
     * @brief 获取命令
     */
    [[nodiscard]] const std::string& getCommand() const { return m_command; }

    /**
     * @brief 获取上次输出
     */
    [[nodiscard]] const std::string& getLastOutput() const { return m_lastOutput; }

    /**
     * @brief 获取成功次数
     */
    [[nodiscard]] i32 getSuccessCount() const { return m_successCount; }

    /**
     * @brief 设置成功次数
     *
     * 命令执行后更新成功次数，用于比较器信号输出。
     *
     * @param count 成功次数
     */
    void setSuccessCount(i32 count) { m_successCount = count; }

    // ========== Entity 接口重写 ==========

    /**
     * @brief 获取比较器输出信号强度
     *
     * 返回命令执行成功次数（0-15），上限为15。
     */
    [[nodiscard]] i32 getComparatorOutput() const override;

    /**
     * @brief 激活铁轨通过时执行命令
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;

private:
    /**
     * @brief 执行命令
     */
    void _executeCommand();

    std::string m_command;
    std::string m_lastOutput;
    i32 m_successCount = 0;
    bool mPowered = false; ///< 当前是否被激活
};

/**
 * @brief 刷怪笼矿车
 *
 * 刷怪笼矿车可以：
 * - 在矿车位置附近自动生成实体
 * - 使用 SpawnerLogic 控制生成逻辑（与 MobSpawnerBlockEntity 共享）
 * - 被破坏时不会掉落任何物品（既不掉矿车也不掉刷怪笼方块）
 * - 在矿车内显示刷怪笼方块
 *
 * 对应 MC Java 的 MinecartSpawner（net.minecraft.world.entity.vehicle.MinecartSpawner），
 * 内部持有 BaseSpawner 实例（本项目为 SpawnerLogic）。
 *
 * 注意：刷怪笼矿车在原版中没有对应物品，只能通过 /summon 命令生成。
 * 创造模式选取（getPickResult）返回普通矿车。
 */
class SpawnerMinecartEntity : public AbstractMinecartEntity {
public:
    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    SpawnerMinecartEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 序列化额外数据
     *
     * 保存刷怪笼逻辑参数（生成延迟、实体类型、生成候选列表等）。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 反序列化额外数据
     *
     * 读取刷怪笼逻辑参数。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 刷怪笼逻辑访问 ==========

    /**
     * @brief 获取刷怪笼逻辑
     * @return 刷怪笼逻辑的引用
     */
    [[nodiscard]] blockentity::SpawnerLogic& getSpawnerLogic() { return m_spawnerLogic; }
    [[nodiscard]] const blockentity::SpawnerLogic& getSpawnerLogic() const { return m_spawnerLogic; }

private:
    /// 刷怪笼逻辑（对应 MC Java 的 BaseSpawner）
    blockentity::SpawnerLogic m_spawnerLogic;
};

} // namespace entity
} // namespace mc
