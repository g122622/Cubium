#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../basic/AnimalEntity.hpp"
#include <memory>
#include <optional>
#include <random>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class DamageSource;

/**
 * @brief 熊猫实体
 *
 * 具有独特性格基因的被动动物。
 *
 * 特性：
 * - 7种性格：普通、懒惰、忧愁、顽皮、好斗、棕色、虚弱
 * - 吃竹子：繁殖食物
 * - 打喷嚏：幼体可能打喷嚏喷出粘液球
 * - 打滚：顽皮熊猫会打滚
 * - 救助：好斗熊猫会保护其他熊猫
 * - 懒惰：懒惰熊猫经常躺着
 * - 忧愁：忧愁熊猫下雨时吃竹子
 * - 棕色：稀有棕色变种
 * - 虚弱：虚弱熊猫生病且生命值低
 *
 * 参考 MC 1.16.5 PandaEntity
 */
class PandaEntity : public AnimalEntity {
public:
    /**
     * @brief 熊猫性格基因
     */
    enum class Personality : u8 {
        Normal = 0,        // 普通
        Lazy = 1,          // 懒惰
        Worried = 2,       // 忧愁
        Playful = 3,       // 顽皮
        Aggressive = 4,    // 好斗
        Weak = 5,          // 虚弱
        Brown = 6,         // 棕色（普通性格）
        AggressiveLazy = 7 // 好斗懒惰（隐藏）
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    PandaEntity(LegacyEntityType type, EntityId id);
    ~PandaEntity() override = default;

    // 禁止拷贝
    PandaEntity(const PandaEntity&) = delete;
    PandaEntity& operator=(const PandaEntity&) = delete;

    // 允许移动
    PandaEntity(PandaEntity&&) = default;
    PandaEntity& operator=(PandaEntity&&) = default;

    /**
     * @brief 创建熊猫实体
     * @param world 世界实例
     * @return 新的熊猫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 性格 ==========

    /**
     * @brief 获取性格
     */
    [[nodiscard]] Personality getPersonality() const { return m_personality; }

    /**
     * @brief 设置性格
     */
    void setPersonality(Personality personality) { m_personality = personality; }

    /**
     * @brief 随机生成性格
     */
    void randomizePersonality();

    /**
     * @brief 是否是懒惰性格
     */
    [[nodiscard]] bool isLazy() const { return m_personality == Personality::Lazy; }

    /**
     * @brief 是否是好斗性格
     */
    [[nodiscard]] bool isAggressive() const { return m_personality == Personality::Aggressive; }

    /**
     * @brief 是否是顽皮性格
     */
    [[nodiscard]] bool isPlayful() const { return m_personality == Personality::Playful; }

    /**
     * @brief 是否是忧愁性格
     */
    [[nodiscard]] bool isWorried() const { return m_personality == Personality::Worried; }

    /**
     * @brief 是否是虚弱性格
     */
    [[nodiscard]] bool isWeak() const { return m_personality == Personality::Weak; }

    /**
     * @brief 是否是棕色变种
     */
    [[nodiscard]] bool isBrown() const { return m_personality == Personality::Brown; }

    // ========== 行为状态 ==========

    /**
     * @brief 是否正在打滚
     */
    [[nodiscard]] bool isRolling() const { return m_rolling; }

    /**
     * @brief 设置打滚状态
     */
    void setRolling(bool rolling) { m_rolling = rolling; }

    /**
     * @brief 是否正在打喷嚏
     */
    [[nodiscard]] bool isSneezing() const { return m_sneezing; }

    /**
     * @brief 设置打喷嚏状态
     */
    void setSneezing(bool sneezing) { m_sneezing = sneezing; }

    /**
     * @brief 是否正在吃东西
     */
    [[nodiscard]] bool isEating() const { return m_eating; }

    /**
     * @brief 设置吃东西状态
     */
    void setEating(bool eating) { m_eating = eating; }

    /**
     * @brief 是否正在躺着
     */
    [[nodiscard]] bool isLying() const { return m_lying; }

    /**
     * @brief 设置躺着状态
     */
    void setLying(bool lying) { m_lying = lying; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.6f : 1.2f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 根据性格返回不同音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 播放进食音效
     */
    void playEatSound();

    /**
     * @brief 播放打喷嚏音效
     */
    void playSneezeSound();

    /**
     * @brief 播放咬音效
     */
    void playBiteSound();

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 刻更新 ==========
    void tick() override;

private:
    // 性格
    Personality m_personality = Personality::Normal;

    // 行为状态
    bool m_rolling = false;
    bool m_sneezing = false;
    bool m_eating = false;
    bool m_lying = false;

    // 计时器
    i32 m_rollTimer = 0;
    i32 m_sneezeTimer = 0;
    i32 m_eatTimer = 0;
    i32 m_lyingTimer = 0;

    // 基因（用于遗传）
    u8 m_mainGene = 0;
    u8 m_hiddenGene = 0;
};

} // namespace mc
