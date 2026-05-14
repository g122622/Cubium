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

namespace mc {

ClientDimensionManager::ClientDimensionManager()
    : m_overworldType(std::make_unique<DimensionType>(DimensionType::overworld()))
    , m_netherType(std::make_unique<DimensionType>(DimensionType::nether()))
    , m_endType(std::make_unique<DimensionType>(DimensionType::theEnd()))
{}

void ClientDimensionManager::initialize(const std::vector<DimensionId>& dimensionInfo)
{
    m_availableDimensions = dimensionInfo;

    // 确保至少有主世界
    if (m_availableDimensions.empty()) {
        m_availableDimensions.push_back(0);
    }

    // 默认在主世界
    m_currentDimension = 0;
    m_transitionState = TransitionState::None;
}

void ClientDimensionManager::reset()
{
    m_currentDimension = 0;
    m_targetDimension = 0;
    m_targetPosition = Vector3d();
    m_transitionState = TransitionState::None;
    m_availableDimensions.clear();
    m_needsRenderReset = false;
}

void ClientDimensionManager::setCurrentDimension(DimensionId dimension)
{
    if (m_currentDimension != dimension) {
        m_currentDimension = dimension;
        m_needsRenderReset = true;
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
}

void ClientDimensionManager::completeDimensionChange()
{
    m_currentDimension = m_targetDimension;
    m_transitionState = TransitionState::None;
    m_targetDimension = 0;
}

void ClientDimensionManager::cancelDimensionChange()
{
    m_transitionState = TransitionState::None;
    m_targetDimension = 0;
    m_targetPosition = Vector3d();
}

bool ClientDimensionManager::isDimensionAvailable(DimensionId dimension) const
{
    return std::find(m_availableDimensions.begin(), m_availableDimensions.end(), dimension) !=
        m_availableDimensions.end();
}

const DimensionType* ClientDimensionManager::getDimensionType(DimensionId dimension) const
{
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    // TODO 未来可能需要支持更多维度，这里可以改成一个枚举 + 映射表来存储维度ID和维度类型的对应关系
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
