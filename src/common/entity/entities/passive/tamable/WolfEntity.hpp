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

#include "TameableEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/passive/tamable/Crackiness.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 狼实体
 *
 * 可驯服的动物，驯服后成为狗。
 *
 * 特性：
 * - 驯服：用骨头驯服
 * - 愤怒：被攻击后反击
 * - 跟随主人：驯服后跟随
 * - 坐下/站起：右键切换
 * - 攻击目标：保护主人
 * - 尾巴角度：表示生命值和心情
 * - 颈圈颜色：驯服后可染色
 * - 狼铠：驯服后可装备狼铠，提供伤害吸收
 *
 * 狼铠交互逻辑（MC 1.21.11）：
 * - 装备：主人右键狼 + 手持狼铠 + 未装备 + 非幼年 → 装备狼铠
 * - 修复：主人右键坐下的狼 + 犰狳鳞甲 + 狼铠已受损 → 修复 12.5% 耐久
 * - 染色：主人右键狼 + 手持染料 + 已装备可染色狼铠 → 改变狼铠颜色
 * - 剪切：主人右键狼 + 手持剪刀 + 已装备狼铠 → 剪下狼铠
 * - 伤害吸收：穿戴狼铠时，非绕过护甲的伤害由狼铠吸收，狼不受伤
 * - 裂纹：狼铠受损到不同阈值时播放裂纹音效和粒子
 */
