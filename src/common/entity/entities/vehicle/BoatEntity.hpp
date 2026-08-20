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

#include "../../core/Entity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <memory>
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;
class BlockState;
class DamageSource;
class Item;

namespace world::explosion {
struct ExplosionImmunityContext;
} // namespace world::explosion

namespace entity {

/**
 * @brief 船的状态
 *
 * 用于确定船在不同介质中的行为
 */
enum class BoatStatus : u8 {
    InWater,           // 在水面上
    UnderWater,        // 在水下（水源方块）
    UnderFlowingWater, // 在水下（流动水）
    OnLand,            // 在陆地上
    InAir              // 在空中
};

/**
 * @brief 船实体
 *
 * 可乘坐的水上交通工具，有多种木材类型变体。
 *
 * 特性：
 * - 浮力：在水面漂浮
 * - 控制：玩家可以控制方向和速度
 * - 乘客：最多承载2名乘客
 * - 损坏：受到足够伤害会掉落物品
 * - 气泡柱：会被气泡柱影响
 */
class BoatEntity : public Entity {
public:
    /**
     * @brief 船的类型（木材类型）
     */
    enum class Type : u8 {
        OAK = 0,
        SPRUCE = 1,
        BIRCH = 2,
        JUNGLE = 3,
        ACACIA = 4,
        DARK_OAK = 5,
        MANGROVE = 6,
        CHERRY = 7,
        PALE_OAK = 8,
        BAMBOO = 9
    };

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    explicit BoatEntity(Type type, ecs::EntityRegistry& registry);
    ~BoatEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 处理玩家交互
     *
     * 玩家右键点击船时，优先尝试让玩家乘坐。
     * 蹲下时不会乘坐（由 ChestBoatEntity 使用此行为来打开容器）。
     * 船失控（水下超时）时也会拒绝乘坐。
     */
    ActionResultType processInitialInteract(Player& player, Hand hand) override;

    /**
     * @brief 处理伤害
     * @note 船不继承LivingEntity，所以不重写hurt方法
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 检查是否可以被碰撞
     */
    [[nodiscard]] bool canBeCollidedWith() const override { return isAlive(); }

    /**
     * @brief 判断船是否忽略此次爆炸
     *
     * 间接源为 Mob 的爆炸：受不受影响取决于 mobGriefing（关闭则忽略）。
     * 其他爆炸回退基类行为。
     */
    [[nodiscard]] bool ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const override;

    /**
     * @brief 检查是否可以被推动
     */
    [[nodiscard]] bool canBePushed() const { return true; }

protected:
    /**
     * @brief 注册数据参数
     */
    void registerData() override;

    /**
     * @brief 取本船在 Java entity_type 注册表中的 vanilla 木种变体名
     *
     * vanilla 1.21.11 无泛型 boat/chest_boat，按木种拆 oak_boat/mangrove_chest_boat/bamboo_raft
     * 等。bamboo 用 raft/raft_chest，其余 9 种用 <wood>_boat/<wood>_chest_boat。chest=true 取
     * 箱子船变体。供 getJavaEntityTypeId() override 查 JavaEntityTypeIdMap 用。
     *
     * @param chest 是否取箱子船变体
     * @return vanilla 变体名（含 minecraft: 前缀）
     */
    [[nodiscard]] std::string boatVariantName(bool chest) const;

public:
    [[nodiscard]] f32 width() const override { return 1.375f; }
    [[nodiscard]] f32 height() const override { return 0.5625f; }
    [[nodiscard]] f32 eyeHeight() const override { return height(); }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    /**
     * @brief 乘客乘坐高度偏移
     */
    [[nodiscard]] f64 getMountedYOffset() const override;

    /**
     * @brief 最大乘客数量
     */
    [[nodiscard]] i32 getMaxPassengers() const override { return MAX_PASSENGERS; }

    /**
     * @brief 更新乘客位置
     */
    void updatePassengerPosition(Entity& passenger) override;

    // ========== 伤害和掉落 ==========

    /**
     * @brief 处理摔落伤害
     */
    void updateFallState(f64 y, bool onGround);

    /**
     * @brief 掉落船物品
     */
    virtual void dropItem();

    /**
     * @brief 掉落船物品（带伤害倍率）
     */
    void dropItemWithDamage();

