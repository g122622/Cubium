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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "TameableEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/util/color/DyeColor.hpp"

#include <memory>

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
 */
class CatEntity : public TameableEntity {
public:
    /**
     * @brief 猫皮肤类型
     */
    enum class CatType : u8 {
        Tabby = 0,            // 虎斑猫
        Black = 1,            // 黑猫
        Red = 2,              // 红猫/姜黄猫
        Siamese = 3,          // 暹罗猫
        BritishShorthair = 4, // 英国短毛猫
        Calico = 5,           // 三花猫
        Persian = 6,          // 波斯猫
        Ragdoll = 7,          // 布偶猫
        White = 8,            // 白猫
        Jellie = 9,           // Jellie猫（社区投票）
        AllBlack = 10         // 全黑猫（万圣节）
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    CatEntity(EntityInstanceId id);
    ~CatEntity() override = default;

    // 禁止拷贝
    CatEntity(const CatEntity&) = delete;
    CatEntity& operator=(const CatEntity&) = delete;

    // 允许移动
    CatEntity(CatEntity&&) = delete;
    CatEntity& operator=(CatEntity&&) = delete;

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

    // ========== 动画状态 ==========

    /**
     * @brief 是否躺下（在床上或主人身边）
     */
    [[nodiscard]] bool isLieDown() const { return m_dataManager.get<bool>(DATA_LYING_PARAM); }

    /**
     * @brief 设置躺下状态
     */
    void setLieDown(bool lying) { m_dataManager.set(DATA_LYING_PARAM, lying); }

    /**
     * @brief 是否处于放松状态（看向睡眠主人）
     */
    [[nodiscard]] bool isRelaxStateOne() const { return m_dataManager.get<bool>(DATA_RELAX_STATE_ONE_PARAM); }

    /**
     * @brief 设置放松状态
     */
    void setRelaxStateOne(bool relax) { m_dataManager.set(DATA_RELAX_STATE_ONE_PARAM, relax); }

    /**
     * @brief 获取躺下动画进度（插值后）
     * @param partialTick 部分刻（用于插值）
     * @return 躺下动画进度 (0-1)
     */
    [[nodiscard]] f32 getLieDownAmount(f32 partialTick) const
    {
        return math::lerp(m_prevLieDownAmount, m_lieDownAmount, partialTick);
    }

    /**
     * @brief 获取躺下尾巴动画进度（插值后）
     * @param partialTick 部分刻（用于插值）
     * @return 躺下尾巴动画进度 (0-1)
     */
    [[nodiscard]] f32 getLieDownAmountTail(f32 partialTick) const
    {
        return math::lerp(m_prevLieDownAmountTail, m_lieDownAmountTail, partialTick);
    }

    /**
     * @brief 获取放松状态动画进度（插值后）
     * @param partialTick 部分刻（用于插值）
     * @return 放松状态动画进度 (0-1)
     */
    [[nodiscard]] f32 getRelaxStateOneAmount(f32 partialTick) const
    {
        return math::lerp(m_prevRelaxStateOneAmount, m_relaxStateOneAmount, partialTick);
    }

    /**
     * @brief 是否躺在睡眠玩家上方
     */
    [[nodiscard]] bool isLyingOnTopOfSleepingPlayer() const { return m_lyingOnTopOfSleepingPlayer; }

    // ========== 数据同步参数 ==========

    /**
     * @brief 获取躺下状态参数 ID（用于客户端元数据同步）
     */
    [[nodiscard]] static u16 getLyingParamId() { return DATA_LYING_PARAM.id(); }

    /**
     * @brief 获取放松状态参数 ID（用于客户端元数据同步）
     */
    [[nodiscard]] static u16 getRelaxStateOneParamId() { return DATA_RELAX_STATE_ONE_PARAM.id(); }

    // ========== 交互 ==========

    /**
     * @brief 玩家与猫交互
     *
     * 交互逻辑（按优先级）：
     * 1. 未驯服 + 猫食（生鳕鱼/生鲑鱼）→ 尝试驯服（1/3概率）
     * 2. 已驯服 + 主人 + 染料 → 改变项圈颜色
     * 3. 已驯服 + 主人 + 猫食 + 未满血 → 喂食治疗
     * 4. 已驯服 + 主人 + 父类未处理 → 切换坐下/站起
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 如果是生鱼返回true
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

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

    // ========== 项圈颜色 ==========

    /**
     * @brief 获取项圈颜色
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getCollarColor() const { return m_collarColor; }

    /**
     * @brief 设置项圈颜色
     * @param color 染料颜色
     */
    void setCollarColor(DyeColor color) { m_collarColor = color; }

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

    /**
     * @brief 发出嘶嘶声
     *
     * 当幻翼检测到附近的猫时，会调用此方法让猫发出嘶嘶声。
     * 对应 MC 原版 Cat.hiss()，仅播放 ENTITY_CAT_HISS 音效。
     */
    void hiss();

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据同步 ==========
    /**
     * @brief 注册同步数据参数
     *
     * 注册 DATA_LYING_PARAM（躺下状态）和 DATA_RELAX_STATE_ONE_PARAM（放松状态）到
     * EntityDataManager。客户端通过 getLyingParamId() / getRelaxStateOneParamId()
     * 读取参数 ID 并在元数据同步时更新客户端镜像状态。
     *
     * 注意：由于 C++ 虚函数在基类构造函数中不会派发到派生类，
     * CatEntity 构造函数必须显式调用此方法（参考 WolfEntity / ZombieVillagerEntity 模式）。
     */
    void registerData() override;

