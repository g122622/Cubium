#pragma once
#include "common/core/Types.hpp"
#include <cmath>

namespace mc::util::core {

class CoordConverter {
public:
    // ========== 区块坐标转换 ==========
    static ChunkCoord blockToChunk(f32 blockCoord) { return static_cast<ChunkCoord>(std::floor(blockCoord / 16.0f)); }

    static ChunkCoord blockToChunk(i32 blockCoord)
    {
        return static_cast<ChunkCoord>(blockCoord >> 4); // blockCoord / 16
    }
};

} // namespace mc::util::core