class WolfEntity : public TameableEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    WolfEntity(EntityId id);
    ~WolfEntity() override = default;

    // 禁止拷贝
    WolfEntity(const WolfEntity&) = delete;
    WolfEntity& operator=(const WolfEntity&) = delete;

    // 允许移动
    WolfEntity(WolfEntity&&) = delete;
    WolfEntity& operator=(WolfEntity&&) = delete;

    /**
     * @brief 创建狼实体
     * @param world 世界实例
     * @return 新的狼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 交互 ==========

    /**
     * @brief 玩家与狼交互
     *
     * 交互逻辑（按优先级）：
     * 1. 未驯服 + 骨头 → 尝试驯服（1/3概率）
     * 2. 已驯服 + 食物 + 未满血 → 喂食治疗
     * 3. 已驯服 + 狼铠 + 可装备 + 主人 + 非幼年 → 装备狼铠
     * 4. 已驯服 + 犰狳鳞甲 + 坐下 + 狼铠受损 + 主人 → 修复狼铠
     * 5. 已驯服 + 染料 + 已装备狼铠 + 主人 → 染色狼铠
     * 6. 已驯服 + 染料 + 颈圈 → 改变颈圈颜色
     * 7. 已驯服 + 调用父类（处理繁殖/成长）
     * 8. 已驯服 + 主人 + 父类未处理 → 切换坐下/站起
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 如果是骨头返回true
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于繁殖
     * @param itemStack 物品堆
     * @return 如果是肉类返回true
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于治疗
     * @param itemStack 物品堆
     * @return 如果是肉类返回true
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    /**
     * @brief 判断此狼是否想要攻击指定目标
     *
     * 狼不会攻击以下目标：
     * - 苦力怕、恶魂、盔甲架
     * - 已驯服的其他驯服动物（包括已驯服的马）
     * - 与自己同主人的其他狼
     * - 主人不能伤害的玩家（PvP保护）
     *
     * @param target 待攻击的目标实体
     * @param owner 此狼的主人（可能为 nullptr）
     * @return true 如果此狼愿意攻击该目标
     */
    [[nodiscard]] bool wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const override;

    // ========== 狼铠 ==========

    /**
     * @brief 检查玩家是否可以剪切实体装备
     *
     * 狼只允许主人剪切狼铠。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.canShearEquipment()
     *
     * @param player 尝试剪切的玩家
     * @return 如果是主人返回 true
     */
    [[nodiscard]] bool canShearEquipment(const Player& player) const override;

    /**
     * @brief 实际受伤处理（狼铠伤害吸收）
     *
     * 当狼穿戴狼铠且伤害源不绕过护甲时，伤害由狼铠吸收（狼不扣血）。
     * 狼铠受损时检查裂纹等级变化，等级变化时播放裂纹音效和粒子。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.actuallyHurt()
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    void actuallyHurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 盔甲受损回调
     *
     * 狼穿戴狼铠时，将伤害传递给狼铠。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.hurtArmor()
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    void damageArmor(DamageSource& source, f32 amount) override;

    /**
     * @brief 获取受伤声音
     *
     * 穿戴狼铠且伤害由狼铠吸收时，播放狼铠受伤音效而非普通狼受伤音效。
     *
     * @param source 伤害来源
     * @return 声音事件资源位置
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    // ========== 繁殖 ==========

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 刻更新
     *
     * 实现 MC 1.21.11 Wolf.tick() 的甩水状态机：
     * 1. interestedAngle 插值（向 1.0 或 0.0 趋近）
     * 2. isWet 标记（在水中/雨中时为 true）
     * 3. 甩水动画进度（shakeAnim 每 tick +0.05，shakeAnimO >= 2.0 时结束）
     * 4. SPLASH 粒子发射（shakeAnim > 0.4 时）
     *
     * 甩水动画触发（对应 MC Wolf.aiStep()）在 TameableEntity::tick() 内部
     * 调用 LivingEntity::tick() → aiStep() 时已执行，本方法处理触发后的进度。
     */
    void tick() override;

    // ========== 死亡 ==========

    /**
     * @brief 死亡回调
     *
     * 重置甩水状态（isWet, isShaking, shakeAnim, shakeAnimO），
     * 防止死亡过程中残留甩水动画。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.die()
     */
    void die(DamageSource& cause) override;

    // ========== NBT 序列化 ==========

    /**
     * @brief 保存颈圈颜色到 NBT
     *
     * 对应 MC 1.21.11 Wolf.addAdditionalSaveData 中的 "CollarColor" 字段。
     * 颈圈颜色存储为 i32（DyeColor 的序数值）。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 从 NBT 读取颈圈颜色
     *
     * 对应 MC 1.21.11 Wolf.readAdditionalSaveData 中的 "CollarColor" 字段。
     * 如果 NBT 中没有该字段，保持默认值（红色）。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 愤怒系统 ==========

    /**
     * @brief 获取尾巴角度
     * @return 尾巴角度（弧度）
     *
     * 尾巴角度基于生命值：
     * - 满血时约40度
     * - 低血量时约-10度
     * - 愤怒时约88度（1.539f）
     */
    [[nodiscard]] f32 getTailAngle() const;

    /**
     * @brief 检查是否处于愤怒状态
     *
     * 重写 TameableEntity::isAngry()，从 DATA_ANGER_TIME_PARAM 读取而非成员变量。
     * 服务端调用 setAngry/setAngerTime 修改后，会自动通过元数据同步到客户端，
     * 客户端 ClientEntity::wolfIsAngry 镜像此状态用于渲染（尾巴角度、纹理选择等）。
     *
     * @return 如果愤怒时间 > 0 返回 true
     */
    [[nodiscard]] bool isAngry() const override { return getAngerTime() > 0; }

    /**
     * @brief 获取愤怒时间（ticks）
     *
     * 通过 DataParameter 从 EntityDataManager 读取。每 tick 由 updateAnger() 递减。
     *
     * @return 剩余愤怒时间（ticks）
     */
    [[nodiscard]] i32 getAngerTime() const override { return m_dataManager.get<i32>(DATA_ANGER_TIME_PARAM); }

    /**
     * @brief 设置愤怒时间（ticks）
     *
     * 写入 DataParameter，会标记为脏数据并由 EntityTracker 在下一 tick 广播
     * EntityMetadataPacket 到所有观察者客户端。
     *
     * @param time 愤怒时间（ticks）
     */
    void setAngerTime(i32 time) override { m_dataManager.set(DATA_ANGER_TIME_PARAM, time); }

    /**
     * @brief 设置愤怒状态
     *
     * 等价于 setAngerTime(MAX_ANGER_TIME) 或 setAngerTime(0) 并清除攻击目标。
     * 重写 TameableEntity::setAngry() 是为了让狼的愤怒状态写入 DataParameter
     * 而非 TameableEntity 的 m_angerTime 成员变量，从而实现服务端→客户端同步。
     *
     * @param angry 是否愤怒
     */
    void setAngry(bool angry) override;

    /**
     * @brief 获取愤怒时间数据参数 ID
     *
     * 用于客户端从元数据中读取愤怒时间（ClientEntity::syncMetadataFromDataManager）。
     */
    [[nodiscard]] static u16 getAngerTimeParamId() { return DATA_ANGER_TIME_PARAM.id(); }

    // ========== 兴趣状态 ==========

    /**
     * @brief 检查是否感兴趣（乞求食物）
     *
     * 通过 DataParameter 从 EntityDataManager 读取，服务端调用 setInterested 修改后
     * 会自动通过元数据同步到客户端。
     *
     * @return 如果正在乞求食物返回true
     */
    [[nodiscard]] bool isInterested() const { return m_dataManager.get<bool>(DATA_INTERESTED_PARAM); }

    /**
     * @brief 设置感兴趣状态
     *
     * 写入 DataParameter，会标记为脏数据并由 EntityTracker 在下一 tick 广播
     * EntityMetadataPacket 到所有观察者客户端。
     *
     * @param interested 是否感兴趣
     */
    void setInterested(bool interested) { m_dataManager.set(DATA_INTERESTED_PARAM, interested); }

    /**
     * @brief 获取兴趣状态数据参数 ID
     *
     * 用于客户端从元数据中读取兴趣状态（ClientEntity::syncMetadataFromDataManager）。
     */
    [[nodiscard]] static u16 getInterestedParamId() { return DATA_INTERESTED_PARAM.id(); }

    // ========== 颈圈颜色 ==========

    /**
     * @brief 获取颈圈颜色
     *
     * 通过 DataParameter 从 EntityDataManager 读取，服务端调用 setCollarColor 修改后
     * 会自动通过元数据同步到客户端。客户端 ClientEntity::wolfCollarColor 镜像此状态。
     *
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getCollarColor() const
    {
        return static_cast<DyeColor>(m_dataManager.get<i32>(DATA_COLLAR_COLOR_PARAM));
    }

    /**
     * @brief 设置颈圈颜色
     *
     * 写入 DataParameter，会标记为脏数据并由 EntityTracker 在下一 tick 广播
     * EntityMetadataPacket 到所有观察者客户端。
     *
     * @param color 染料颜色
     */
    void setCollarColor(DyeColor color) { m_dataManager.set(DATA_COLLAR_COLOR_PARAM, static_cast<i32>(color)); }

    /**
     * @brief 获取颈圈颜色数据参数 ID
     *
     * 用于客户端从元数据中读取颈圈颜色（ClientEntity::syncMetadataFromDataManager）。
     */
    [[nodiscard]] static u16 getCollarColorParamId() { return DATA_COLLAR_COLOR_PARAM.id(); }

    // ========== 水域行为 ==========

    /**
     * @brief 检查是否在水中
     * 狼在水中会减速
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 检查是否在水中或雨中
     *
     * 用于驱动甩水状态机：狼接触水或雨时 isWet 设为 true。
     * 对应 MC Entity.isInWaterOrRain()。
     */
    [[nodiscard]] virtual bool isInWaterOrRain() const;

    // ========== 甩水动画状态 ==========

    /**
     * @brief 检查是否处于湿润状态
     *
     * isWet 在狼接触水/雨时设为 true，在甩水动画完成后设为 false。
     * 与 isInWater() 不同：isInWater() 表示当前帧是否在水中，
     * isWet 表示自上次甩水以来是否曾接触水。
     *
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.isWet
     */
    [[nodiscard]] bool isWet() const { return m_isWet; }

    /**
     * @brief 检查是否正在甩水
     * @return 如果甩水动画正在进行返回 true
     */
    [[nodiscard]] bool isShaking() const { return m_isShaking; }

    /**
     * @brief 获取甩水动画进度（插值）
     * @param partialTick 部分 tick（0.0-1.0）
     * @return 插值后的甩水动画进度（0.0-2.0）
     *
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.getShakeAnim()
     */
    [[nodiscard]] f32 getShakeAnim(f32 partialTick) const;

    /**
     * @brief 获取湿润着色值（用于渲染变暗）
     * @param partialTick 部分 tick（0.0-1.0）
     * @return 着色值（0.75-1.0），1.0 表示完全干燥
     *
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.getWetShade()
     */
    [[nodiscard]] f32 getWetShade(f32 partialTick) const;

    /**
     * @brief 获取头部转动角度（乞求食物时的头部倾斜）
     * @param partialTick 部分 tick（0.0-1.0）
     * @return 头部 Z 轴旋转角度（弧度）
     *
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.getHeadRollAngle()
     */
    [[nodiscard]] f32 getHeadRollAngle(f32 partialTick) const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.4f : 0.8f; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据同步 ==========
    /**
     * @brief 注册同步数据参数
     *
     * 注册 DATA_INTERESTED_PARAM（兴趣状态）、DATA_COLLAR_COLOR_PARAM（颈圈颜色）、
     * DATA_ANGER_TIME_PARAM（愤怒时间）到 EntityDataManager。
     * 客户端通过对应 get*ParamId() 读取参数 ID 并在元数据同步时
     * 调用 setWolfIsInterested/setWolfCollarColor/setWolfIsAngry 更新客户端镜像状态。
     *
     * 注意：由于 C++ 虚函数在基类构造函数中不会派发到派生类，
     * WolfEntity 构造函数必须显式调用此方法（参考 ZombieVillagerEntity 模式）。
     */
    void registerData() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.6f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.85f; }

    // ========== 驯服回调 ==========
    void onTamed(bool tamed) override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取声音音量
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 0.4f; }

    /**
     * @brief 播放脚步声音
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;
    void playStepSound();

    // ========== 甩水动画状态机（参考 MC 1.21.11 Wolf） ==========

    /**
     * @brief 播放甩水音效
     *
     * 在甩水动画开始时调用（m_shakeAnim == 0.0f 时）。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.playShakingSound()
     */
    void playShakingSound();

    /**
     * @brief 取消甩水动画
     *
     * 重置 isShaking、shakeAnim、shakeAnimO。
     * 当狼在甩水过程中再次接触水时调用。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.cancelShake()
     */
    void _cancelShake();

