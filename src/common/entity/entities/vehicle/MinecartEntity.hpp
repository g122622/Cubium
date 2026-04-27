#pragma once

#include "../../core/Entity.hpp"
#include "../../../world/block/BlockPos.hpp"
#include <string>
#include <array>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;
class Block;
class IWorld;

namespace entity {

/**
 * @brief 铁轨形状枚举
 *
 * MC 1.16.5 RailShape
 */
enum class RailShape : u8 {
    NorthSouth = 0,    // 南北向直轨
    EastWest = 1,      // 东西向直轨
    AscendingEast = 2, // 向东上坡
    AscendingWest = 3, // 向西上坡
    AscendingNorth = 4,// 向北上坡
    AscendingSouth = 5,// 向南上坡
    SouthEast = 6,     // 东南弯道
    SouthWest = 7,     // 西南弯道
    NorthWest = 8,     // 西北弯道
    NorthEast = 9      // 东北弯道
};

/**
 * @brief 矿车实体基类
 *
 * 可在铁轨上行驶的车辆。
 * 有多种变体：普通、箱、漏斗、熔炉、TNT、命令方块矿车。
 *
 * 参考 MC 1.16.5 AbstractMinecartEntity
 */
class AbstractMinecartEntity : public Entity {
public:
    /**
     * @brief 矿车类型
     */
    enum class Type : u8 {
        Rideable = 0,       // 普通矿车
        Chest = 1,          // 箱子矿车
        Furnace = 2,        // 熔炉矿车
        TNT = 3,            // TNT矿车
        Spawner = 4,        // 刷怪笼矿车
        Hopper = 5,         // 漏斗矿车
        CommandBlock = 6    // 命令方块矿车
    };

    /**
     * @brief 构造函数
     * @param type 矿车类型
     * @param id 实体ID
     */
    AbstractMinecartEntity(Type type, EntityId id);

    /**
     * @brief 构造函数（用于工厂创建）
     * @param type 矿车类型
     */
    explicit AbstractMinecartEntity(Type type = Type::Rideable);

    ~AbstractMinecartEntity() override = default;

    // 禁止拷贝
    AbstractMinecartEntity(const AbstractMinecartEntity&) = delete;
    AbstractMinecartEntity& operator=(const AbstractMinecartEntity&) = delete;

    // 允许移动
    AbstractMinecartEntity(AbstractMinecartEntity&&) = default;
    AbstractMinecartEntity& operator=(AbstractMinecartEntity&&) = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 矿车宽度
     * MC 1.16.5: 0.98F
     */
    [[nodiscard]] f32 width() const override { return 0.98f; }

    /**
     * @brief 矿车高度
     * MC 1.16.5: 0.7F
     */
    [[nodiscard]] f32 height() const override { return 0.7f; }

    /**
     * @brief 矿车眼睛高度
     * MC 1.16.5: 0.0D (乘客坐在矿车水平面上)
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.0f; }

    // ========== 矿车类型 ==========

    /**
     * @brief 获取矿车类型
     */
    [[nodiscard]] Type getMinecartType() const { return m_type; }

    // ========== 速度和物理 ==========

    /**
     * @brief 获取最大速度
     * MC 1.16.5: getMaximumSpeed() -> 0.4D
     */
    [[nodiscard]] virtual f32 getMaxSpeed() const { return DEFAULT_MAX_SPEED; }

    /**
     * @brief 获取空中最大横向速度
     * MC 1.16.5: getMaxSpeedAirLateral()
     */
    [[nodiscard]] virtual f32 getMaxSpeedAirLateral() const { return m_maxSpeedAirLateral; }

    /**
     * @brief 获取空中最大纵向速度
     * MC 1.16.5: getMaxSpeedAirVertical()
     */
    [[nodiscard]] virtual f32 getMaxSpeedAirVertical() const { return m_maxSpeedAirVertical; }

    /**
     * @brief 获取空气阻力
     * MC 1.16.5: getDragAir()
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
     * MC 1.16.5: getDamage()
     */
    [[nodiscard]] i32 getDamage() const { return m_damage; }

    /**
     * @brief 设置损坏值
     */
    void setDamage(i32 damage) { m_damage = damage; }

    /**
     * @brief 获取摇晃幅度
     * MC 1.16.5: getRollingAmplitude()
     */
    [[nodiscard]] i32 getRollingAmplitude() const { return m_rollingAmplitude; }

    /**
     * @brief 获取摇晃方向
     * MC 1.16.5: getRollingDirection()
     */
    [[nodiscard]] i32 getRollingDirection() const { return m_rollingDirection; }

    // ========== 乘客和碰撞 ==========

    /**
     * @brief 乘客乘坐高度偏移
     * MC 1.16.5: getMountedYOffset() -> 0.0D
     */
    [[nodiscard]] f32 getMountedYOffset() const { return 0.0f; }

