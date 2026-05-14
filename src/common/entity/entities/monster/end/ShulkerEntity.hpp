#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../MonsterEntity.hpp"
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
 * - 贝壳防御：闭合时免疫大部分伤害
 * - 悬浮攻击：发射子弹使目标悬浮
 * - 瞬移：受到伤害时会瞬移
 * - 变色：外壳颜色会渐变
 *
 * 参考 MC 1.16.5 ShulkerEntity
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
        Purple2 = 11,  // 紫色
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
     * @param type 实体类型
     * @param id 实体ID
     */
    ShulkerEntity(LegacyEntityType type, EntityId id);
    ~ShulkerEntity() override = default;

    // 禁止拷贝
    ShulkerEntity(const ShulkerEntity&) = delete;
    ShulkerEntity& operator=(const ShulkerEntity&) = delete;

    // 允许移动
    ShulkerEntity(ShulkerEntity&&) = default;
    ShulkerEntity& operator=(ShulkerEntity&&) = default;

    /**
     * @brief 创建潜影贝实体
     * @param world 世界实例
     * @return 新的潜影贝实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 颜色系统 ==========

    /**
     * @brief 获取颜色
     */
    [[nodiscard]] ShulkerColor getColor() const { return m_color; }

    /**
     * @brief 设置颜色
     */
    void setColor(ShulkerColor color) { m_color = color; }

    // ========== 贝壳状态 ==========

    /**
     * @brief 获取贝壳状态
     */
    [[nodiscard]] ShellState getShellState() const { return m_shellState; }

    /**
     * @brief 是否贝壳打开
     */
    [[nodiscard]] bool isShellOpen() const { return m_shellState == ShellState::Open; }

    /**
     * @brief 是否贝壳闭合
     */
    [[nodiscard]] bool isShellClosed() const { return m_shellState == ShellState::Closed; }

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
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    /**
     * @brief 获取攻击冷却
     */
    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }

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
     */
    void teleport();

    // ========== 方向 ==========

    /**
     * @brief 获取朝向
     */
    [[nodiscard]] BlockPos getAttachmentPos() const { return m_attachmentPos; }

    /**
     * @brief 设置附着位置
     */
    void setAttachmentPos(const BlockPos& pos) { m_attachmentPos = pos; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.5f; }

    /**
     * @brief 潜影贝不会燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

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

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 更新贝壳状态
    void updateShellState();

    // 颜色
    ShulkerColor m_color = ShulkerColor::Purple;

    // 贝壳状态
    ShellState m_shellState = ShellState::Closed;
    i32 m_shellStateTime = 0;

    // 攻击状态
    bool m_attacking = false;
    i32 m_attackCooldown = 0;
    f32 m_prevRenderOffset = 0.0f;

    // 附着位置
    BlockPos m_attachmentPos;

    // 常量
    static constexpr i32 OPEN_DURATION = 20;        // 打开动画时间
    static constexpr i32 CLOSE_DURATION = 20;       // 关闭动画时间
    static constexpr i32 ATTACK_COOLDOWN = 40;      // 攻击冷却
    static constexpr f32 BULLET_DAMAGE = 4.0f;      // 子弹伤害
    static constexpr i32 LEVITATION_DURATION = 100; // 悬浮持续时间
};

} // namespace mc
