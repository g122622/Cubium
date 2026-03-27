#pragma once

#include "WorldSettings.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include "../../../entity/Player.hpp"  // 用于 PlayerAbilities, FoodStats
#include <memory>
#include <vector>
#include <optional>

namespace mc::world::save::data {

/**
 * @brief 物品槽位数据
 *
 * 用于存储物品栏中的物品信息。
 */
struct SlotData {
    i32 itemId = -1;          ///< 物品 ID（-1 表示空）
    i32 count = 0;            ///< 数量
    i32 damage = 0;           ///< 损坏值/元数据
    std::unique_ptr<nbt::CompoundTag> tag;  ///< 附加数据（附魔、属性等）

    /**
     * @brief 检查槽位是否为空
     */
    [[nodiscard]] bool isEmpty() const { return itemId < 0 || count <= 0; }

    /**
     * @brief 序列化到 NBT
     */
    [[nodiscard]] std::unique_ptr<nbt::CompoundTag> serialize() const;

    /**
     * @brief 从 NBT 反序列化
     */
    [[nodiscard]] static Result<SlotData> deserialize(const nbt::CompoundTag& nbt);
};

/**
 * @brief 属性数据
 *
 * 用于存储实体属性。
 */
struct AttributeData {
    String name;              ///< 属性名称
    f64 baseValue = 0.0;      ///< 基础值
    // 省略修饰符列表，简化实现

    /**
     * @brief 序列化到 NBT
     */
    [[nodiscard]] std::unique_ptr<nbt::CompoundTag> serialize() const;

    /**
     * @brief 从 NBT 反序列化
     */
    [[nodiscard]] static Result<AttributeData> deserialize(const nbt::CompoundTag& nbt);
};

/**
 * @brief 效果数据
 *
 * 用于存储实体效果（药水效果等）。
 */
struct EffectData {
    String id;                ///< 效果 ID
    i32 amplifier = 0;        ///< 放大器（等级-1）
    i32 duration = 0;         ///< 持续时间（ticks）
    bool ambient = false;     ///< 是否为环境效果
    bool showParticles = true; ///< 是否显示粒子

    /**
     * @brief 序列化到 NBT
     */
    [[nodiscard]] std::unique_ptr<nbt::CompoundTag> serialize() const;

    /**
     * @brief 从 NBT 反序列化
     */
    [[nodiscard]] static Result<EffectData> deserialize(const nbt::CompoundTag& nbt);
};

/**
 * @brief 玩家存储数据
 *
 * 对应 playerdata/<uuid>.dat 文件。
 * 参考 MC 1.16.5 PlayerEntity.writeWithoutTypeId()
 *
 * ## 使用示例
 * ```cpp
 * // 从玩家实体创建
 * auto playerData = PlayerData::fromPlayer(player);
 *
 * // 序列化到 NBT
 * auto nbt = playerData.serialize();
 *
 * // 从 NBT 加载
 * auto result = PlayerData::deserialize(*nbt);
 * if (result.success()) {
 *     playerData = std::move(result.value());
 * }
 *
 * // 应用到玩家实体
 * playerData.applyToPlayer(player);
 * ```
 */
class PlayerData {
public:
    // ========== 标识 ==========
    UUID uuid;                        ///< 玩家 UUID
    String username;                  ///< 玩家名

    // ========== 位置 ==========
    DimensionId dimension = DimensionId::Overworld;  ///< 维度
    f64 posX = 0.0;                   ///< X 坐标
    f64 posY = 64.0;                  ///< Y 坐标
    f64 posZ = 0.0;                   ///< Z 坐标
    f32 yaw = 0.0f;                   ///< 偏航角
    f32 pitch = 0.0f;                 ///< 俯仰角

    // ========== 速度 ==========
    f64 motionX = 0.0;                ///< X 方向速度
    f64 motionY = 0.0;                ///< Y 方向速度
    f64 motionZ = 0.0;                ///< Z 方向速度

    // ========== 状态 ==========
    f32 health = 20.0f;               ///< 生命值
    f32 maxHealth = 20.0f;            ///< 最大生命值
    f32 absorptionAmount = 0.0f;      ///< 吸收伤害值（金苹果效果）
    f32 fallDistance = 0.0f;          ///< 摔落距离

