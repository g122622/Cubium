#include "common/test/base/data/TestData.hpp"

#include <nlohmann/json.hpp>

namespace {

// Rotation ↔ 字符串映射（对齐 Java Rotation codec 名）
// 文件局部工具，避免污染命名空间
const char* rotationToString(mc::Rotation r) noexcept
{
    switch (r) {
        case mc::Rotation::None:
            return "none";
        case mc::Rotation::Clockwise90:
            return "clockwise_90";
        case mc::Rotation::Clockwise180:
            return "180";
        case mc::Rotation::CounterClockwise90:
            return "counterclockwise_90";
    }
    return "none";
}

mc::Rotation rotationFromString(std::string_view s)
{
    if (s == "clockwise_90") return mc::Rotation::Clockwise90;
    if (s == "180") return mc::Rotation::Clockwise180;
    if (s == "counterclockwise_90") return mc::Rotation::CounterClockwise90;
    return mc::Rotation::None;
}

} // namespace

namespace mc::test {

// ADL 序列化（nlohmann::json 经参数依赖查找发现这两个自由函数）。
// 不用 adl_serializer 显式特化：json.hpp 已定义 adl_serializer 主模板，特化需在 nlohmann 命名空间
// 且与主模板的可见性易冲突（ambiguous）；ADL 自由函数是 nlohmann 官方推荐的最简形式。
void to_json(nlohmann::json& j, const TestData& d)
{
    j = nlohmann::json{
        {"environment", d.m_environment},
        {"structure", d.m_structure},
        {"max_ticks", d.m_maxTicks},
        {"setup_ticks", d.m_setupTicks},
        {"required", d.m_required},
        {"rotation", rotationToString(d.m_rotation)},
        {"manual_only", d.m_manualOnly},
        {"max_attempts", d.m_maxAttempts},
        {"required_successes", d.m_requiredSuccesses},
        {"sky_access", d.m_skyAccess},
        {"padding", d.m_padding},
        {"batch_name", d.m_batchName},
    };
}

void from_json(const nlohmann::json& j, TestData& d)
{
    if (j.contains("environment")) j.at("environment").get_to(d.m_environment);
    if (j.contains("structure")) j.at("structure").get_to(d.m_structure);
    if (j.contains("max_ticks")) j.at("max_ticks").get_to(d.m_maxTicks);
    if (j.contains("setup_ticks")) j.at("setup_ticks").get_to(d.m_setupTicks);
    if (j.contains("required")) j.at("required").get_to(d.m_required);
    if (j.contains("rotation")) {
        std::string s;
        j.at("rotation").get_to(s);
        d.m_rotation = rotationFromString(s);
    }
    if (j.contains("manual_only")) j.at("manual_only").get_to(d.m_manualOnly);
    if (j.contains("max_attempts")) j.at("max_attempts").get_to(d.m_maxAttempts);
    if (j.contains("required_successes")) j.at("required_successes").get_to(d.m_requiredSuccesses);
    if (j.contains("sky_access")) j.at("sky_access").get_to(d.m_skyAccess);
    if (j.contains("padding")) j.at("padding").get_to(d.m_padding);
    if (j.contains("batch_name")) j.at("batch_name").get_to(d.m_batchName);
}

} // namespace mc::test
