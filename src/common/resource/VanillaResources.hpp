#pragma once

#include "common/resource/InMemoryResourcePack.hpp"
#include <memory>

namespace mc {

/**
 * @brief 原版内置资源
 *
 * 提供 Minecraft 原版的基础模型和 blockstates。
 * 这些资源作为内置资源包，优先级最低，始终加载。
 */
class VanillaResources {
public:
    /**
     * @brief 创建内置资源包
     * @return 包含原版基础模型的内存资源包
     */
    [[nodiscard]] static std::unique_ptr<InMemoryResourcePack> createResourcePack();

private:
    /**
     * @brief 注册基础模型
     */
    static void registerBaseModels(InMemoryResourcePack& pack);

    /**
     * @brief 注册 blockstates
     */
    static void registerBlockStates(InMemoryResourcePack& pack);

    // 模型模板
    static const char* MODEL_CUBE_ALL;     // 单面纹理方块
    static const char* MODEL_CUBE_COLUMN;  // 柱状方块（原木等）
    static const char* MODEL_CUBE;         // 六面不同纹理
    static const char* MODEL_LEAVES;       // 树叶
    static const char* MODEL_CROSS;        // 交叉纹理（花草等）
    static const char* MODEL_TINTED_CROSS; // 染色交叉纹理
    static const char* MODEL_AIR;          // 空气
};

} // namespace mc