    // ========== 食物 ==========
    i32 foodLevel = 20;               ///< 饥饿值 (0-20)
    f32 foodSaturation = 5.0f;        ///< 饱和度
    f32 foodExhaustion = 0.0f;        ///< 消耗累积值
    i32 foodTickTimer = 0;            ///< 食物计时器

    // ========== 经验 ==========
    i32 xpLevel = 0;                  ///< 经验等级
    i32 xpTotal = 0;                  ///< 总经验值
    f32 xpProgress = 0.0f;            ///< 经验条进度 (0.0-1.0)
    i32 xpSeed = 0;                   ///< 经验种子

    // ========== 游戏模式 ==========
    GameType gameType = GameType::Survival;  ///< 游戏模式
    bool canBeKilledInstantly = false;       ///< 是否可以被立即杀死（创造模式）

    // ========== 能力 ==========
    bool invulnerable = false;        ///< 无敌
    bool flying = false;              ///< 正在飞行
    bool canFly = false;              ///< 允许飞行
    bool creativeMode = false;        ///< 创造模式能力
    bool allowEdit = true;            ///< 允许编辑方块
    f32 flySpeed = 0.05f;             ///< 飞行速度
    f32 walkSpeed = 0.1f;             ///< 行走速度

    // ========== 物品栏 ==========
    std::vector<SlotData> inventory;  ///< 物品栏（41个槽位）
    std::vector<SlotData> enderChest; ///< 末影箱（27个槽位）
    i32 selectedSlot = 0;             ///< 选中的快捷栏槽位

    // ========== 属性 ==========
    std::vector<AttributeData> attributes;  ///< 属性列表

    // ========== 效果 ==========
    std::vector<EffectData> effects;  ///< 效果列表

    // ========== 其他 ==========
    bool seenCredits = false;         ///< 是否看过终末之诗
    i32 score = 0;                    ///< 分数
    Optional<BlockPos> lastDeathLocation;  ///< 最后死亡位置

    // ========== 数据版本 ==========
    i32 dataVersion = 2586;           ///< 数据版本

    // ========== 构造函数 ==========

    PlayerData() = default;
    ~PlayerData() = default;

    // 允许移动
    PlayerData(PlayerData&&) = default;
    PlayerData& operator=(PlayerData&&) = default;

    // 禁止拷贝（因为 SlotData 包含 unique_ptr）
    PlayerData(const PlayerData&) = delete;
    PlayerData& operator=(const PlayerData&) = delete;

    // ========== 工厂方法 ==========

    /**
     * @brief 从 Player 实体创建存储数据
     *
     * @param player 玩家实体
     * @return PlayerData 实例
     */
    [[nodiscard]] static std::unique_ptr<PlayerData> fromPlayer(const mc::Player& player);

    /**
     * @brief 应用到 Player 实体
     *
     * @param player 玩家实体
     */
    void applyToPlayer(mc::Player& player) const;

    // ========== 序列化 ==========

    /**
     * @brief 序列化到 NBT
     *
     * @return NBT 复合标签
     */
    [[nodiscard]] std::unique_ptr<nbt::CompoundTag> serialize() const;

    /**
     * @brief 从 NBT 反序列化
     *
     * @param nbt NBT 数据
     * @return 成功返回 PlayerData，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<PlayerData>> deserialize(const nbt::CompoundTag& nbt);

    // ========== 辅助方法 ==========

    /**
     * @brief 生成 UUID 的字符串表示
     *
     * @return UUID 字符串（无连字符）
     */
    [[nodiscard]] String uuidString() const;

private:
    void serializePosition(nbt::CompoundTag& nbt) const;
    void serializeMotion(nbt::CompoundTag& nbt) const;
    void serializeRotation(nbt::CompoundTag& nbt) const;
    void serializeFood(nbt::CompoundTag& nbt) const;
    void serializeExperience(nbt::CompoundTag& nbt) const;
    void serializeAbilities(nbt::CompoundTag& nbt) const;
    void serializeInventory(nbt::CompoundTag& nbt) const;
    void serializeAttributes(nbt::CompoundTag& nbt) const;
    void serializeEffects(nbt::CompoundTag& nbt) const;

    static void deserializePosition(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeMotion(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeRotation(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeFood(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeExperience(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeAbilities(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeInventory(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeAttributes(PlayerData& data, const nbt::CompoundTag& nbt);
    static void deserializeEffects(PlayerData& data, const nbt::CompoundTag& nbt);
};

} // namespace mc::world::save::data