    // ========== Tick ==========
    void tick() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.6f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.7f; }

    // ========== 驯服回调 ==========
    void onTamed(bool tamed) override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    // ========== 内部 AI Goal 类 ==========

    /**
     * @brief 猫的食物诱惑目标
     *
     * 继承自 TemptGoal，重写 shouldExecute() 使其只在未驯服时执行。
     */
    class CatTemptGoal : public entity::ai::goal::TemptGoal {
    public:
        CatTemptGoal(CatEntity* cat, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement);

        [[nodiscard]] bool shouldExecute() override;

    private:
        CatEntity* m_cat;
    };

    /**
     * @brief 猫的避开玩家目标
     *
     * 继承自 AvoidEntityGoal，重写使其只在未驯服时避开玩家。
     */
    class CatAvoidPlayerGoal : public entity::ai::goal::AvoidEntityGoal {
    public:
        CatAvoidPlayerGoal(CatEntity* cat, f32 avoidDistance, f64 farSpeed, f64 nearSpeed);

        [[nodiscard]] bool shouldExecute() override;
        [[nodiscard]] bool shouldContinueExecuting() override;

    private:
        CatEntity* m_cat;
    };

    // ========== 数据同步参数 ==========
    static entity::DataParameter<bool> DATA_LYING_PARAM;
    static entity::DataParameter<bool> DATA_RELAX_STATE_ONE_PARAM;

    // ========== 动画状态更新 ==========
    void _updateLieDownAmount();
    void _updateRelaxStateOneAmount();
    void _handleLieDown();

    /**
     * @brief 设置驯服后的 AI
     *
     * 动态添加/移除 AvoidPlayerGoal
     */
    void _setupTamedAI();

    /**
     * @brief 尝试驯服猫
     * @param player 尝试驯服的玩家
     *
     * 1/3 概率驯服成功，成功时设置驯服状态和主人，
     * 广播 TamingSucceeded/TamingFailed 粒子效果。
     */
    void _tryToTame(Player& player);

    /**
     * @brief 获取染料物品对应的颜色
     * @param item 物品指针
     * @return 对应的 DyeColor，如果不是染料返回 std::nullopt
     */
    [[nodiscard]] static std::optional<DyeColor> _getDyeColorFromItem(const Item* item);

    // 皮肤类型
    CatType m_catType = CatType::Tabby;

    // 项圈颜色（默认红色，与 MC 原版一致）
    DyeColor m_collarColor = DyeColor::Red;

    // 攀爬状态
    bool m_treeClimbing = false;

    // 恐惧状态（未驯服时逃离玩家）
    bool m_fleeing = false;

    // 礼物计时器
    i32 m_giftTimer = 0;

    // AI Goal 指针（用于动态移除）
    CatAvoidPlayerGoal* m_avoidPlayerGoal = nullptr;
    CatTemptGoal* m_temptGoal = nullptr;

    // ========== 躺下/放松动画插值 ==========
    f32 m_lieDownAmount = 0.0f;                ///< 当前躺下动画进度 (0-1)
    f32 m_prevLieDownAmount = 0.0f;            ///< 上一 tick 的躺下动画进度
    f32 m_lieDownAmountTail = 0.0f;            ///< 当前躺下尾巴动画进度 (0-1)
    f32 m_prevLieDownAmountTail = 0.0f;        ///< 上一 tick 的躺下尾巴动画进度
    f32 m_relaxStateOneAmount = 0.0f;          ///< 当前放松状态动画进度 (0-1)
    f32 m_prevRelaxStateOneAmount = 0.0f;      ///< 上一 tick 的放松状态动画进度
    bool m_lyingOnTopOfSleepingPlayer = false; ///< 是否躺在睡眠玩家上方

    // 常量
    static constexpr i32 GIFT_INTERVAL = 24000; // 礼物间隔（1200秒 = 20分钟）

    // 猫的移动速度常量
    static constexpr f64 TEMPT_SPEED = 0.6;       // 诱惑速度
    static constexpr f64 AVOID_FAR_SPEED = 0.8;   // 远距离逃避速度
    static constexpr f64 AVOID_NEAR_SPEED = 1.33; // 近距离逃避速度
    static constexpr f32 AVOID_DISTANCE = 16.0f;  // 逃避检测距离

    // 猫的食物治疗量（生鳕鱼/生鲑鱼的饥饿值为 2，治疗量 = 饥饿值）
    static constexpr f32 FOOD_HEAL_AMOUNT = 2.0f;

    // 躺下/放松动画速率常量
    static constexpr f32 LIE_DOWN_AMOUNT_INCREASE = 0.15f; ///< 躺下动画增长速率
    static constexpr f32 LIE_DOWN_AMOUNT_DECREASE = 0.22f; ///< 躺下动画衰减速率
    static constexpr f32 LIE_DOWN_TAIL_INCREASE = 0.08f;   ///< 尾巴动画增长速率
    static constexpr f32 LIE_DOWN_TAIL_DECREASE = 0.13f;   ///< 尾巴动画衰减速率
    static constexpr f32 RELAX_AMOUNT_INCREASE = 0.1f;     ///< 放松动画增长速率
    static constexpr f32 RELAX_AMOUNT_DECREASE = 0.13f;    ///< 放松动画衰减速率
};

} // namespace mc