    // ========== 船类型 ==========

    /**
     * @brief 获取船的类型
     */
    [[nodiscard]] Type getBoatType() const { return m_type; }

    /**
     * @brief 设置船的类型
     */
    void setBoatType(Type type) { m_type = type; }

    /**
     * @brief 获取船对应的物品
     * @return 对应木材类型的普通船物品（箱子船由子类重写返回箱子船物品）
     */
    [[nodiscard]] virtual const Item* getBoatItem() const;

    /**
     * @brief 取普通船的 vanilla entity_type 注册表 id（按木种选 <wood>_boat 变体）
     *
     * 重写 Entity::getJavaEntityTypeId()：vanilla 无泛型 boat，按 m_type 木种拼变体名
     * （oak_boat/mangrove_boat/bamboo_raft 等）查 JavaEntityTypeIdMap。ChestBoatEntity 再
     * override 取 <wood>_chest_boat 变体。
     */
    [[nodiscard]] u32 getJavaEntityTypeId() const override;

    /**
     * @brief 是否为带箱子的船
     */
    [[nodiscard]] bool hasChest() const { return m_hasChest; }

    /**
     * @brief 设置是否为带箱子的船
     */
    void setHasChest(bool hasChest) { m_hasChest = hasChest; }

    /**
     * @brief 获取船的状态
     */
    [[nodiscard]] BoatStatus getStatus() const { return m_status; }

    /**
     * @brief 是否在水下
     *
     * 用于渲染器判断气泡柱倾斜效果是否生效。
     * 当状态为 UnderWater 或 UnderFlowingWater 时返回 true。
     */
    [[nodiscard]] bool isUnderWater() const
    {
        return m_status == BoatStatus::UnderWater || m_status == BoatStatus::UnderFlowingWater;
    }

    // ========== 伤害状态 ==========

    /**
     * @brief 获取上次受击时间
     *
     * 对应 MC VehicleEntity.getHurtTime()，范围 0-10，
     * 受击时设为 10，每 tick 递减。渲染器用于计算受损抖动角度。
     */
    [[nodiscard]] i32 getTimeSinceHit() const { return m_timeSinceHit; }

    /**
     * @brief 设置上次受击时间
     */
    void setTimeSinceHit(i32 time) { m_timeSinceHit = time; }

    /**
     * @brief 获取前进方向
     */
    [[nodiscard]] i32 getForwardDirection() const { return m_forwardDirection; }

    /**
     * @brief 设置前进方向
     */
    void setForwardDirection(i32 direction) { m_forwardDirection = direction; }

    /**
     * @brief 获取累积伤害
     */
    [[nodiscard]] f32 getDamageTaken() const { return m_damageTaken; }

    /**
     * @brief 设置累积伤害
     */
    void setDamageTaken(f32 damage) { m_damageTaken = damage; }

    /**
     * @brief 获取摇晃tick数
     */
    [[nodiscard]] i32 getRockingTicks() const { return m_rockingTicks; }

    /**
     * @brief 设置摇晃tick数
     */
    void setRockingTicks(i32 ticks) { m_rockingTicks = ticks; }

    // ========== 桨动画 ==========

    /**
     * @brief 获取桨角度
     * @param side 0=左桨, 1=右桨
     */
    [[nodiscard]] f32 getPaddleState(i32 side) const
    {
        if (side < 0 || side > 1) return 0.0f;
        return m_paddlePositions[side];
    }

    /**
     * @brief 设置桨状态
     */
    void setPaddleState(bool left, bool right)
    {
        m_leftPaddle = left;
        m_rightPaddle = right;
    }

    /**
     * @brief 检查桨是否在划动
     * @param side 0=左桨, 1=右桨
     *
     * 对应 MC AbstractBoat.getPaddleState(int)，
     * 返回对应桨是否正在划动（由玩家输入驱动）。
     * 渲染器据此判断是否应用划桨动画。
     */
    [[nodiscard]] bool isPaddleActive(i32 side) const
    {
        if (side == 0) return m_leftPaddle;
        if (side == 1) return m_rightPaddle;
        return false;
    }

