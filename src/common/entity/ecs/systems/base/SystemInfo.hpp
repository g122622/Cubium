#pragma once

#include "common/entity/ecs/systems/base/SystemCategory.hpp"

namespace mc::ecs {

/**
 * @brief System 元信息
 *
 * 承载 entt::organizer 推导不出的元信息：name / categories / blocking / usesEntityFactory。
 *
 * 依赖关系不由 SystemInfo 手填——由 entt::organizer 从 system 遍历函数签名自动推导
 * ro（const T&）/ rw（T&）（见 OrganizerGraph）。这是调度框架的核心能力：依赖图自动
 * 构建与冲突检测，阶段内可按拓扑序执行无冲突顶点。
 *
 * 注册 system 到 Collection 时显式提供 SystemInfo（自由函数/成员函数/sync_point 三类
 * 注册入口各带一个 SystemInfo 参数）。blocking=true 的 system 在阶段内强制串行；
 * usesEntityFactory=true 表示 tick 中可能创建/销毁实体（影响调度，需独占 registry）。
 */
struct SystemInfo {
    /// System 名（profiling 与日志）
    const char* name{nullptr};

    /// 所属 category 位掩码（决定走哪条 tick 路径，见 SystemCategory）
    SystemCategory categories{SystemCategory::Game};

    /// 是否阻塞——true 时阶段内强制串行
    bool blocking{false};

    /// 是否在 tick 中创建/销毁实体——true 时需独占 registry
    bool usesEntityFactory{false};
};

} // namespace mc::ecs
