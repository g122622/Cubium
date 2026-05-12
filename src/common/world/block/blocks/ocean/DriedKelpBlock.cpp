#include "DriedKelpBlock.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DriedKelpBlock 实现
// ============================================================================

DriedKelpBlock::DriedKelpBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 干海带块不需要特殊逻辑
}

} // namespace blocks
} // namespace mc
