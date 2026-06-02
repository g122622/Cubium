#pragma once

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/component/CustomComponentParameters.hpp"
#include <string>
#include <optional>
#include <functional>

namespace mc::mod::bedrock::addon {

/**
 * @brief 物品组件事件基类
 *
 * 所有物品自定义组件事件共享itemTypeId和source信息。
 */
struct ItemComponentEvent {
    /** 物品类型ID（如"minecraft:diamond_sword"） */
    std::string itemTypeId;

    /** 使用物品的实体ID */
    u64 sourceEntityId = 0;
};

/**
 * @brief 物品使用事件
 *
 * 当玩家在空中右键使用物品时触发。对应Bedrock API的onUse。
 */
struct ItemComponentUseEvent : ItemComponentEvent {
    /** 使用物品的实体ID */
    u64 sourceId = 0;

    /** 物品栈数量 */
    i32 itemStackAmount = 1;
};

/**
 * @brief 物品对方块使用事件
 *
 * 当玩家右键点击方块时使用物品触发。对应Bedrock API的onUseOn。
 */
struct ItemComponentUseOnEvent : ItemComponentEvent {
    /** 使用物品的实体ID */
    u64 sourceId = 0;

    /** 目标方块位置 */
    i32 blockX = 0;
    i32 blockY = 0;
    i32 blockZ = 0;

    /** 目标方块类型ID */
    std::string usedOnBlockTypeId;

    /** 目标方块排列类型ID */
    std::string usedOnBlockPermutationTypeId;

    /** 点击面方向 */
    i32 face = 0;

    /** 点击面位置 */
    f32 faceX = 0.0f;
    f32 faceY = 0.0f;
    f32 faceZ = 0.0f;
};

/**
 * @brief 物品击中实体事件
 *
 * 当物品攻击实体时触发。对应Bedrock API的onHitEntity。
 */
struct ItemComponentHitEntityEvent : ItemComponentEvent {
    /** 攻击者实体ID */
    u64 attackingEntityId = 0;

    /** 被击中的实体ID */
    u64 hitEntityId = 0;

    /** 物品栈数量 */
    i32 itemStackAmount = 1;

    /** 攻击是否有效果 */
    bool hadEffect = false;
};

/**
 * @brief 物品耐久伤害前事件（可修改伤害值）
 *
 * 在耐久伤害应用之前触发，脚本可以修改伤害值。
 * 对应Bedrock API的onBeforeDurabilityDamage。
 */
struct ItemComponentBeforeDurabilityDamageEvent : ItemComponentEvent {
    /** 攻击者实体ID */
    u64 attackingEntityId = 0;

    /** 被击中的实体ID */
    u64 hitEntityId = 0;

    /** 物品栈数量 */
    i32 itemStackAmount = 1;

    /** 耐久伤害值（可修改） */
    i32 durabilityDamage = 1;
};

/**
 * @brief 物品挖掘方块事件
 *
 * 当物品被用于挖掘方块时触发。对应Bedrock API的onMineBlock。
 */
struct ItemComponentMineBlockEvent : ItemComponentEvent {
    /** 挖掘者实体ID */
    u64 sourceId = 0;

    /** 被挖掘方块位置 */
    i32 blockX = 0;
    i32 blockY = 0;
    i32 blockZ = 0;

    /** 被挖掘方块类型ID */
    std::string blockTypeId;

    /** 被挖掘方块排列类型ID */
    std::string minedBlockPermutationTypeId;

    /** 物品栈数量 */
    i32 itemStackAmount = 1;
};

/**
 * @brief 物品使用完成事件
 *
 * 当物品使用动画完成后触发（如蓄力完成）。
 * 对应Bedrock API的onCompleteUse。
 */
struct ItemComponentCompleteUseEvent : ItemComponentEvent {
    /** 使用物品的实体ID */
    u64 sourceId = 0;

    /** 使用时长（tick） */
    i32 useDuration = 0;

    /** 物品栈数量 */
    i32 itemStackAmount = 1;
};

/**
 * @brief 物品消耗事件
 *
 * 当消耗型物品（食物、药水）被消耗时触发。
 * 对应Bedrock API的onConsume。
 */
struct ItemComponentConsumeEvent : ItemComponentEvent {
    /** 消耗物品的实体ID */
    u64 sourceId = 0;

    /** 物品栈数量 */
    i32 itemStackAmount = 1;
};

} // namespace mc::mod::bedrock::addon