    /**
     * @brief 获取划桨插值时间
     * @param side 0=左桨, 1=右桨
     * @param partialTicks 部分 tick（用于插值）
     *
     * 对应 MC AbstractBoat.getRowingTime(int, float)。
     * 当桨在划动时返回 [paddlePositions[side] - PI/8, paddlePositions[side]] 之间的插值，
     * 否则返回 0。
     */
    [[nodiscard]] f32 getRowingTime(i32 side, f32 partialTicks) const
    {
        if (side < 0 || side > 1) return 0.0f;
        if (!isPaddleActive(side)) return 0.0f;
        // clampedLerp(partialTicks, paddlePositions[side] - PI/8, paddlePositions[side])
        const f32 lo = m_paddlePositions[side] - math::PI / 8.0f;
        const f32 hi = m_paddlePositions[side];
        return math::clampedLerp(partialTicks, lo, hi);
    }

    /**
     * @brief 获取气泡柱倾斜角度（插值）
     * @param partialTicks 部分 tick（用于插值）
     *
     * 对应 MC AbstractBoat.getBubbleAngle(float)。
     * 返回 prevRockingAngle 到 rockingAngle 的插值，
     * 渲染器据此应用绕 (1,0,1) 轴的倾斜旋转。
     */
    [[nodiscard]] f32 getBubbleAngle(f32 partialTicks) const
    {
        return math::lerp(m_prevRockingAngle, m_rockingAngle, partialTicks);
    }

    // ========== 控制 ==========

    /**
     * @brief 处理玩家控制输入
     * @param left 左转
     * @param right 右转
     * @param forward 前进
     * @param backward 后退
     */
    void handleInput(bool left, bool right, bool forward, bool backward);

    /**
     * @brief 设置船的朝向
     */
    void setRotation(f32 yaw) { Entity::setRotation(yaw, m_builtIn.rotation->m_rot.y); }

    // ========== 数据参数访问器（供客户端渲染器读取同步状态） ==========

    /**
     * @brief 获取"受击时间"数据参数 ID
     *
     * 客户端渲染器通过此 ID 从 ClientEntity::dataManager() 读取
     * 服务端同步过来的受击时间，用于计算受损抖动角度。
     */
    [[nodiscard]] static entity::DataParameter<i32>& getTimeSinceHitParam() { return DATA_TIME_SINCE_HIT_PARAM; }
    [[nodiscard]] static entity::DataParameter<i32>& getForwardDirectionParam() { return DATA_FORWARD_DIRECTION_PARAM; }
    [[nodiscard]] static entity::DataParameter<f32>& getDamageTakenParam() { return DATA_DAMAGE_TAKEN_PARAM; }
    [[nodiscard]] static entity::DataParameter<bool>& getLeftPaddleParam() { return DATA_LEFT_PADDLE_PARAM; }
    [[nodiscard]] static entity::DataParameter<bool>& getRightPaddleParam() { return DATA_RIGHT_PADDLE_PARAM; }
    [[nodiscard]] static entity::DataParameter<i32>& getBubbleTimeParam() { return DATA_BUBBLE_TIME_PARAM; }

    /// 本类继承链标识（parent = Entity::classInfo()）。见 Entity::classInfo()。
    /// 字段集对齐 vanilla 1.21.11 AbstractBoat/VehicleEntity（船类型由 EntityType 区分，
    /// 非同步字段；HURT/HURTDIR/DAMAGE/PADDLE_LEFT/PADDLE_RIGHT/BUBBLE_TIME 同步）。
    static const entity::EntityClassInfo& classInfo();

    // ========== 乘客 ==========

    /**
     * @brief 检查是否可以添加指定乘客
     *
     * 检查乘客数量未满且船不在水下。
     * 检查乘客数量未满且船不在水下。
     */
    [[nodiscard]] bool canAddPassenger(const Entity& passenger) const override
    {
        (void)passenger;
        return static_cast<i32>(m_passengers.size()) < MAX_PASSENGERS && m_status != BoatStatus::UnderWater;
    }

    /**
     * @brief 获取上方水面高度
     */
    [[nodiscard]] f32 getWaterLevelAbove();

protected:
    /**
     * @brief 更新运动
     */
    void updateMotion();

    /**
     * @brief 计算浮力
     */
    void floatBoat();

    /**
     * @brief 控制船的移动
     */
    void controlBoat();

    /**
     * @brief 更新插值
     */
    void tickLerp();

