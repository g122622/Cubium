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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/component/CustomComponentParameters.hpp"
#include <functional>
#include <optional>
#include <string>

namespace mc::world::block {
class BlockState;
}

namespace mc {
class IWorld;
}

namespace mc::entity {
class Entity;
}

namespace mc::entity::player {
class Player;
}

namespace mc::util {
class Direction;
}

namespace mc::mod::bedrock::addon {

// 前向声明 - 脚本包装类型（后续在modules/中定义）
class ScriptBlock;
class ScriptDimension;
class ScriptEntity;
class ScriptPlayer;
class ScriptBlockPermutation;
class ScriptVec3;

/**
 * @brief 方块组件事件基类
 *
 * 所有方块自定义组件事件共享block和dimension信息。
 */
struct BlockComponentEvent {
    /** 事件发生位置的方块类型ID（如"minecraft:stone"） */
    std::string blockTypeId;

    /** 事件发生位置 */
    i32 blockX = 0;
    i32 blockY = 0;
    i32 blockZ = 0;

    /** 方块所在维度ID */
    i32 dimensionId = 0;
};

/**
 * @brief 实体踩上方块事件
 *
 * 当实体站在方块上时触发。对应Bedrock API的onStepOn。
 */
struct BlockComponentStepOnEvent : BlockComponentEvent {
    /** 踩上方块的实体ID（可能为空） */
    std::optional<u64> entityId;
};

/**
 * @brief 实体离开方块事件
 *
 * 当实体离开方块时触发。对应Bedrock API的onStepOff。
 */
struct BlockComponentStepOffEvent : BlockComponentEvent {
    /** 离开方块的实体ID（可能为空） */
    std::optional<u64> entityId;
};

/**
 * @brief 方块被放置事件
 *
 * 当方块被放置到世界中时触发。对应Bedrock API的onPlace。
 */
struct BlockComponentOnPlaceEvent : BlockComponentEvent {
    /** 放置前的方块排列（被替换的方块） */
    std::string previousBlockTypeId;
};

/**
 * @brief 方块被破坏事件
 *
 * 当方块被任何方式破坏时触发。对应Bedrock API的onBreak。
 */
struct BlockComponentBreakEvent : BlockComponentEvent {
    /** 被破坏的方块排列类型ID */
    std::string brokenBlockPermutationTypeId;

    /** 破坏来源实体ID（可能为空） */
    std::optional<u64> entitySourceId;
};

/**
 * @brief 玩家破坏方块事件
 *
 * 当玩家破坏方块时触发。对应Bedrock API的onPlayerBreak。
 */
struct BlockComponentPlayerBreakEvent : BlockComponentEvent {
    /** 破坏方块的玩家ID */
    std::optional<u64> playerId;

    /** 被破坏的方块排列类型ID */
    std::string brokenBlockPermutationTypeId;
};

/**
 * @brief 玩家与方块交互事件
 *
 * 当玩家右键点击方块时触发。对应Bedrock API的onPlayerInteract。
 */
struct BlockComponentPlayerInteractEvent : BlockComponentEvent {
    /** 交互的玩家ID */
    std::optional<u64> playerId;

    /** 交互面方向（0=Down, 1=Up, 2=North, 3=South, 4=West, 5=East） */
    i32 face = 0;

    /** 交互面位置X */
    f32 faceX = 0.0f;
    f32 faceY = 0.0f;
    f32 faceZ = 0.0f;
};

/**
 * @brief 玩家放置方块前事件（可取消）
 *
 * 在玩家放置方块之前触发，脚本可以取消放置。
 * 对应Bedrock API的beforeOnPlayerPlace。
 */
struct BlockComponentPlayerPlaceBeforeEvent : BlockComponentEvent {
    /** 是否取消放置 */
    bool cancel = false;

    /** 放置方块的玩家ID */
    std::optional<u64> playerId;

    /** 要放置的方块排列类型ID */
    std::string permutationToPlaceTypeId;

    /** 放置面方向 */
    i32 face = 0;
};

/**
 * @brief 实体落在方块上事件
 *
 * 当实体掉落到方块上时触发。对应Bedrock API的onEntityFallOn。
 */
struct BlockComponentEntityFallOnEvent : BlockComponentEvent {
    /** 落下的实体ID */
    std::optional<u64> entityId;

    /** 掉落距离（格） */
    f32 fallDistance = 0.0f;
};

/**
 * @brief 方块随机刻事件
 *
 * 当方块收到随机刻时触发。对应Bedrock API的onRandomTick。
 */
struct BlockComponentRandomTickEvent : BlockComponentEvent {};

/**
 * @brief 方块刻事件
 *
 * 当方块收到计划刻时触发。对应Bedrock API的onTick。
 */
struct BlockComponentTickEvent : BlockComponentEvent {};

/**
 * @brief 红石更新事件
 *
 * 当方块收到红石信号更新时触发。对应Bedrock API的onRedstoneUpdate。
 */
struct BlockComponentRedstoneUpdateEvent : BlockComponentEvent {
    /** 当前红石信号强度 */
    i32 powerLevel = 0;

    /** 上一次红石信号强度 */
    i32 previousPowerLevel = 0;

    /** 是否为首次更新 */
    bool firstUpdate = false;
};

/**
 * @brief 实体事件（实体触发方块事件）
 *
 * 当实体通过/data等命令向方块发送事件时触发。
 * 对应Bedrock API的onEntity。
 */
struct BlockComponentEntityEvent : BlockComponentEvent {
    /** 触发事件的实体ID */
    u64 entitySourceId = 0;

    /** 事件名称 */
    std::string name;

    /** 方块排列类型ID */
    std::string blockPermutationTypeId;
};

/**
 * @brief 方块状态变化事件
 *
 * 当方块状态（属性）发生变化时触发。
 * 对应Bedrock API的onBlockStateChange。
 */
struct BlockComponentBlockStateChangeEvent : BlockComponentEvent {
    /** 变化前的方块排列类型ID */
    std::string previousPermutationTypeId;

    /** 变化后的方块排列类型ID */
    std::string newPermutationTypeId;
};

} // namespace mc::mod::bedrock::addon
