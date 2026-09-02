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

#include "BlockStatePredictionHandler.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::client {

void BlockStatePredictionHandler::retainKnownServerState(
    const BlockPos& pos, const BlockState* oldState, const Vector3& playerPos)
{
    const i64 key = pos.asLong();
    auto it = m_serverVerifiedStates.find(key);
    if (it == m_serverVerifiedStates.end()) {
        // 新记录：保存预测前的服务端权威状态 + 当前 sequence + 玩家位置
        m_serverVerifiedStates.emplace(key, ServerVerifiedState{m_currentSequenceNr, oldState, playerPos});
    } else {
        // 已有记录：仅更新 sequence 与玩家位置，保留最早的 oldState（对齐原版语义）
        it->second.sequence = m_currentSequenceNr;
        it->second.playerPos = playerPos;
    }
}

bool BlockStatePredictionHandler::updateKnownServerState(const BlockPos& pos, const BlockState* serverState)
{
    auto it = m_serverVerifiedStates.find(pos.asLong());
    if (it == m_serverVerifiedStates.end()) {
        return false; // 无预测记录，调用方应直接写入
    }
    // 该位置有预测记录：服务端权威状态已到达，更新 oldState 为新的服务端状态。
    // 调用方应跳过立即写入（待 ACK 时 syncBlockState 统一处理）。
    it->second.blockState = serverState;
    return true;
}

void BlockStatePredictionHandler::endPredictionsUpTo(i32 ackSequence, ClientWorld& world)
{
    // 对齐原版 endPredictionsUpTo：遍历所有记录，确认 sequence <= ackSequence 的预测。
    // 注意：遍历过程中可能 erase，需先收集待确认 key 再处理。
    std::vector<i64> confirmedKeys;
    confirmedKeys.reserve(m_serverVerifiedStates.size());
    for (const auto& [key, state] : m_serverVerifiedStates) {
        if (state.sequence <= ackSequence) {
            confirmedKeys.push_back(key);
        }
    }

    for (i64 key : confirmedKeys) {
        auto it = m_serverVerifiedStates.find(key);
        if (it == m_serverVerifiedStates.end()) {
            continue;
        }
        const BlockPos pos = BlockPos::fromLong(key);
        // syncBlockState：若当前方块状态与服务端权威不符，回滚到服务端权威状态；
        // 若玩家与恢复后方块碰撞，absSnapTo 回弹到预测前位置（it->second.playerPos）。
        world.syncBlockState(pos, it->second.blockState, it->second.playerPos);
        m_serverVerifiedStates.erase(it);
    }
}

void BlockStatePredictionHandler::reset()
{
    m_serverVerifiedStates.clear();
    m_currentSequenceNr = -1;
    m_isPredicting = false;
}

} // namespace mc::client