    /**
     * @brief 应用力到矿车上（被推动时）
     * MC 1.16.5: addVelocity()
     * @param x X方向力
     * @param z Z方向力
     */
    void applyForce(f32 x, f32 z);

    // ========== 虚方法 ==========

    /**
     * @brief 掉落矿车物品
     * MC 1.16.5: killMinecart()
     */
    virtual void dropItem();

    /**
     * @brief 激活矿车（如熔炉矿车）
     * MC 1.16.5: activate()
     */
    virtual void activate();

    /**
     * @brief 检查矿车是否已激活
     */
    [[nodiscard]] virtual bool isActivated() const { return false; }

    /**
     * @brief 激活铁轨通过回调
     * MC 1.16.5: onActivatorRailPass()
     * @param x 铁轨X坐标
     * @param y 铁轨Y坐标
     * @param z 铁轨Z坐标
     * @param powered 是否充能
     */
    virtual void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered);

    /**
     * @brief 应用摩擦力
     * MC 1.16.5: applyDrag()
     */
    virtual void applyDrag();

protected:
    /**
     * @brief 检查当前位置的铁轨并更新状态
     */
    void checkRailState();

    /**
     * @brief 沿铁轨移动
     * MC 1.16.5: moveAlongTrack()
     * @param pos 铁轨位置
     */
    void moveAlongTrack(const BlockPos& pos);

    /**
     * @brief 脱轨移动
     * MC 1.16.5: moveDerailedMinecart()
     */
    void moveDerailedMinecart();

    /**
     * @brief 计算铁轨方向向量
     * @param shape 铁轨形状
     * @return 方向向量对 (起点方向, 终点方向)
     */
    [[nodiscard]] std::pair<Vector3, Vector3> getRailDirectionVectors(RailShape shape) const;

    /**
     * @brief 获取铁轨上的贴靠位置
     * MC 1.16.5: getPos()
     * @param x 当前X
     * @param y 当前Y
     * @param z 当前Z
     * @return 贴靠后的位置
     */
    [[nodiscard]] Vector3 getPosOnRail(f64 x, f64 y, f64 z) const;

    /**
     * @brief 获取斜坡调整值
     * MC 1.16.5: getSlopeAdjustment() -> 0.0078125D
     */
    [[nodiscard]] static constexpr f64 getSlopeAdjustment() { return SLOPE_ADJUSTMENT; }

    /**
     * @brief 处理实体碰撞
     */
    void handleEntityCollisions();

    /**
     * @brief 处理矿车间碰撞
     */
    void handleMinecartCollisions();

    /**
     * @brief 更新摇晃动画
     */
    void updateRollingAnimation();

    /**
     * @brief 更新矿车朝向
     */
    void updateRotation();

    // ========== 铁轨类型检测 ==========

    /**
     * @brief 检查是否为动力铁轨
     */
    [[nodiscard]] bool isPoweredRail(const BlockPos& pos) const;

    /**
     * @brief 检查是否为探测铁轨
     */
    [[nodiscard]] bool isDetectorRail(const BlockPos& pos) const;

    /**
     * @brief 检查是否为激活铁轨
     */
    [[nodiscard]] bool isActivatorRail(const BlockPos& pos) const;

    /**
     * @brief 检查铁轨是否充能
     */
    [[nodiscard]] bool isRailPowered(const BlockPos& pos) const;

private:
    // 矿车类型
    Type m_type;

    // 铁轨状态
    bool m_onRail = false;
    BlockPos m_railPos;
    RailShape m_railShape = RailShape::NorthSouth;
    bool m_flipped = false;  // MC 1.16.5: isInReverse

    // 速度
    f32 m_maxSpeed = DEFAULT_MAX_SPEED;
    f32 m_maxSpeedAirLateral = DEFAULT_MAX_SPEED_AIR_LATERAL;
    f32 m_maxSpeedAirVertical = DEFAULT_MAX_SPEED_AIR_VERTICAL;
    f32 m_dragAir = DEFAULT_AIR_DRAG;

    // 损坏和动画
    i32 m_damage = 0;
    i32 m_rollingAmplitude = 0;
    i32 m_rollingDirection = 1;

    // 推动力（熔炉矿车用）
    f32 m_pushX = 0.0f;
    f32 m_pushZ = 0.0f;