    /**
     * @brief 更新状态
     */
    void updateStatus();

    /**
     * @brief 设置船的状态（用于测试和子类）
     */
    void setStatus(BoatStatus status) { m_status = status; }

    /**
     * @brief 设置陆地滑度值（用于测试和子类）
     */
    void setBoatGlide(f32 glide) { m_boatGlide = glide; }

    /**
     * @brief 获取水下状态
     */
    [[nodiscard]] BoatStatus getUnderwaterStatus();

    /**
     * @brief 检测是否在水中
     */
    [[nodiscard]] bool checkInWater();

    /**
     * @brief 获取地面滑动系数
     */
    [[nodiscard]] f32 getBoatGlide();

    /**
     * @brief 更新所有乘客位置
     */
    void updateAllPassengerPositions();

    /**
     * @brief 将船的朝向应用到乘客
     */
    void applyOrientationToEntity(Entity& passenger) override;

    /**
     * @brief 更新摇晃（气泡柱）
     */
    void updateRocking();

private:
    // 船类型
    Type m_type = Type::OAK;

    // 是否为带箱子的船
    bool m_hasChest = false;

    // 状态
    BoatStatus m_status = BoatStatus::InWater;
    BoatStatus m_previousStatus = BoatStatus::InWater;

    // 运动状态
    f32 m_speed = 0.0f;
    f32 m_rotationVelocity = 0.0f;

    // 桨状态
    f32 m_paddlePositions[2] = {0.0f, 0.0f};
    bool m_leftPaddle = false;
    bool m_rightPaddle = false;

    // 输入状态
    bool m_leftInputDown = false;
    bool m_rightInputDown = false;
    bool m_forwardInputDown = false;
    bool m_backwardInputDown = false;
    f32 m_deltaRotation = 0.0f;

    // 损伤状态
    i32 m_timeSinceHit = 0;
    i32 m_forwardDirection = 1;
    f32 m_damageTaken = 0.0f;

    // 插值
    i32 m_interpolationSteps = 0;
    f64 m_interpolationX = 0.0;
    f64 m_interpolationY = 0.0;
    f64 m_interpolationZ = 0.0;
    f64 m_interpolationYaw = 0.0;
    f64 m_interpolationPitch = 0.0;

    // 气泡柱
    i32 m_rockingTicks = 0;
    bool m_rocking = false;
    bool m_rockingDownwards = false;
    f32 m_rockingIntensity = 0.0f;
    f32 m_rockingAngle = 0.0f;
    f32 m_prevRockingAngle = 0.0f;

    // 水位
    f64 m_waterLevel = 0.0;
    f32 m_boatGlide = 0.0f;

    // 失控计时器
    i32 m_outOfControlTicks = 0;

    // 落下追踪
    f64 m_lastYd = 0.0;

    // 常量
    static constexpr f32 MAX_SPEED = 0.4f;
    static constexpr i32 MAX_PASSENGERS = 2;

    // 静态数据参数（id 由继承链分配器按 registerData 调用顺序分配，对齐 vanilla 1.21.11
    // AbstractBoat/VehicleEntity：HURT(8)/HURTDIR(9)/DAMAGE(10)/PADDLE_LEFT(11)/
    // PADDLE_RIGHT(12)/BUBBLE_TIME(13)。船类型由 EntityType 区分，非同步字段。）
    static entity::DataParameter<i32> DATA_TIME_SINCE_HIT_PARAM;    // = vanilla DATA_ID_HURT (8)
    static entity::DataParameter<i32> DATA_FORWARD_DIRECTION_PARAM; // = vanilla DATA_ID_HURTDIR (9)
    static entity::DataParameter<f32> DATA_DAMAGE_TAKEN_PARAM;      // = vanilla DATA_ID_DAMAGE (10)
    static entity::DataParameter<bool> DATA_LEFT_PADDLE_PARAM;      // = vanilla DATA_ID_PADDLE_LEFT (11)
    static entity::DataParameter<bool> DATA_RIGHT_PADDLE_PARAM;     // = vanilla DATA_ID_PADDLE_RIGHT (12)
    static entity::DataParameter<i32> DATA_BUBBLE_TIME_PARAM;       // = vanilla DATA_ID_BUBBLE_TIME (13)
};

} // namespace entity
} // namespace mc
