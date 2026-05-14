#pragma once

#include "../../../../core/Types.hpp"
#include <functional>
#include <memory>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief 内存模块状态枚举
 *
 * 用于检查内存模块的状态
 */
enum class MemoryModuleStatus {
    VALUE_PRESENT, // 内存有值
    VALUE_ABSENT,  // 内存无值
    REGISTERED     // 已注册
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc

// std::hash 特化 - 用于 std::unordered_set/std::unordered_map
namespace std {

template <>
struct hash<mc::entity::ai::brain::memory::MemoryModuleStatus> {
    size_t operator()(mc::entity::ai::brain::memory::MemoryModuleStatus status) const noexcept
    {
        return static_cast<size_t>(status);
    }
};

} // namespace std
