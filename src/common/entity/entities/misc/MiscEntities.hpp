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
#include "../../core/Entity.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Block;
class Player;
class LivingEntity;
class DamageSource;

namespace entity {

/**
 * @brief 下落方块实体
 *
 * 沙子、砾石等方块下落时创建的实体。
 *
 * 参考 MC 1.16.5 FallingBlockEntity
 */
class FallingBlockEntity : public Entity {
public:
    FallingBlockEntity();
    ~FallingBlockEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.98f; }
    [[nodiscard]] f32 height() const override { return 0.98f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 设置方块ID
     */
    void setBlockId(u32 blockId) { m_blockId = blockId; }
    [[nodiscard]] u32 getBlockId() const { return m_blockId; }

    /**
     * @brief 设置是否在落地时造成伤害
     */
    void setHurtEntities(bool hurt) { m_hurtEntities = hurt; }
    [[nodiscard]] bool shouldHurtEntities() const { return m_hurtEntities; }

    /**
     * @brief 设置下落起始位置（用于计算伤害）
     */
    void setFallStartPos(f64 y) { m_fallStartY = y; }

    /**
     * @brief 检查是否应该放置方块
     */
    [[nodiscard]] bool shouldPlaceBlock() const { return m_placeBlock; }

    /**
     * @brief 设置是否应该掉落物品
     *
     * MC 1.16.5: shouldDropItem
     * 默认为 true。某些特殊方块（如损坏的铁砧）可能设置为 false。
     */
    void setShouldDropItem(bool drop) { m_shouldDropItem = drop; }
    [[nodiscard]] bool shouldDropItem() const { return m_shouldDropItem; }

    /**
     * @brief 设置是否不放置方块
     *
     * MC 1.16.5: dontSetBlock
     * 当铁砧损坏时会设置为 true，此时只调用 onBroken 回调。
     */
    void setDontSetBlock(bool dontSet) { m_dontSetBlock = dontSet; }
    [[nodiscard]] bool dontSetBlock() const { return m_dontSetBlock; }

private:
    void handleLanding();

    /**
     * @brief 尝试放置方块
     *
     * @param world 世界指针
     * @param landingPos 落地位置
     * @param fallingState 下落的方块状态
     * @param hitState 落地点的方块状态
     * @return 是否成功放置
     */
    bool tryPlaceBlock(IWorld* world, const BlockPos& landingPos, const BlockState* fallingState, const BlockState* hitState);

    /**
     * @brief 掉落物品
     *
     * @param world 世界指针
     * @param pos 掉落位置
     */
    void dropItem(IWorld* world, const BlockPos& pos);

    /**
     * @brief 伤害碰撞箱内的实体
     *
     * @param world 世界指针
     */
    void hurtEntities(IWorld* world);

    u32 m_blockId = 0;            ///< 方块ID
    bool m_hurtEntities = false;  ///< 是否伤害实体（铁砧=true）
    bool m_placeBlock = true;     ///< 是否应该放置方块
    bool m_shouldDropItem = true; ///< 是否应该掉落物品
    bool m_dontSetBlock = false;  ///< 是否不放置方块（铁砧损坏时）
    f64 m_fallStartY = 0.0;       ///< 下落起始Y坐标
    i32 m_fallTime = 0;           ///< 下落时间（tick）
    static constexpr f32 HURT_AMOUNT = 2.0f;      ///< 每格下落伤害系数
    static constexpr i32 MAX_HURT_AMOUNT = 40;    ///< 最大伤害值
    static constexpr i32 MAX_FALL_TIME = 600;     ///< 最大下落时间（30秒）
};

/**
 * @brief TNT实体
 *
 * 被激活的TNT方块，倒计时后爆炸。
 *
 * 参考 MC 1.16.5 TNTEntity
 */
class TNTEntity : public Entity {
public:
    TNTEntity();
    explicit TNTEntity(EntityId id);
    ~TNTEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.98f; }
    [[nodiscard]] f32 height() const override { return 0.98f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 获取爆炸倒计时
     */
    [[nodiscard]] i32 getFuse() const { return m_fuse; }
    void setFuse(i32 fuse) { m_fuse = fuse; }

    /**
     * @brief 设置爆炸半径
     */
    void setExplosionRadius(f32 radius) { m_explosionRadius = radius; }
    [[nodiscard]] f32 getExplosionRadius() const { return m_explosionRadius; }

    /**
     * @brief 点燃TNT
     *
     * 设置引信时间并开始倒计时。
     */
    void ignite();

    /**
     * @brief 设置点燃者
     * @param igniter 点燃TNT的实体（可为nullptr）
     *
     * 用于追踪爆炸责任的归属。
     */
    void setOwner(LivingEntity* igniter) { m_owner = igniter; }
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    /**
     * @brief 爆炸
     */
    void explode();

    /**
     * @brief 检查是否已点燃
     */
    [[nodiscard]] bool isPrimed() const { return m_fuse > 0; }

private:
    i32 m_fuse = 0;
    f32 m_explosionRadius = 4.0f;
    bool m_exploded = false;
    LivingEntity* m_owner = nullptr;
    static constexpr i32 DEFAULT_FUSE = 80; // 4秒（80 ticks）
};

/**
 * @brief 寂守者警告效果
 *
 * 寂守者检测到振动后产生的警告效果。
 * 不是实体，是效果的一种，但暂时放在这里。
 */
class WardenWarningEffect {
public:
    WardenWarningEffect() = default;
    ~WardenWarningEffect() = default;

    void tick();

    [[nodiscard]] i32 getWarningLevel() const { return m_warningLevel; }
    void increaseWarning();
    void decreaseWarning();

    [[nodiscard]] BlockPos getSourcePos() const { return m_sourcePos; }
    void setSourcePos(BlockPos pos) { m_sourcePos = pos; }

    [[nodiscard]] f32 getWarningRadius() const { return m_warningRadius; }

private:
    i32 m_warningLevel = 0;
    BlockPos m_sourcePos;
    f32 m_warningRadius = 10.0f;
    i32 m_cooldown = 0;
    static constexpr i32 MAX_WARNING = 4;
    static constexpr i32 DECREASE_INTERVAL = 200;
};

// 注意: EvokerFangsEntity 和 EyeOfEnderEntity 已移至
// src/common/entity/entities/projectile/OtherProjectiles.hpp 以避免重复定义

} // namespace entity
} // namespace mc
