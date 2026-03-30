#pragma once

#include "../../core/Entity.hpp"
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;
class BlockState;

namespace entity {

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
        RIDEABLE = 0,       // 普通矿车
        CHEST = 1,          // 箱子矿车
        FURNACE = 2,        // 熔炉矿车
        TNT = 3,            // TNT矿车
        SPAWNER = 4,        // 刷怪笼矿车
        HOPPER = 5,         // 漏斗矿车
        COMMAND_BLOCK = 6   // 命令方块矿车
    };

    explicit AbstractMinecartEntity(Type type = Type::RIDEABLE);
    ~AbstractMinecartEntity() override = default;

    // Entity overrides
    void tick() override;

    // Minecart-specific methods
    [[nodiscard]] Type getMinecartType() const { return m_type; }
    [[nodiscard]] f32 getMaxSpeed() const { return m_maxSpeed; }
    void setMaxSpeed(f32 speed) { m_maxSpeed = speed; }

    /**
     * @brief 获取当前轨道位置
     */
    void adjustOnRail();

    /**
     * @brief 沿铁轨移动
     * @param distance 移动距离
     */
    void moveAlongRail(f32 distance);

    /**
     * @brief 检查是否在铁轨上
     */
    [[nodiscard]] bool isOnRail() const { return m_onRail; }

    /**
     * @brief 掉落矿车物品
     */
    virtual void dropItem();

    /**
     * @brief 激活矿车（如熔炉矿车）
     */
    virtual void activate();

    /**
     * @brief 检查矿车是否已激活
     */
    [[nodiscard]] virtual bool isActivated() const { return false; }

    /**
     * @brief 获取矿车的损坏值
     */
    [[nodiscard]] i32 getDamage() const { return m_damage; }
    void setDamage(i32 damage) { m_damage = damage; }

    /**
     * @brief 应用力到矿车上（被推动时）
     * @param x X方向力
     * @param z Z方向力
     */
    void applyForce(f32 x, f32 z);

protected:
    /**
     * @brief 处理铁轨逻辑
     */
    void handleRailLogic();

    /**
     * @brief 计算轨道方向
     */
    void calculateRailDirection();

    /**
     * @brief 处理轨道分支
     */
    void handleRailJunction();

    /**
     * @brief 检查轨道是否为动力轨道
     */
    [[nodiscard]] bool isPoweredRail() const;

    /**
     * @brief 检查轨道是否为减速轨道
     */
    [[nodiscard]] bool isDetectorRail() const;

    /**
     * @brief 检查轨道是否为激活轨道
     */
    [[nodiscard]] bool isActivatorRail() const;

private:
    Type m_type;
    f32 m_maxSpeed = 0.4f;
    f32 m_velocityModifier = 0.0f;
    bool m_onRail = false;
    i32 m_damage = 0;
    i32 m_railDirection = 0; // 0=南北, 1=东西, 2=斜坡
    bool m_flipped = false;
    static constexpr f32 DEFAULT_MAX_SPEED = 0.4f;
    static constexpr f32 POWERED_RAIL_SPEED = 0.06f;
    static constexpr f32 FRICTION = 0.98f;
};

/**
 * @brief 普通矿车
 */
class RideableMinecartEntity : public AbstractMinecartEntity {
public:
    RideableMinecartEntity() : AbstractMinecartEntity(Type::RIDEABLE) {}
};

/**
 * @brief 箱子矿车
 */
class ChestMinecartEntity : public AbstractMinecartEntity {
public:
    ChestMinecartEntity() : AbstractMinecartEntity(Type::CHEST) {}

    // TODO: 实现库存系统
    // Inventory* getInventory();
};

/**
 * @brief 熔炉矿车
 */
class FurnaceMinecartEntity : public AbstractMinecartEntity {
public:
    FurnaceMinecartEntity() : AbstractMinecartEntity(Type::FURNACE) {}

    [[nodiscard]] bool isActivated() const override { return m_fuel > 0; }

    void addFuel(i32 ticks) { m_fuel += ticks; }
    [[nodiscard]] i32 getFuel() const { return m_fuel; }

    void tick() override {
        AbstractMinecartEntity::tick();
        if (m_fuel > 0) {
            m_fuel--;
            setMaxSpeed(0.4f);
        } else {
            setMaxSpeed(0.2f);
        }
    }

private:
    i32 m_fuel = 0;
};

/**
 * @brief TNT矿车
 */
class TNTMinecartEntity : public AbstractMinecartEntity {
public:
    TNTMinecartEntity() : AbstractMinecartEntity(Type::TNT) {}

    void prime() { m_fuse = 80; }

    void tick() override {
        AbstractMinecartEntity::tick();
        if (m_fuse > 0) {
            m_fuse--;
            if (m_fuse <= 0) {
                explode();
            }
        }
    }

private:
    void explode() {
        // TODO: 爆炸逻辑
        // world->createExplosion(position(), 4.0f);
        remove();
    }

    i32 m_fuse = -1;
};

/**
 * @brief 漏斗矿车
 */
class HopperMinecartEntity : public AbstractMinecartEntity {
public:
    HopperMinecartEntity() : AbstractMinecartEntity(Type::HOPPER) {}

    // TODO: 实现漏斗逻辑
    // void collectItems();
    // void transferItems();
};

/**
 * @brief 命令方块矿车
 */
class CommandBlockMinecartEntity : public AbstractMinecartEntity {
public:
    CommandBlockMinecartEntity() : AbstractMinecartEntity(Type::COMMAND_BLOCK) {}

    void setCommand(const std::string& command) { m_command = command; }
    [[nodiscard]] const std::string& getCommand() const { return m_command; }

    void executeCommand();

private:
    std::string m_command;
    std::string m_lastOutput;
    i32 m_successCount = 0;
};

} // namespace entity
} // namespace mc
