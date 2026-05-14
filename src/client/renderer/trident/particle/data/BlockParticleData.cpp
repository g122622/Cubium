#include "BlockParticleData.hpp"
#include "../ParticleRegistry.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::data {

BlockParticleData::BlockParticleData(ParticleTypeId type, BlockState blockState)
    : m_type(type)
    , m_blockState(std::move(blockState))
{
    MC_ASSERT_MSG(requiresBlockState(type), "BlockParticleData requires a block-type particle");
}

std::string BlockParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(m_type);
}

std::string BlockParticleData::getParameters() const
{
    // 方块粒子参数格式: block_state
    // 例如: minecraft:stone
    std::string result = m_blockState.blockLocation().toString();
    const std::string modelKey = m_blockState.toModelKey();
    if (!modelKey.empty()) {
        result += "[";
        result += modelKey;
        result += "]";
    }
    return result;
}

std::unique_ptr<ParticleData> BlockParticleData::clone() const
{
    return std::make_unique<BlockParticleData>(m_type, m_blockState);
}

} // namespace mc::client::renderer::trident::particle::data
