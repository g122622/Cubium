#pragma once

#include "TameableEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>
#include <random>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 猫实体
 *
 * 可驯服的猫科动物，有多种皮肤。
 *
 * 特性：
 * - 驯服：用生鳕鱼或生鲑鱼驯服
 * - 11种皮肤：虎斑、西服、姜黄、暹罗、英国短毛等
 * - 恐惧：未驯服时会逃离玩家
 * - 跟随主人：驯服后跟随
 * - 坐下/站起：右键切换
 * - 礼物：驯服后给玩家带来礼物（羽毛、腐肉等）
 * - 爬树：可以爬墙/树
 * - 驱逐爬行者：爬行者会避开猫
 * - 驱逐幻翼：幻翼会避开猫
 *
 * 音效：
 * - ENTITY_CAT_AMBIENT: 喵叫声（驯服后）
 * - ENTITY_CAT_STRAY_AMBIENT: 流浪猫叫声（未驯服）
 * - ENTITY_CAT_PURR: 呼噜声（驯服后空闲）
 * - ENTITY_CAT_PURREOW: 长喵（求食）
 * - ENTITY_CAT_HURT: 受伤音效
 * - ENTITY_CAT_DEATH: 死亡音效
 * - ENTITY_CAT_HISS: 嘶嘶声（恐惧时）
 * - ENTITY_CAT_EAT: 进食音效
 * - ENTITY_CAT_BEG_FOR_FOOD: 乞食音效
 *
 * 参考 MC 1.16.5 CatEntity
 */
class CatEntity : public TameableEntity {
public:
    /**
     * @brief 猫皮肤类型
     */
    enum class CatType : u8 {
        Tabby = 0,        // 虎斑猫
        Black = 1,        // 黑猫
        Red = 2,          // 红猫/姜黄猫
        Siamese = 3,      // 暹罗猫
        BritishShorthair = 4, // 英国短毛猫
        Calico = 5,       // 三花猫
        Persian = 6,      // 波斯猫
        Ragdoll = 7,      // 布偶猫
        White = 8,        // 白猫
        Jellie = 9,       // Jellie猫（社区投票）
        AllBlack = 10     // 全黑猫（万圣节）
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    CatEntity(LegacyEntityType type, EntityId id);
    ~CatEntity() override = default;

    // 禁止拷贝
    CatEntity(const CatEntity&) = delete;
    CatEntity& operator=(const CatEntity&) = delete;

    // 允许移动
    CatEntity(CatEntity&&) = default;
    CatEntity& operator=(CatEntity&&) = default;

    /**
     * @brief 创建猫实体
     * @param world 世界实例
     * @return 新的猫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 皮肤类型 ==========

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] CatType getCatType() const { return m_catType; }

    /**
     * @brief 设置皮肤类型
     */
    void setCatType(CatType type) { m_catType = type; }

    /**
     * @brief 随机设置皮肤类型
     */
    void setRandomCatType();

    // ========== 驯服系统 ==========

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 如果是生鱼返回true
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const;

    /**
     * @brief 检查物品是否可用于繁殖
     * @param itemStack 物品堆
     * @return 如果是生鱼返回true
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于治疗
     * @param itemStack 物品堆
     * @return 如果是生鱼返回true
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    // ========== 繁殖 ==========

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 行为 ==========

    /**
     * @brief 检查是否在树上/高处
     */
    [[nodiscard]] bool isTreeClimbing() const { return m_treeClimbing; }

    /**
     * @brief 设置树上攀爬状态
     */
    void setTreeClimbing(bool climbing) { m_treeClimbing = climbing; }

    /**
     * @brief 检查是否恐惧（逃跑模式）
     * 未驯服的猫会逃离玩家
     */
    [[nodiscard]] bool isFleeing() const { return m_fleeing; }

    /**
     * @brief 设置恐惧状态
     */
    void setFleeing(bool fleeing) { m_fleeing = fleeing; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.35f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 驯服后使用 ENTITY_CAT_AMBIENT，未驯服使用 ENTITY_CAT_STRAY_AMBIENT
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

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.6f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.7f; }

    // ========== 驯服回调 ==========
    void onTamed(bool tamed) override;

private:
    // 皮肤类型
    CatType m_catType = CatType::Tabby;

    // 攀爬状态
    bool m_treeClimbing = false;

    // 恐惧状态（未驯服时逃离玩家）
    bool m_fleeing = false;

    // 礼物计时器
    i32 m_giftTimer = 0;

    // 常量
    static constexpr i32 GIFT_INTERVAL = 24000; // 礼物间隔（1200秒 = 20分钟）
};

} // namespace mc
