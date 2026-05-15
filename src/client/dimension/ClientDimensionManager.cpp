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

ClientDimensionManager::ClientDimensionManager()
    : m_overworldType(std::make_unique<DimensionType>(DimensionType::overworld()))
    , m_netherType(std::make_unique<DimensionType>(DimensionType::nether()))
    , m_endType(std::make_unique<DimensionType>(DimensionType::theEnd()))
{}

void ClientDimensionManager::initialize(const std::vector<DimensionId>& dimensionInfo)
{
    // 转换为 ClientDimensionInfo 格式
    std::vector<ClientDimensionInfo> infos;
    infos.reserve(dimensionInfo.size());

    for (DimensionId id : dimensionInfo) {
        ClientDimensionInfo info;
        info.id = id;

        // 根据 ID 设置默认名称和属性
        switch (id) {
            case 0: // Overworld
                info.name = "minecraft:overworld";
                info.hasSkyLight = true;
                info.hasCeiling = false;
                info.ambientLight = 0.0f;
                break;
            case -1: // Nether
                info.name = "minecraft:the_nether";
                info.hasSkyLight = false;
                info.hasCeiling = true;
                info.ambientLight = 0.1f;
                break;
            case 1: // The End
                info.name = "minecraft:the_end";
                info.hasSkyLight = false;
                info.hasCeiling = false;
                info.ambientLight = 0.0f;
                break;
            default:
                info.name = "minecraft:unknown";
                info.hasSkyLight = true;
                info.hasCeiling = false;
                info.ambientLight = 0.0f;
                break;
        }

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
            static_cast<i32>(dim.id), dim.name, dim.hasSkyLight, dim.hasCeiling, dim.ambientLight);
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
            static_cast<i32>(targetDimension), targetInfo->name);
    } else {
        spdlog::info("[ClientDimensionManager] Beginning dimension change to {}",
            static_cast<i32>(targetDimension));
    }
}

void ClientDimensionManager::completeDimensionChange()
{
    spdlog::info("[ClientDimensionManager] Dimension change completed: {} -> {}",
        static_cast<i32>(m_currentDimension), static_cast<i32>(m_targetDimension));

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
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    // 这里使用预定义的维度类型，因为 DimensionType 包含服务器没有发送的额外属性
    switch (dimension) {
        case 0: // Overworld
            return m_overworldType.get();
        case -1: // Nether (MC 1.16.5 使用 -1)
            return m_netherType.get();
        case 1: // The End (MC 1.16.5 使用 1)
            return m_endType.get();
        default:
            return nullptr;
    }
}

} // namespace mc