    // MC 1.16.5 常量
    static constexpr f32 DEFAULT_MAX_SPEED = 0.4f;           // 最大铁轨速度
    static constexpr f32 DEFAULT_MAX_SPEED_AIR_LATERAL = 0.4f; // 空中最大横向速度
    static constexpr f32 DEFAULT_MAX_SPEED_AIR_VERTICAL = -1.0f; // 空中最大纵向速度（-1表示禁用）
    static constexpr f32 DEFAULT_AIR_DRAG = 0.95f;           // 空气阻力
    static constexpr f64 SLOPE_ADJUSTMENT = 0.0078125;       // 斜坡调整值 (1/128)
    static constexpr f32 POWERED_RAIL_BOOST = 0.06f;         // 动力铁轨加速
    static constexpr f32 UNPOWERED_RAIL_THRESHOLD = 0.03f;   // 无动力铁轨停止阈值
    static constexpr f32 OCCUPIED_DRAG = 0.997f;             // 有乘客摩擦
    static constexpr f32 EMPTY_DRAG = 0.96f;                 // 无乘客摩擦
    static constexpr f32 DAMAGE_THRESHOLD = 40.0f;           // 摧毁阈值
};

/**
 * @brief 普通矿车
 * MC 1.16.5 MinecartEntity
 */
class RideableMinecartEntity : public AbstractMinecartEntity {
public:
    RideableMinecartEntity(EntityId id)
        : AbstractMinecartEntity(Type::Rideable, id) {}

    /**
     * @brief 激活铁轨通过时弹出乘客
     * MC 1.16.5: onActivatorRailPass()
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;
};

/**
 * @brief 箱子矿车
 * MC 1.16.5 ChestMinecartEntity (ContainerMinecartEntity)
 */
class ChestMinecartEntity : public AbstractMinecartEntity {
public:
    ChestMinecartEntity(EntityId id)
        : AbstractMinecartEntity(Type::Chest, id) {}

    /**
     * @brief 箱子矿车有额外的摩擦力
     * MC 1.16.5: applyDrag() with redstone signal calculation
     */
    void applyDrag() override;

    // TODO: 实现库存系统
    // Inventory* getInventory();
};

/**
 * @brief 熔炉矿车
 * MC 1.16.5 FurnaceMinecartEntity
 */
class FurnaceMinecartEntity : public AbstractMinecartEntity {
public:
    FurnaceMinecartEntity(EntityId id)
        : AbstractMinecartEntity(Type::Furnace, id) {}

    void tick() override;

    [[nodiscard]] bool isActivated() const override { return m_fuel > 0; }

    /**
     * @brief 熔炉矿车速度较慢
     * MC 1.16.5: getMaximumSpeed() -> 0.2D
     */
    [[nodiscard]] f32 getMaxSpeed() const override { return 0.2f; }

    /**
     * @brief 添加燃料
     */
    void addFuel(i32 ticks) { m_fuel += ticks; }

    /**
     * @brief 获取剩余燃料
     */
    [[nodiscard]] i32 getFuel() const { return m_fuel; }

    /**
     * @brief 熔炉矿车摩擦力计算
     * MC 1.16.5: applyDrag()
     */
    void applyDrag() override;

    /**
     * @brief 激活熔炉矿车
     */
    void activate() override;

private:
    i32 m_fuel = 0;
    f32 m_pushX = 0.0f;
    f32 m_pushZ = 0.0f;
};

/**
 * @brief TNT矿车
 * MC 1.16.5 TNTMinecartEntity
 */
class TNTMinecartEntity : public AbstractMinecartEntity {
public:
    TNTMinecartEntity(EntityId id)
        : AbstractMinecartEntity(Type::TNT, id) {}

    void tick() override;

    /**
     * @brief 点燃TNT
     */
    void prime() { m_fuse = 80; }

    /**
     * @brief 是否已点燃
     */
    [[nodiscard]] bool isPrimed() const { return m_fuse > 0; }

    /**
     * @brief 激活铁轨点燃TNT
     * MC 1.16.5: onActivatorRailPass()
     */
    void onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) override;

private:
    /**
     * @brief 爆炸
     */
    void explode();

    i32 m_fuse = -1;
};

/**
 * @brief 漏斗矿车
 * MC 1.16.5 HopperMinecartEntity (ContainerMinecartEntity)
 */
class HopperMinecartEntity : public AbstractMinecartEntity {
public:
    HopperMinecartEntity(EntityId id)
        : AbstractMinecartEntity(Type::Hopper, id) {}

    void tick() override;

    /**
     * @brief 是否可以从上方吸取物品
     */
    [[nodiscard]] bool canSuckItems() const { return m_suckCooldown <= 0; }

    // TODO: 实现漏斗逻辑
    // void collectItems();
    // void transferItems();

private:
    i32 m_suckCooldown = 0;
};

/**
 * @brief 命令方块矿车
 * MC 1.16.5 MinecartCommandBlockEntity
 */
class CommandBlockMinecartEntity : public AbstractMinecartEntity {
public:
    CommandBlockMinecartEntity(EntityId id)
        : AbstractMinecartEntity(Type::CommandBlock, id) {}

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

private:
    /**
     * @brief 执行命令
     */
    void executeCommand();

    std::string m_command;
    std::string m_lastOutput;
    i32 m_successCount = 0;
};

} // namespace entity
} // namespace mc