protected:
    // ========== 测试用辅助方法（仅供测试子类访问甩水状态） ==========
    // 这些方法暴露私有状态字段，用于单元测试中直接设置甩水状态。
    // 不应在生产代码中调用。

    /// 设置 isWet 标志（测试用）
    void _setWetForTest(bool value) { m_isWet = value; }

    /// 设置 isShaking 标志（测试用）
    void _setShakingForTest(bool value) { m_isShaking = value; }

    /// 设置 shakeAnim 和 shakeAnimO（测试用）
    void _setShakeAnimForTest(f32 anim, f32 animO)
    {
        m_shakeAnim = anim;
        m_shakeAnimO = animO;
    }

private:
    // ========== 数据同步 ==========
    /**
     * @brief 兴趣状态同步参数
     *
     * 对应 MC 1.21.11 Wolf.DATA_INTERESTED_ID。
     * 由 BegGoal 在 startExecuting/resetTask 时通过 setInterested 写入，
     * 由 EntityTracker 自动广播到所有观察者客户端。
     */
    static entity::DataParameter<bool> DATA_INTERESTED_PARAM;

    /**
     * @brief 颈圈颜色同步参数
     *
     * 对应 MC 1.21.11 Wolf.DATA_COLLAR_COLOR。
     * 存储 DyeColor 的 i32 值（0-15），由 setCollarColor 写入，
     * 由 EntityTracker 自动广播到所有观察者客户端。
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取此参数并调用 setWolfCollarColor。
     */
    static entity::DataParameter<i32> DATA_COLLAR_COLOR_PARAM;

    /**
     * @brief 愤怒时间同步参数
     *
     * 对应 MC 1.21.11 Wolf.DATA_ANGER_END_TIME（语义上：剩余愤怒 ticks）。
     * 由 setAngerTime/setAngry 写入，由 updateAnger() 每 tick 递减，
     * 由 EntityTracker 自动广播到所有观察者客户端。
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取此参数并调用 setWolfIsAngry。
     *
     * 注意：MC 1.21.11 原版使用 Long 类型的「愤怒结束时刻」(gameTime 戳)，
     * 此处采用更直观的「剩余 ticks」i32 表示，与 BeeEntity::ANGER_TIME_PARAM 风格一致。
     */
    static entity::DataParameter<i32> DATA_ANGER_TIME_PARAM;

    // 常量
    static constexpr f32 TAIL_ANGLE_HEALTHY = 0.698f;    // 健康时尾巴角度（弧度）
    static constexpr f32 TAIL_ANGLE_UNHEALTHY = -0.175f; // 不健康时尾巴角度（弧度）
    static constexpr f32 ARMOR_REPAIR_UNIT = 0.125F;     // 狼铠修复单位（12.5%最大耐久）

    // 声音状态
    f32 m_stepSoundDistance = 0.0f;
    f32 m_nextStepSoundDistance = 1.0f;

    // 甩水动画状态（参考 MC 1.21.11 Wolf.tick() / Wolf.aiStep()）
    bool m_isWet = false;          ///< 自上次甩水以来是否曾接触水
    bool m_isShaking = false;      ///< 是否正在甩水
    f32 m_shakeAnim = 0.0f;        ///< 甩水动画进度（每 tick +0.05，达到 2.0 时结束）
    f32 m_shakeAnimO = 0.0f;       ///< 上一 tick 的甩水进度（用于插值）
    f32 m_interestedAngle = 0.0f;  ///< 乞求食物头部角度（向 1.0 或 0.0 插值）
    f32 m_interestedAngleO = 0.0f; ///< 上一 tick 的乞求角度（用于插值）

    // ========== 私有辅助方法 ==========

    /**
     * @brief 尝试驯服狼
     * @param player 尝试驯服的玩家
     *
     * 1/3 概率驯服成功，成功时设置驯服状态和主人，
     * 广播 TamingSucceeded/TamingFailed 粒子效果。
     */
    void _tryToTame(Player& player);

    /**
     * @brief 获取狼食物的治疗量
     * @param item 食物物品
     * @return 治疗量（生命值），非食物返回 0
     *
     * MC 原版中治疗量为 2.0 * food.nutrition，
     * 此处使用硬编码映射近似 MC 原版的营养值。
     */
    [[nodiscard]] f32 _getFoodHealAmount(const Item* item) const;

    /**
     * @brief 获取染料物品对应的颜色
     * @param item 物品指针
     * @return 对应的 DyeColor，如果不是染料返回 std::nullopt
     */
    [[nodiscard]] static std::optional<DyeColor> _getDyeColorFromItem(const Item* item);

    /**
     * @brief 检查狼铠是否可以吸收此伤害
     *
     * 条件：穿戴狼铠 且 伤害源不绕过护甲
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.canArmorAbsorb()
     *
     * @param source 伤害来源
     * @return 如果狼铠可以吸收此伤害返回 true
     */
    [[nodiscard]] bool _canArmorAbsorb(const DamageSource& source) const;
};

} // namespace mc
