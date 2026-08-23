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

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "entity/entities/monster/MonsterEntity.hpp"
#include "util/Direction.hpp"
#include "world/block/BlockPos.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declaration
class DamageSource;

/**
 * @brief 潜影贝实体
 *
 * 生活在末地城市的敌对生物，会发射追踪子弹。
 *
 * 特性：
 * - 贝壳防御：闭合时免疫大部分伤害（箭矢免疫）
 * - 悬浮攻击：发射子弹使目标悬浮
 * - 瞬移：受到伤害时会瞬移到附近位置
 * - 附着方块：附着在方块表面，不移动
 * - 护甲加成：闭合时获得额外护甲
 */
class ShulkerEntity : public MonsterEntity {
public:
    /**
     * @brief 潜影贝颜色
     */
    enum class ShulkerColor : u8 {
        Purple = 0,    // 紫色（默认）
        White = 1,     // 白色
        Orange = 2,    // 橙色
        Magenta = 3,   // 品红
        LightBlue = 4, // 浅蓝
        Yellow = 5,    // 黄色
        Lime = 6,      // 黄绿
        Pink = 7,      // 粉色
        Gray = 8,      // 灰色
        LightGray = 9, // 浅灰
        Cyan = 10,     // 青色
        Purple2 = 11,  // 紫色（另一种）
        Blue = 12,     // 蓝色
        Brown = 13,    // 棕色
        Green = 14,    // 绿色
        Red = 15,      // 红色
        Black = 16     // 黑色
    };

    /**
     * @brief 贝壳状态
     */
    enum class ShellState : u8 {
        Closed = 0,  // 闭合
        Opening = 1, // 正在打开
        Open = 2,    // 打开
        Closing = 3  // 正在关闭
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    ShulkerEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ShulkerEntity() override = default;

    // 禁止拷贝
    ShulkerEntity(const ShulkerEntity&) = delete;
    ShulkerEntity& operator=(const ShulkerEntity&) = delete;

    // 禁止移动（实体不支持移动语义）
    ShulkerEntity(ShulkerEntity&&) = delete;
    ShulkerEntity& operator=(ShulkerEntity&&) = delete;

    /**
     * @brief 创建潜影贝实体
     * @param world 世界实例
     * @return 新的潜影贝实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 颜色系统 ==========

    /**
     * @brief 获取颜色
     */
    [[nodiscard]] ShulkerColor getColor() const noexcept { return m_color; }

    /**
     * @brief 设置颜色
     */
    void setColor(ShulkerColor color) noexcept { m_color = color; }

    // ========== 贝壳状态 ==========

    /**
     * @brief 获取贝壳状态
     */
    [[nodiscard]] ShellState getShellState() const noexcept { return m_shellState; }

    /**
     * @brief 是否贝壳打开
     */
    [[nodiscard]] bool isShellOpen() const noexcept { return m_shellState == ShellState::Open; }

    /**
     * @brief 是否贝壳闭合
     */
    [[nodiscard]] bool isShellClosed() const noexcept { return m_shellState == ShellState::Closed; }

    /**
     * @brief 获取开壳程度（0.0-1.0）
     * 用于渲染动画
     */
    [[nodiscard]] f32 getPeekAmount() const noexcept { return m_peekAmount; }

    /**
     * @brief 获取上一tick的开壳程度
     */
    [[nodiscard]] f32 getPrevPeekAmount() const noexcept { return m_prevPeekAmount; }

    /**
     * @brief 获取当前开壳tick数
     */
    [[nodiscard]] i32 getPeekTicks() const noexcept { return m_peekTicks; }

    /**
     * @brief 更新开壳tick数（同步护甲和音效）
     * @param peekTicks 开壳tick数（0-100）
     */
    void updatePeekTicks(i32 peekTicks);

    /**
     * @brief 打开贝壳
     */
    void openShell();

    /**
     * @brief 关闭贝壳
     */
    void closeShell();

    // ========== 攻击系统 ==========

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const noexcept { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) noexcept { m_attacking = attacking; }

    /**
     * @brief 获取攻击冷却
     */
    [[nodiscard]] i32 getAttackCooldown() const noexcept { return m_attackCooldown; }

    /**
     * @brief 发射子弹
     */
    void shootBullet();

    // ========== 防御系统 ==========

    /**
     * @brief 闭合时是否免疫伤害
     */
    [[nodiscard]] bool isImmuneToDamage() const;

