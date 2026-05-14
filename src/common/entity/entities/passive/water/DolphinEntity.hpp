#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../water/WaterMobEntity.hpp"
#include <memory>

namespace mc {

// Forward declarations
class LivingEntity;

/**
 * @brief 海豚实体
 *
 * 生活在海洋中的聪明哺乳动物。
 *
 * 特性：
 * - 游泳：快速游泳，可以跳出水面
 * - 寻找宝藏：喂食鱼后会引导玩家到宝藏
 * - 救助：会将溺水的玩家推向水面
 * - 群居：会形成小群体
 * - 掉落：生鳕鱼
 *
 * 音效：
 * - ENTITY_DOLPHIN_AMBIENT: 陆地环境音
 * - ENTITY_DOLPHIN_AMBIENT_WATER: 水中环境音
 * - ENTITY_DOLPHIN_ATTACK: 攻击音效
 * - ENTITY_DOLPHIN_DEATH: 死亡音效
 * - ENTITY_DOLPHIN_EAT: 进食音效
 * - ENTITY_DOLPHIN_HURT: 受伤音效
 * - ENTITY_DOLPHIN_JUMP: 跳跃落水音效
 * - ENTITY_DOLPHIN_PLAY: 玩耍物品音效
 * - ENTITY_DOLPHIN_SPLASH: 溅水音效
 * - ENTITY_DOLPHIN_SWIM: 游泳音效
 *
 * 参考 MC 1.16.5 DolphinEntity
 */
class DolphinEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    DolphinEntity(LegacyEntityType type, EntityId id);
    ~DolphinEntity() override = default;

    // 禁止拷贝
    DolphinEntity(const DolphinEntity&) = delete;
    DolphinEntity& operator=(const DolphinEntity&) = delete;

    // 允许移动
    DolphinEntity(DolphinEntity&&) = default;
    DolphinEntity& operator=(DolphinEntity&&) = default;

    /**
     * @brief 创建海豚实体
     * @param world 世界实例
     * @return 新的海豚实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 游泳行为 ==========

    /**
     * @brief 是否正在跳跃
     */
    [[nodiscard]] bool isJumping() const { return m_jumping; }

    /**
     * @brief 设置跳跃状态
     */
    void setJumping(bool jumping) override { m_jumping = jumping; }

    /**
     * @brief 是否可以跳出水
     */
    [[nodiscard]] bool canJumpOutOfWater() const;

    // ========== 宝藏寻找 ==========

    /**
     * @brief 获取宝藏位置
     */
    [[nodiscard]] const BlockPos& getTreasurePos() const { return m_treasurePos; }

    /**
     * @brief 设置宝藏位置
     */
    void setTreasurePos(const BlockPos& pos);

    /**
     * @brief 是否有宝藏目标
     */
    [[nodiscard]] bool hasTreasureTarget() const { return m_hasTreasure; }

    /**
     * @brief 清除宝藏目标
     */
    void clearTreasureTarget();

    /**
     * @brief 是否正在引导玩家
     */
    [[nodiscard]] bool isGuidingPlayer() const { return m_guidingPlayer; }

    /**
     * @brief 设置引导玩家状态
     */
    void setGuidingPlayer(bool guiding, u64 playerId = 0);

    /**
     * @brief 获取被引导的玩家ID
     */
    [[nodiscard]] u64 getGuidedPlayerId() const { return m_guidedPlayerId; }

    // ========== 食物 ==========

    /**
     * @brief 检查物品是否是食物
     * 海豚吃鱼
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.3f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.9f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 0.6f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 在水中和陆地播放不同的音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 播放攻击音效
     */
    void playAttackSound(LivingEntity& target) override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 水状态回调 ==========
    void onLeaveWater() override;

private:
    // 跳跃状态
    bool m_jumping = false;

    // 宝藏寻找
    BlockPos m_treasurePos;
    bool m_hasTreasure = false;
    bool m_guidingPlayer = false;
    u64 m_guidedPlayerId = 0;
    i32 m_guideTimer = 0;

    // 游泳计时器
    i32 m_swimTimer = 0;

    // 常量
    static constexpr i32 GUIDE_DURATION = 1200; // 60秒引导时间
    static constexpr f32 SWIM_SPEED = 0.6f;
    static constexpr f32 JUMP_VELOCITY = 0.7f;
};

} // namespace mc
