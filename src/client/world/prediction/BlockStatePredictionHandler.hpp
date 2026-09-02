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
#include "common/world/block/BlockPos.hpp"
#include <unordered_map>

namespace mc {
class BlockState;
} // namespace mc

namespace mc::client {

class ClientWorld;

/**
 * @brief 客户端方块状态预测处理器
 *
 * 对齐 Java 1.21.11 net.minecraft.client.multiplayer.prediction.BlockStatePredictionHandler。
 *
 * 机制：客户端在发送带 sequence 的方块交互包（use_item_on / use_item / PlayerAction
 * Start/StopDestroy）时，本地预测性地修改方块状态（如挖掘时本地清方块），并记录该位置
 * 在预测前的"服务端权威状态"。收到服务端 ClientboundBlockChangedAckPacket(sequence) 后，
 * 对所有 sequence <= ackSequence 的预测位置调 syncBlockState：若当前方块状态与服务端权威
 * 状态不符（说明服务端拒绝了预测），则回滚到服务端权威状态。
 *
 * 这套"预测-ACK-回滚"机制是 Minecraft 反作弊与延迟补偿的核心：客户端在服务端确认前本地
 * 应用方块变更以降低感知延迟，服务端确认后若预测错误则回滚。
 *
 * 序列号语义：每次 startPredicting() 自增 currentSequenceNr，作为出站包的 sequence 字段。
 * 服务端取 max 累积、每 tick 末批量回一个 maxSequence 的 ACK。客户端收到 ACK 后确认所有
 * <= maxSequence 的预测。
 */
class BlockStatePredictionHandler {
public:
    BlockStatePredictionHandler() = default;

    /**
     * @brief 开始一次预测性方块操作
     *
     * 自增序列号并标记 isPredicting=true。在 isPredicting 期间通过 setBlockState 写入的
     * 方块状态会被 retainKnownServerState 记录预测前的服务端权威状态。
     *
     * @return 本次预测的序列号（用于构造出站包的 sequence 字段）
     */
    i32 startPredicting()
    {
        ++m_currentSequenceNr;
        m_isPredicting = true;
        return m_currentSequenceNr;
    }

    /**
     * @brief 结束本次预测性操作
     *
     * 配合 startPredicting 使用，复位 isPredicting 标志。
     */
    void stopPredicting() { m_isPredicting = false; }

    /**
     * @brief 是否正在预测性操作中
     */
    [[nodiscard]] bool isPredicting() const { return m_isPredicting; }

    /**
     * @brief 当前预测序列号
     */
    [[nodiscard]] i32 currentSequence() const { return m_currentSequenceNr; }

    /**
     * @brief 记录某位置预测前的服务端权威状态
     *
     * 在预测性 setBlock 写入新状态**之前**调用。若该位置已有记录，仅更新 sequence
     * （保留最早的 oldState，对齐原版 retainKnownServerState 语义）。
     *
     * @param pos 方块位置
     * @param oldState 预测前的服务端权威方块状态
     */
    void retainKnownServerState(const BlockPos& pos, const BlockState* oldState);

    /**
     * @brief 服务端权威 BlockUpdate 到达时更新已知服务端状态
     *
     * 对齐原版 setServerVerifiedBlockState 内的 updateKnownServerState：若该位置有预测
     * 记录，更新其 oldState 为新的服务端权威状态（服务端已覆盖预测），返回 true 表示
     * 预测已被服务端处理，调用方应跳过立即写入（待 ACK 时 syncBlockState 统一处理）；
     * 返回 false 表示无预测记录，调用方应直接写入。
     *
     * @param pos 方块位置
     * @param serverState 服务端权威方块状态
     * @return true 表示该位置有预测记录（已被服务端覆盖），false 表示无预测记录
     */
    bool updateKnownServerState(const BlockPos& pos, const BlockState* serverState);

    /**
     * @brief 收到 ACK 后确认并回滚所有 sequence <= ackSequence 的预测
     *
     * 对齐原版 endPredictionsUpTo：遍历所有记录，对 sequence <= ackSequence 的位置调
     * ClientWorld::syncBlockState 回滚（若当前状态与服务端权威不符），然后移除该记录。
     *
     * @param ackSequence 服务端确认的序列号
     * @param world 客户端世界（用于 syncBlockState 回滚）
     */
    void endPredictionsUpTo(i32 ackSequence, ClientWorld& world);

    /**
     * @brief 重置所有预测状态（断线重连时调用）
     */
    void reset();

private:
    /**
     * @brief 服务端权威状态记录
     *
     * 对齐原版 ServerVerifiedState：记录某位置预测前的服务端权威方块状态。
     * 注意：blockState 是裸指针，指向 BlockRegistry 中的全局方块状态对象，
     * 生命周期与进程一致，无需管理所有权。
     */
    struct ServerVerifiedState {
        i32 sequence = -1;
        const BlockState* blockState = nullptr;
    };

    std::unordered_map<i64, ServerVerifiedState> m_serverVerifiedStates; // key = BlockPos.asLong
    i32 m_currentSequenceNr = -1;                                        // 对齐原版初值，首次 startPredicting 后变 0
    bool m_isPredicting = false;
};

/**
 * @brief RAII 预测守卫
 *
 * 对齐原版 try-with-resources startPredicting() 模式。构造时开始预测，析构时结束。
 * 用法：
 *   i32 seq;
 *   {
 *       BlockPredictionGuard guard(handler);
 *       seq = guard.sequence();
 *       // 在此作用域内执行本地预测写方块 + 构造出站包（用 seq）
 *   }
 */
class BlockPredictionGuard {
public:
    explicit BlockPredictionGuard(BlockStatePredictionHandler& handler)
        : m_handler(handler)
        , m_sequence(handler.startPredicting())
    {}

    ~BlockPredictionGuard() { m_handler.stopPredicting(); }

    BlockPredictionGuard(const BlockPredictionGuard&) = delete;
    BlockPredictionGuard& operator=(const BlockPredictionGuard&) = delete;

    [[nodiscard]] i32 sequence() const { return m_sequence; }

private:
    BlockStatePredictionHandler& m_handler;
    i32 m_sequence;
};

} // namespace mc::client
