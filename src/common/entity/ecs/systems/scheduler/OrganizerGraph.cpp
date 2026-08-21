#include "common/entity/ecs/systems/scheduler/OrganizerGraph.hpp"

#include <sstream>

namespace mc::ecs {

void OrganizerGraph::emplaceSyncPoint(void (*callback)(const void*, Registry&), const void* payload, const char* name)
{
    // 用 organizer 第三种 emplace 重载（function_type* + payload），该重载内部强制
    // sync_point=true——永远串行，无依赖推导。回调桥接壳委托 EntityManager 遍历方法，
    // 不是纯 view 遍历，无法被 organizer 推导，强制串行是正确语义。
    m_organizer.emplace(callback, payload, name);
}

std::vector<OrganizerGraph::Vertex> OrganizerGraph::graph() const
{
    // entt::stl::vector 默认即 std::vector，无缝返回。
    return m_organizer.graph();
}

std::string OrganizerGraph::dumpDot() const
{
    // 遍历 vertex 的 out-edges 输出 Graphviz dot 格式。organizer 的 graph() 返回邻接表，
    // 每个 vertex 含 out_edges()（出边目标顶点下标）。organizer 内部的 adjacency_matrix
    // 是 private，无法直接喂给 entt::dot()，故此处手动遍历邻接表生成 dot。
    const auto vertices = m_organizer.graph();
    std::ostringstream out;
    out << "digraph{";

    // 顶点声明（带 name 标签 + ro/rw 资源数）
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto& v = vertices[i];
        out << "n" << i << "[label=\"";
        out << (v.name() != nullptr ? v.name() : "anon");
        out << " ro=" << v.ro_count() << " rw=" << v.rw_count();
        out << "\"];";
    }

    // 边（依赖关系：A->B 表示 B 依赖 A，即 A 须先执行）
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        for (const auto& to : vertices[i].out_edges()) {
            out << "n" << i << "->n" << to << ";";
        }
    }

    out << "}";
    return out.str();
}

std::size_t OrganizerGraph::size() const noexcept
{
    // organizer 未直接暴露顶点数，用 graph() 的大小。graph() 每次构建邻接表有开销，
    // 但 size() 仅用于调试/断言，非热路径。
    // TODO(性能): organizer 若未来暴露 vertices 数量的轻量接口则改用之。
    return m_organizer.graph().size();
}

void OrganizerGraph::clear()
{
    m_organizer.clear();
}

} // namespace mc::ecs
