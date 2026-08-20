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

#include <memory>
#include <string>
#include <vector>

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

// Forward declarations
class Player;
class ItemEntity;

namespace world::explosion {
struct ExplosionImmunityContext;
} // namespace world::explosion

namespace entity {

/**
 * @brief 悬挂实体基类
 *
 * 可以挂在墙上的实体（画、物品展示框、拴绳结）。
 */
class HangingEntity : public Entity {
public:
    /**
     * @brief 悬挂方向
     */
    enum class Direction : u8 { SOUTH = 0, WEST = 1, NORTH = 2, EAST = 3 };

    HangingEntity(ecs::EntityRegistry& registry);
    HangingEntity(BlockPos pos, Direction direction, ecs::EntityRegistry& registry);
    ~HangingEntity() override = default;

    // Entity overrides
    void tick() override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    /**
     * @brief 处理悬挂实体受到伤害
     *
     * 悬挂实体（画、物品展示框、拴绳结）被任何伤害一击即毁。
     * 当 mobGriefing 游戏规则关闭时，生物造成的伤害不会影响悬挂实体。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 判断悬挂实体是否忽略此次爆炸
     *
     * 直接源在水中的爆炸不破坏悬挂实体；否则仅当爆炸影响方块类实体时才受影响。
     */
    [[nodiscard]] bool ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const override;

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

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    PaintingEntity(ecs::EntityRegistry& registry);
    PaintingEntity(BlockPos pos, Direction direction, const std::string& motive, ecs::EntityRegistry& registry);

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
 * 红石特性：
 * - 可以输出红石比较器信号（1-8，取决于物品旋转角度）
 * - 无物品时输出 0
 * - 有物品时输出 rotation % 8 + 1
 */
class ItemFrameEntity : public HangingEntity {
public:
    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    ItemFrameEntity(ecs::EntityRegistry& registry);
    ItemFrameEntity(BlockPos pos, Direction direction, ecs::EntityRegistry& registry);

    void tick() override;
    void dropItem() override;

    /**
     * @brief 处理玩家与物品展示框的交互
     *
     * 当玩家右键点击物品展示框时：
     * - 空手/有物品时空展示框：放入物品
     * - 空手时有物品的展示框：旋转物品
     * - 潜行时有物品的展示框：取出物品
     */
    ActionResultType processInitialInteract(Player& player, Hand hand) override;

    [[nodiscard]] i32 getWidth() const override { return 1; }
    [[nodiscard]] i32 getHeight() const override { return 1; }

    /**
     * @brief 设置展示的物品
     * @param stack 要展示的物品堆
     * @param updateComparator 是否通知红石比较器更新（NBT加载时传false，交互时传true）
     */
    void setDisplayedItem(const ItemStack& stack, bool updateComparator = true);

    /**
     * @brief 获取展示的物品堆
     * @return 展示的物品堆（可能为空）
     */
    [[nodiscard]] const ItemStack& getDisplayedItem() const { return m_displayedItem; }

    /**
     * @brief 检查是否有展示物品
     */
    [[nodiscard]] bool hasItem() const { return !m_displayedItem.isEmpty(); }

    /**
     * @brief 设置物品旋转
     * @param rotation 0-7（每45度一个位置）
     * @param updateComparator 是否通知红石比较器更新（NBT加载时传false，交互时传true）
     */
    void setItemRotation(i32 rotation, bool updateComparator = true);

    /**
     * @brief 获取物品旋转值
     * @return 旋转值 0-7
     */
    [[nodiscard]] i32 getItemRotation() const { return m_rotation; }

    /**
     * @brief 旋转物品（右键交互时调用）
     */
    void rotateItem();

    /**
     * @brief 通知悬挂位置周围的红石比较器重新计算输入信号
     *
     * 当物品展示框的内容变化（放入/取出/旋转物品）时调用，
     * 使相邻的红石比较器能够感知到信号变化并更新状态。
     */
    void notifyComparatorUpdate();

    /**
     * @brief 获取红石比较器模拟输出信号
     *
     * 无物品: 返回 0
     * 有物品: 返回 rotation % 8 + 1（范围 1-8）
     *
     * @return 红石信号强度（0-8）
     */
    [[nodiscard]] i32 getAnalogOutput() const;

    /**
     * @brief 获取比较器输出信号强度
     *
     * 委托给 getAnalogOutput()，兼容 Entity::getComparatorOutput() 接口。
     *
     * @return 红石信号强度（0-8）
     */
    [[nodiscard]] i32 getComparatorOutput() const override { return getAnalogOutput(); }

    /**
     * @brief 获取水平朝向（MC 方向）
     *
     * 用于红石比较器检测物品展示框朝向。
     *
     * @return mc::Direction 水平朝向
     */
    [[nodiscard]] mc::Direction getHorizontalFacing() const;

    /**
     * @brief 检查是否为无形展示框（Glow Item Frame）
     */
    [[nodiscard]] bool isGlowing() const { return m_glowing; }
    void setGlowing(bool glowing) { m_glowing = glowing; }

private:
    ItemStack m_displayedItem; ///< 展示的物品堆
    i32 m_rotation = 0;        ///< 旋转值（0-7，每45度一个位置）
    bool m_glowing = false;    ///< 是否为无形展示框
};

/**
 * @brief 拴绳结实体
 *
 * 多条拴绳连接的点。
 */
class LeashKnotEntity : public HangingEntity {
public:
    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    LeashKnotEntity(ecs::EntityRegistry& registry);
    LeashKnotEntity(BlockPos pos, Direction direction, ecs::EntityRegistry& registry);

    void tick() override;
    void dropItem() override;

    /**
     * @brief 处理玩家与拴绳结的交互
     *
     * 当玩家右键点击拴绳结时，委派给 interact() 方法处理拴绳转移逻辑。
     */
    ActionResultType processInitialInteract(Player& player, Hand hand) override;

    [[nodiscard]] i32 getWidth() const override { return 1; }
    [[nodiscard]] i32 getHeight() const override { return 1; }

    /**
     * @brief 获取或创建指定栅栏位置的拴绳结
     *
     * 如果该位置已有拴绳结实体则复用，否则创建新的。
     *
     * @param world 世界实例
     * @param pos 栅栏方块位置
     * @return 拴绳结实体指针，失败返回 nullptr
     */
    static LeashKnotEntity* getOrCreateKnot(IWorld& world, const BlockPos& pos);

    /**
     * @brief 玩家与拴绳结交互
     *
     * 玩家右键拴绳结时的交互逻辑：
     * - 玩家手持拴绳且有拴住的生物：将生物转移到栅栏结上
     * - 玩家没有拴住的生物且不潜行：将栅栏结上的生物取回（拴到玩家身上）
     *
     * @param player 交互的玩家
     * @param hand 使用的手
     * @return 交互结果
     */
    ActionResultType interact(Player& player, Hand hand);

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

    /**
     * @brief 检查栅栏是否还在
     *
     * 拴绳结只存在于栅栏方块上方，如果栅栏被破坏则拴绳结也应销毁。
     */
    [[nodiscard]] bool survives() const;

    /**
     * @brief 播放放置音效
     */
    void playPlacementSound();

private:
    std::vector<Entity*> m_leashedEntities;
};

} // namespace entity
} // namespace mc
