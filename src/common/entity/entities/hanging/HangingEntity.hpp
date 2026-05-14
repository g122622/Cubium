#pragma once

#include "../../core/Entity.hpp"
#include "../../../world/block/BlockPos.hpp"
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;

namespace entity {

/**
 * @brief 悬挂实体基类
 *
 * 可以挂在墙上的实体（画、物品展示框、 leash knot）。
 *
 * 参考 MC 1.16.5 AbstractDecorationEntity
 */
class HangingEntity : public Entity {
public:
    /**
     * @brief 悬挂方向
     */
    enum class Direction : u8 {
        SOUTH = 0,
        WEST = 1,
        NORTH = 2,
        EAST = 3
    };

    HangingEntity();
    HangingEntity(BlockPos pos, Direction direction);
    ~HangingEntity() override = default;

    // Entity overrides
    void tick() override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }

    /**
     * @brief 设置悬挂位置
     */
    void setHangingPosition(BlockPos pos, Direction direction);

    /**
     * @brief 获取悬挂的方块位置
     */
    [[nodiscard]] BlockPos getHangingBlockPos() const { return m_hangingPos; }

    /**
     * @brief 获取悬挂方向
     */
    [[nodiscard]] Direction getDirection() const { return m_direction; }

    /**
     * @brief 检查是否有效悬挂位置
     */
    [[nodiscard]] bool isValidPosition() const;

    /**
     * @brief 检查方块是否可以支撑悬挂物
     */
    [[nodiscard]] bool canPlaceOn() const;

    /**
     * @brief 被攻击
     * @param attacker 攻击者
     * @param damage 伤害值
     */
    void onAttacked(Entity* attacker, f32 damage);

    /**
     * @brief 掉落物品
     */
    virtual void dropItem() = 0;

    /**
     * @brief 获取宽度（方块单位）
     */
    [[nodiscard]] virtual i32 getWidth() const = 0;

    /**
     * @brief 获取高度（方块单位）
     */
    [[nodiscard]] virtual i32 getHeight() const = 0;

protected:
    /**
     * @brief 更新边界框
     */
    void updateBoundingBox();

    BlockPos m_hangingPos;
    Direction m_direction = Direction::SOUTH;
    i32 m_checkInterval = 0;
    static constexpr i32 CHECK_INTERVAL = 100; // 每100tick检查一次
};

/**
 * @brief 画实体
 *
 * 可以挂在墙上的装饰画。
 * 有多种尺寸。
 *
 * 参考 MC 1.16.5 PaintingEntity
 */
class PaintingEntity : public HangingEntity {
public:
    /**
     * @brief 画作类型
     */
    struct PaintingType {
        std::string name;
        i32 width;
        i32 height;
    };

    PaintingEntity();
    PaintingEntity(BlockPos pos, Direction direction, const std::string& motive);

    void dropItem() override;

    [[nodiscard]] i32 getWidth() const override;
    [[nodiscard]] i32 getHeight() const override;

    [[nodiscard]] const std::string& getMotive() const { return m_motive; }
    void setMotive(const std::string& motive);

private:
    std::string m_motive = "Kebab"; // 默认画作

    // 可用画作列表
    static const std::vector<PaintingType> PAINTING_TYPES;
};

/**
 * @brief 物品展示框实体
 *
 * 可以展示物品的框架。
 *
 * 参考 MC 1.16.5 ItemFrameEntity
 */
class ItemFrameEntity : public HangingEntity {
public:
    ItemFrameEntity();
    ItemFrameEntity(BlockPos pos, Direction direction);

    void tick() override;
    void dropItem() override;

    [[nodiscard]] i32 getWidth() const override { return 1; }
    [[nodiscard]] i32 getHeight() const override { return 1; }

    /**
     * @brief 设置展示的物品
     */
    void setItem(const ItemEntity& item);

    /**
     * @brief 获取展示的物品
     */
    [[nodiscard]] const ItemEntity* getItem() const { return m_item; }

    /**
     * @brief 设置物品旋转
     * @param rotation 0-7（每45度一个位置）
     */
    void setItemRotation(i32 rotation);
    [[nodiscard]] i32 getItemRotation() const { return m_rotation; }

    /**
     * @brief 旋转物品
     */
    void rotateItem();

    /**
     * @brief 检查是否为无形展示框（Glow Item Frame）
     */
    [[nodiscard]] bool isGlowing() const { return m_glowing; }
    void setGlowing(bool glowing) { m_glowing = glowing; }

private:
    ItemEntity* m_item = nullptr;
    i32 m_rotation = 0;
    bool m_glowing = false;
};

/**
 * @brief 拴绳结实体
 *
 * 多条拴绳连接的点。
 *
 * 参考 MC 1.16.5 LeashKnotEntity
 */
class LeashKnotEntity : public HangingEntity {
public:
    LeashKnotEntity();
    LeashKnotEntity(BlockPos pos, Direction direction);

    void tick() override;
    void dropItem() override;

    [[nodiscard]] i32 getWidth() const override { return 1; }
    [[nodiscard]] i32 getHeight() const override { return 1; }

    /**
     * @brief 绑定拴绳
     */
    void attachLeash(Entity* entity);

    /**
     * @brief 解绑拴绳
     */
    void detachLeash(Entity* entity);

    /**
     * @brief 获取所有绑定的实体
     */
    [[nodiscard]] const std::vector<Entity*>& getLeashedEntities() const { return m_leashedEntities; }

private:
    std::vector<Entity*> m_leashedEntities;
};

} // namespace entity
} // namespace mc
