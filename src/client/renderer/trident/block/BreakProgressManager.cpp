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

#include "BreakProgressManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

BreakProgressManager& BreakProgressManager::instance()
{
    static BreakProgressManager instance;
    return instance;
}

void BreakProgressManager::initialize()
{
    spdlog::info("BreakProgressManager: Initialized");
}

void BreakProgressManager::cleanup()
{
    m_localBreaking = false;
    m_localProgress = 0.0;
    m_localDamageStage = 0;
    m_remoteProgressByEntity.clear();
    m_remoteProgressByPos.clear();
    spdlog::info("BreakProgressManager: Cleaned up");
}

void BreakProgressManager::tick(f64 deltaTime, u64 currentTick)
{
    m_currentTick = currentTick;
    _cleanupStaleProgress(currentTick);
}

void BreakProgressManager::startBreaking(const BlockPos& pos)
{
    m_localBreaking = true;
    m_localBreakPos = pos;
    m_localProgress = 0.0;
    m_localDamageStage = 0;
}

u8 BreakProgressManager::updateLocalProgress(const BlockPos& pos, f64 progress)
{
    if (!m_localBreaking || m_localBreakPos != pos) {
        startBreaking(pos);
    }

    m_localProgress = std::clamp(progress, 0.0, 1.0);

    u8 newStage = static_cast<u8>(std::min<f64>(static_cast<f64>(MAX_DAMAGE_STAGE), progress * 10.0));

    // 阶段变化时播放击打音效
    if (newStage != m_localDamageStage && newStage > 0) {
        // 播放击打音效
        if (m_hitSoundCallback) {
            m_hitSoundCallback(m_localBreakPos, newStage);
        }
    }

    m_localDamageStage = newStage;
    return m_localDamageStage;
}

void BreakProgressManager::stopBreaking()
{
    m_localBreaking = false;
    m_localProgress = 0.0;
    m_localDamageStage = 0;
}

void BreakProgressManager::updateRemoteProgress(
    EntityInstanceId breakerId, const BlockPos& pos, i8 stage, u64 currentTick)
{
    if (stage < 0 || stage > static_cast<i8>(MAX_DAMAGE_STAGE)) {
        removeRemoteProgress(breakerId);
        return;
    }

    auto it = m_remoteProgressByEntity.find(breakerId);
    if (it == m_remoteProgressByEntity.end()) {
        BlockBreakProgress progress;
        progress.breakerId = breakerId;
        progress.position = pos;
        progress.damageStage = static_cast<u8>(stage);
        progress.creationTick = currentTick;
        progress.lastUpdateTick = currentTick;

        m_remoteProgressByEntity[breakerId] = progress;
        _updatePositionIndex(progress);
    } else {
        BlockPos oldPos = it->second.position;

        if (oldPos != pos) {
            _removeFromPositionIndex(oldPos, breakerId);
            it->second.position = pos;
            _updatePositionIndex(it->second);
        }

        it->second.damageStage = static_cast<u8>(stage);
        it->second.lastUpdateTick = currentTick;
    }
}

void BreakProgressManager::removeRemoteProgress(EntityInstanceId breakerId)
{
    auto it = m_remoteProgressByEntity.find(breakerId);
    if (it != m_remoteProgressByEntity.end()) {
        BlockPos pos = it->second.position;
        _removeFromPositionIndex(pos, breakerId);
        m_remoteProgressByEntity.erase(it);
    }
}

void BreakProgressManager::clearRemoteProgress()
{
    m_remoteProgressByEntity.clear();
    m_remoteProgressByPos.clear();
}

u8 BreakProgressManager::getDamageStage(const BlockPos& pos) const
{
    u8 maxStage = 0;
    bool hasProgress = false;

    if (m_localBreaking && m_localBreakPos == pos) {
        maxStage = m_localDamageStage;
        hasProgress = true;
    }

    auto posIt = m_remoteProgressByPos.find(pos);
    if (posIt != m_remoteProgressByPos.end()) {
        for (EntityInstanceId breakerId : posIt->second) {
            auto entityIt = m_remoteProgressByEntity.find(breakerId);
            if (entityIt != m_remoteProgressByEntity.end()) {
                if (!hasProgress || entityIt->second.damageStage > maxStage) {
                    maxStage = entityIt->second.damageStage;
                }
                hasProgress = true;
            }
        }
    }

    return hasProgress ? maxStage : NO_DAMAGE;
}

std::vector<const BlockBreakProgress*> BreakProgressManager::getProgressAtPos(const BlockPos& pos) const
{
    std::vector<const BlockBreakProgress*> result;

    auto posIt = m_remoteProgressByPos.find(pos);
    if (posIt != m_remoteProgressByPos.end()) {
        for (EntityInstanceId breakerId : posIt->second) {
            auto entityIt = m_remoteProgressByEntity.find(breakerId);
            if (entityIt != m_remoteProgressByEntity.end()) {
                result.push_back(&entityIt->second);
            }
        }
    }

    return result;
}

