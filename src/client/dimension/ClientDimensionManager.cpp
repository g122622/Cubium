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

#include "ClientDimensionManager.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

ClientDimensionManager::ClientDimensionManager() {}

void ClientDimensionManager::initialize(const std::vector<DimensionId>& dimensionInfo)
{
    // 转换为 ClientDimensionInfo 格式
    std::vector<ClientDimensionInfo> infos;
    infos.reserve(dimensionInfo.size());

    for (DimensionId id : dimensionInfo) {
        ClientDimensionInfo info;
        info.id = id;

        // 使用 DimensionType::fromId() 获取维度属性
        DimensionType dimType = DimensionType::fromId(id);
        info.name = dimType.name();
        info.hasSkyLight = dimType.hasSkyLight();
        info.hasCeiling = dimType.hasCeiling();
        info.ambientLight = dimType.ambientLight();

        infos.push_back(info);
    }

    initialize(infos);
}

void ClientDimensionManager::initialize(const std::vector<ClientDimensionInfo>& dimensionInfo)
{
    m_availableDimensions = dimensionInfo;

    // 重建快速查找用的数据结构
    m_availableDimensionIds.clear();
    m_dimensionIndexMap.clear();

    m_availableDimensionIds.reserve(dimensionInfo.size());

    for (size_t i = 0; i < dimensionInfo.size(); ++i) {
        m_availableDimensionIds.push_back(dimensionInfo[i].id);
        m_dimensionIndexMap[dimensionInfo[i].id] = i;
    }

    // 确保至少有主世界
    if (m_availableDimensions.empty()) {
        ClientDimensionInfo overworld;
        overworld.id = 0;
        overworld.name = "minecraft:overworld";
        overworld.hasSkyLight = true;
        overworld.hasCeiling = false;
        overworld.ambientLight = 0.0f;

        m_availableDimensions.push_back(overworld);
        m_availableDimensionIds.push_back(0);
        m_dimensionIndexMap[0] = 0;
    }

    // 默认在主世界
    m_currentDimension = 0;
    m_transitionState = TransitionState::None;

    spdlog::info("[ClientDimensionManager] Initialized with {} dimensions", m_availableDimensions.size());
    for (const auto& dim : m_availableDimensions) {
        spdlog::debug("[ClientDimensionManager]   - Dimension {}: {} (hasSkyLight={}, hasCeiling={}, ambientLight={})",
            static_cast<i32>(dim.id),
            dim.name,
            dim.hasSkyLight,
            dim.hasCeiling,
            dim.ambientLight);
    }
}

void ClientDimensionManager::reset()
{
    m_currentDimension = 0;
    m_targetDimension = 0;
    m_targetPosition = Vector3d();
    m_transitionState = TransitionState::None;
    m_availableDimensions.clear();
    m_availableDimensionIds.clear();
    m_dimensionIndexMap.clear();
    m_needsRenderReset = false;
}

void ClientDimensionManager::setCurrentDimension(DimensionId dimension)
{
    if (m_currentDimension != dimension) {
        m_currentDimension = dimension;
        m_needsRenderReset = true;
        spdlog::info("[ClientDimensionManager] Current dimension changed to {}", static_cast<i32>(dimension));
    }
}

const DimensionType* ClientDimensionManager::currentDimensionType() const
{
    return getDimensionType(m_currentDimension);
}

void ClientDimensionManager::beginDimensionChange(DimensionId targetDimension, const Vector3d& position)
{
    m_targetDimension = targetDimension;
    m_targetPosition = position;
    m_transitionState = TransitionState::Leaving;
    m_needsRenderReset = true;

    const ClientDimensionInfo* targetInfo = getDimensionInfo(targetDimension);
    if (targetInfo) {
        spdlog::info("[ClientDimensionManager] Beginning dimension change to {} ({})",
            static_cast<i32>(targetDimension),
            targetInfo->name);
    } else {
        spdlog::info("[ClientDimensionManager] Beginning dimension change to {}", static_cast<i32>(targetDimension));
    }
}

void ClientDimensionManager::completeDimensionChange()
{
    spdlog::info("[ClientDimensionManager] Dimension change completed: {} -> {}",
        static_cast<i32>(m_currentDimension),
        static_cast<i32>(m_targetDimension));

    m_currentDimension = m_targetDimension;
    m_transitionState = TransitionState::None;
    m_targetDimension = 0;
}

void ClientDimensionManager::cancelDimensionChange()
{
    spdlog::info("[ClientDimensionManager] Dimension change cancelled");
    m_transitionState = TransitionState::None;
    m_targetDimension = 0;
    m_targetPosition = Vector3d();
}

bool ClientDimensionManager::isDimensionAvailable(DimensionId dimension) const
{
    return m_dimensionIndexMap.find(dimension) != m_dimensionIndexMap.end();
}

const ClientDimensionInfo* ClientDimensionManager::getDimensionInfo(DimensionId dimension) const
{
    auto it = m_dimensionIndexMap.find(dimension);
    if (it != m_dimensionIndexMap.end() && it->second < m_availableDimensions.size()) {
        return &m_availableDimensions[it->second];
    }
    return nullptr;
}

const DimensionType* ClientDimensionManager::getDimensionType(DimensionId dimension) const
{
    // 使用静态变量存储预定义的维度类型
    // 这些维度类型实例在程序生命周期内保持有效
    static const DimensionType s_overworldType = DimensionType::overworld();
    static const DimensionType s_netherType = DimensionType::nether();
    static const DimensionType s_endType = DimensionType::theEnd();

    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    switch (dimension) {
        case 0: // Overworld
            return &s_overworldType;
        case -1: // Nether
            return &s_netherType;
        case 1: // The End
            return &s_endType;
        default:
            return nullptr;
    }
}

} // namespace mc
