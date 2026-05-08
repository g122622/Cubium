#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

#include <vector>

namespace mc::client::ui::minecraft::targetinfo {

enum class TargetInfoKind : u8 {
    None,
    Block,
    Entity,
};

class TargetInfoSnapshot {
public:
    TargetInfoSnapshot(TargetInfoKind kind, std::string title, std::vector<std::string> details, u32 accentColor);

    [[nodiscard]] static TargetInfoSnapshot none();

    [[nodiscard]] bool hasTarget() const noexcept { return m_kind != TargetInfoKind::None; }
    [[nodiscard]] TargetInfoKind kind() const noexcept { return m_kind; }
    [[nodiscard]] const std::string& title() const noexcept { return m_title; }
    [[nodiscard]] const std::vector<std::string>& details() const noexcept { return m_details; }
    [[nodiscard]] u32 accentColor() const noexcept { return m_accentColor; }

private:
    TargetInfoKind m_kind;
    std::string m_title;
    std::vector<std::string> m_details;
    u32 m_accentColor;
};

[[nodiscard]] std::string humanizeIdentifier(std::string_view identifier);
[[nodiscard]] std::string humanizeResourceLocation(const ResourceLocation& location);
[[nodiscard]] std::string formatDistance(f32 distance);
[[nodiscard]] std::string formatBlockPos(const BlockPos& pos);
[[nodiscard]] std::string formatDirection(Direction direction);

} // namespace mc::client::ui::minecraft::targetinfo