std::vector<std::pair<BlockPos, u8>> BreakProgressManager::getVisibleProgress(const Vector3& cameraPos) const
{
    std::vector<std::pair<BlockPos, u8>> result;

    // 使用 unordered_map 去重同一位置的多个进度
    std::unordered_map<BlockPos, u8> positionToStage;

    // 添加本地进度
    if (m_localBreaking) {
        const f64 dx = static_cast<f64>(m_localBreakPos.x) - cameraPos.x;
        const f64 dy = static_cast<f64>(m_localBreakPos.y) - cameraPos.y;
        const f64 dz = static_cast<f64>(m_localBreakPos.z) - cameraPos.z;
        const f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= MAX_RENDER_DISTANCE_SQ) {
            positionToStage[m_localBreakPos] = m_localDamageStage;
        }
    }

    // 添加远程进度
    for (const auto& [breakerId, progress] : m_remoteProgressByEntity) {
        MC_UNUSED(breakerId);
        const f64 dx = static_cast<f64>(progress.position.x) - cameraPos.x;
        const f64 dy = static_cast<f64>(progress.position.y) - cameraPos.y;
        const f64 dz = static_cast<f64>(progress.position.z) - cameraPos.z;
        const f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= MAX_RENDER_DISTANCE_SQ) {
            auto it = positionToStage.find(progress.position);
            if (it != positionToStage.end()) {
                it->second = std::max(it->second, progress.damageStage);
            } else {
                positionToStage[progress.position] = progress.damageStage;
            }
        }
    }

    // 转换为 vector 返回
    result.reserve(positionToStage.size());
    for (const auto& [pos, stage] : positionToStage) {
        result.emplace_back(pos, stage);
    }

    return result;
}

void BreakProgressManager::getVisibleProgress(
    const Vector3& cameraPos, std::vector<std::pair<BlockPos, u8>>& outProgress) const
{
    outProgress.clear();

    // 快速路径：如果没有任何进度，直接返回
    if (!m_localBreaking && m_remoteProgressByEntity.empty()) {
        return;
    }

    // 预估容量，避免多次分配
    size_t estimatedSize = (m_localBreaking ? 1 : 0) + m_remoteProgressByEntity.size();
    outProgress.reserve(estimatedSize);

    // 添加本地进度
    if (m_localBreaking) {
        const f64 dx = static_cast<f64>(m_localBreakPos.x) - cameraPos.x;
        const f64 dy = static_cast<f64>(m_localBreakPos.y) - cameraPos.y;
        const f64 dz = static_cast<f64>(m_localBreakPos.z) - cameraPos.z;
        const f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= MAX_RENDER_DISTANCE_SQ) {
            outProgress.emplace_back(m_localBreakPos, m_localDamageStage);
        }
    }

    // 添加远程进度，去重处理
    for (const auto& [breakerId, progress] : m_remoteProgressByEntity) {
        MC_UNUSED(breakerId);
        const f64 dx = static_cast<f64>(progress.position.x) - cameraPos.x;
        const f64 dy = static_cast<f64>(progress.position.y) - cameraPos.y;
        const f64 dz = static_cast<f64>(progress.position.z) - cameraPos.z;
        const f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= MAX_RENDER_DISTANCE_SQ) {
            // 线性搜索去重（对于小数量更高效）
            bool found = false;
            for (auto& [pos, stage] : outProgress) {
                if (pos == progress.position) {
                    stage = std::max(stage, progress.damageStage);
                    found = true;
                    break;
                }
            }
            if (!found) {
                outProgress.emplace_back(progress.position, progress.damageStage);
            }
        }
    }
}

bool BreakProgressManager::hasProgressAt(const BlockPos& pos) const
{
    if (m_localBreaking && m_localBreakPos == pos) {
        return true;
    }

    return m_remoteProgressByPos.find(pos) != m_remoteProgressByPos.end();
}

void BreakProgressManager::_cleanupStaleProgress(u64 currentTick)
{
    std::vector<EntityInstanceId> toRemove;

    for (const auto& [breakerId, progress] : m_remoteProgressByEntity) {
        if (currentTick - progress.lastUpdateTick > PROGRESS_TIMEOUT_TICKS) {
            toRemove.push_back(breakerId);
        }
    }

    for (EntityInstanceId breakerId : toRemove) {
        removeRemoteProgress(breakerId);
    }
}

void BreakProgressManager::_updatePositionIndex(const BlockBreakProgress& progress)
{
    auto& entityList = m_remoteProgressByPos[progress.position];

    for (EntityInstanceId id : entityList) {
        if (id == progress.breakerId) {
            return;
        }
    }

    entityList.push_back(progress.breakerId);
}

void BreakProgressManager::_removeFromPositionIndex(const BlockPos& pos, EntityInstanceId breakerId)
{
    auto posIt = m_remoteProgressByPos.find(pos);
    if (posIt != m_remoteProgressByPos.end()) {
        auto& entityList = posIt->second;
        entityList.erase(std::remove(entityList.begin(), entityList.end(), breakerId), entityList.end());

        if (entityList.empty()) {
            m_remoteProgressByPos.erase(posIt);
        }
    }
}

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
