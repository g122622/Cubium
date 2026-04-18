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
    TargetInfoSnapshot(TargetInfoKind kind, String title, std::vector<String> details, u32 accentColor);

    [[nodiscard]] static TargetInfoSnapshot none();

    [[nodiscard]] bool hasTarget() const noexcept { return m_kind != TargetInfoKind::None; }
    [[nodiscard]] TargetInfoKind kind() const noexcept { return m_kind; }
    [[nodiscard]] const String& title() const noexcept { return m_title; }
    [[nodiscard]] const std::vector<String>& details() const noexcept { return m_details; }
    [[nodiscard]] u32 accentColor() const noexcept { return m_accentColor; }

private:
    TargetInfoKind m_kind;
    String m_title;
    std::vector<String> m_details;
    u32 m_accentColor;
};

[[nodiscard]] String humanizeIdentifier(StringView identifier);
[[nodiscard]] String humanizeResourceLocation(const ResourceLocation& location);
[[nodiscard]] String formatDistance(f32 distance);
[[nodiscard]] String formatBlockPos(const BlockPos& pos);
[[nodiscard]] String formatDirection(Direction direction);

} // namespace mc::client::ui::minecraft::targetinfo