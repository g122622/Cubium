#include "DimensionManager.hpp"
#include "../../util/assert/AssertAll.hpp"

namespace mc {

// ============================================================================
// 初始化
// ============================================================================

void DimensionManager::initialize(u64 seed)
{
    if (m_initialized) {
        return;
    }

    m_worldSeed = seed;
    registerVanillaDimensions(seed);
    m_initialized = true;
}

void DimensionManager::shutdown()
{
    m_dimensions.clear();
    m_nameToId.clear();
    m_initialized = false;
}

// ============================================================================
// 维度注册
// ============================================================================

bool DimensionManager::registerDimension(std::unique_ptr<Dimension> dimension)
{
    if (!dimension) {
        return false;
    }

    DimensionId id = dimension->id();

    // 检查是否已存在
    if (hasDimension(id)) {
        return false;
    }

    // 保存名称映射
    m_nameToId[dimension->type().name()] = id;

    // 注册维度
    m_dimensions[id] = std::move(dimension);

    return true;
}

bool DimensionManager::unregisterDimension(DimensionId id)
{
    auto it = m_dimensions.find(id);
    if (it == m_dimensions.end()) {
        return false;
    }

    // 移除名称映射
    const std::string& name = it->second->type().name();
    m_nameToId.erase(name);

    // 移除维度
    m_dimensions.erase(it);

    return true;
}

// ============================================================================
// 维度访问
// ============================================================================

Dimension* DimensionManager::getDimension(DimensionId id)
{
    auto it = m_dimensions.find(id);
    return it != m_dimensions.end() ? it->second.get() : nullptr;
}

const Dimension* DimensionManager::getDimension(DimensionId id) const
{
    auto it = m_dimensions.find(id);
    return it != m_dimensions.end() ? it->second.get() : nullptr;
}

bool DimensionManager::hasDimension(DimensionId id) const
{
    return m_dimensions.find(id) != m_dimensions.end();
}

Dimension* DimensionManager::getOverworld()
{
    return getDimension(OVERWORLD);
}

const Dimension* DimensionManager::getOverworld() const
{
    return getDimension(OVERWORLD);
}

Dimension* DimensionManager::getNether()
{
    return getDimension(NETHER);
}

const Dimension* DimensionManager::getNether() const
{
    return getDimension(NETHER);
}

Dimension* DimensionManager::getTheEnd()
{
    return getDimension(THE_END);
}

const Dimension* DimensionManager::getTheEnd() const
{
    return getDimension(THE_END);
}

// ============================================================================
// 维度类型
// ============================================================================

const DimensionType* DimensionManager::getDimensionType(DimensionId id) const
{
    const Dimension* dim = getDimension(id);
    return dim ? &dim->type() : nullptr;
}

DimensionId DimensionManager::getDimensionIdByName(const std::string& name) const
{
    auto it = m_nameToId.find(name);
    return it != m_nameToId.end() ? it->second : static_cast<DimensionId>(-1);
}

// ============================================================================
// 遍历
// ============================================================================

void DimensionManager::forEachDimension(std::function<void(Dimension&)> func)
{
    for (auto& pair : m_dimensions) {
        func(*pair.second);
    }
}

void DimensionManager::forEachDimension(std::function<void(const Dimension&)> func) const
{
    for (const auto& pair : m_dimensions) {
        func(*pair.second);
    }
}

// ============================================================================
// 信息
// ============================================================================

std::vector<DimensionId> DimensionManager::getDimensionIds() const
{
    std::vector<DimensionId> ids;
    ids.reserve(m_dimensions.size());
    for (const auto& pair : m_dimensions) {
        ids.push_back(pair.first);
    }
    return ids;
}

// ============================================================================
// 原版维度注册
// ============================================================================

void DimensionManager::registerVanillaDimensions(u64 seed)
{
    // 注册主世界
    auto overworld = Dimension::createOverworld(seed);
    MC_ASSERT(registerDimension(std::move(overworld)));

    // 注册下界
    auto nether = Dimension::createNether(seed);
    MC_ASSERT(registerDimension(std::move(nether)));

    // 注册末地
    auto theEnd = Dimension::createTheEnd(seed);
    MC_ASSERT(registerDimension(std::move(theEnd)));
}

} // namespace mc