    /**
     * @brief 传送到新位置
     * @return 是否瞬移成功
     */
    bool teleport();

    // ========== 附着方向 ==========

    /**
     * @brief 获取附着方向
     */
    [[nodiscard]] Direction getAttachmentFacing() const noexcept { return m_attachmentFacing; }

    /**
     * @brief 设置附着方向
     */
    void setAttachmentFacing(Direction facing) noexcept { m_attachmentFacing = facing; }

    /**
     * @brief 获取附着位置
     */
    [[nodiscard]] BlockPos getAttachmentPos() const noexcept { return m_attachmentPos; }

    /**
     * @brief 设置附着位置
     */
    void setAttachmentPos(const BlockPos& pos) noexcept { m_attachmentPos = pos; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const noexcept override { return 0.5f; }

    /**
     * @brief 潜影贝不会燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const noexcept override { return false; }

    /**
     * @brief 碰撞箱边框
     * 潜影贝的碰撞箱会根据开壳程度扩展
     */
    [[nodiscard]] f32 getCollisionBorderSize() const noexcept override { return 0.0f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音效
     * 贝壳闭合时使用不同的音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 播放打开贝壳音效
     */
    void playOpenSound();

    /**
     * @brief 播放关闭贝壳音效
     */
    void playCloseSound();

    /**
     * @brief 播放发射子弹音效
     */
    void playShootSound();

    /**
     * @brief 播放瞬移音效
     */
    void playTeleportSound();

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 受伤处理
     * 闭合时对箭矢免疫
     */
    bool hurt(DamageSource& source, f32 amount) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 更新贝壳状态
    void _updateShellState();

    // 尝试瞬移到新位置
    bool _tryTeleportToNewPosition();

    // 检查是否可以附着在指定位置的指定方向
    [[nodiscard]] bool _canAttachAt(const BlockPos& pos, Direction facing) const;

    // 寻找可附着的方向
    [[nodiscard]] std::optional<Direction> _findValidFacing(const BlockPos& pos) const;

    // 被潜影弹命中时的瞬移+繁殖回调（对齐 vanilla Shulker.hitByShulkerBullet:440-455）。
    // 仅当贝壳打开（!isClosed）且 teleportSomewhere 成功时执行：在原位置附近 8 格范围内
    // 统计存活潜影贝数量 i，繁殖概率阈值 f=(i-1)/5.0，当 random.nextFloat() >= f 时在原位置
    // 生成一只同颜色（variant）的新潜影贝。潜影密度越高越不容易繁殖（i>=6 时 f>=1 必不繁殖）。
    void _hitByShulkerBullet(const Vector3& originalPos);

    // 颜色
    ShulkerColor m_color = ShulkerColor::Purple;

    // 贝壳状态
    ShellState m_shellState = ShellState::Closed;
    i32 m_shellStateTime = 0;
    i32 m_peekTicks = 0;         // 开壳tick数（0-100）
    f32 m_peekAmount = 0.0f;     // 当前开壳程度
    f32 m_prevPeekAmount = 0.0f; // 上一tick开壳程度

    // 攻击状态
    bool m_attacking = false;
    i32 m_attackCooldown = 0;

    // 附着位置和方向
    Direction m_attachmentFacing = Direction::Down;
    BlockPos m_attachmentPos;

    // 瞬移冷却
    i32 m_teleportCooldown = 0;

    // 常量
    static constexpr i32 OPEN_DURATION = 20;           // 打开动画时间
    static constexpr i32 CLOSE_DURATION = 20;          // 关闭动画时间
    static constexpr i32 ATTACK_COOLDOWN_MIN = 20;     // 最小攻击冷却
    static constexpr i32 ATTACK_COOLDOWN_RANDOM = 100; // 攻击冷却随机范围
    static constexpr f32 BULLET_DAMAGE = 4.0f;         // 子弹伤害
    static constexpr i32 LEVITATION_DURATION = 200;    // 悬浮持续时间（ticks）
    static constexpr i32 TELEPORT_COOLDOWN = 50;       // 瞬移冷却
    static constexpr i32 TELEPORT_RANGE = 8;           // 瞬移范围
    static constexpr i32 TELEPORT_ATTEMPTS = 5;        // 瞬移尝试次数
    static constexpr f32 ARMOR_BONUS = 20.0f;          // 闭合时护甲加成
    static constexpr f32 ATTACK_RANGE = 20.0f;         // 攻击范围（平方）
};

} // namespace mc
