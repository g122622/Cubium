#include "probe.hpp"

#include "common/util/math/MathUtils.hpp"

#include <nlohmann/json.hpp>

#include <bit>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 射线与轴对齐包围盒求交，对应原版 `Mth.rayIntersectsAABB`
 *
 * 这是实体拾取、区块剔除等热路径上的判定，逻辑分支密集：
 *
 * 1. 三轴各做一次「起点在盒外且方向朝外」的快速排除；
 * 2. 三对轴做叉积，前两次超出即排除，**最后一次要在界内才相交**。
 *
 * 最后一次用 `<` 而前两次用 `>`，这种不对称是边界行为的关键。用例必须包含
 * 「擦边」「正好在界上」「退化盒（零体积）」这几类。
 *
 * 单元格式：`<射线起点>|<射线方向>|<盒边界>@<组合下标>`
 */
struct RayCase {
    const char* id;
    f64 ox, oy, oz;
    f64 dx, dy, dz;
    f64 minX, minY, minZ, maxX, maxY, maxZ;
};

const std::vector<RayCase>& rayCases()
{
    static const std::vector<RayCase> table = {
        // 单位盒，射线从远处正对着射入
        {"hit_x", -10.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 从内部出发（必然相交）
        {"inside", 0.5, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 背向：起在盒外、方向朝外
        {"away", -10.0, 0.5, 0.5, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 平行且擦边：y 恰好等于 maxY
        {"graze_edge", -10.0, 1.0, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 平行且在盒外一点点
        {"just_outside", -10.0, 1.0000001, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 对角射线穿过盒心
        {"diagonal", -5.0, -5.0, -5.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 对角射线擦过一个角
        {"corner_graze", -5.0, 3.0, -5.0, 1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 退化盒：零体积的点
        {"degenerate_point", -10.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5},
        // 退化盒：扁平（y 方向零厚）
        {"degenerate_flat", -10.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.5, 0.0, 1.0, 0.5, 1.0},
        // 大坐标：精度边界
        {"large_coords", -1.0e6, 5.0e5, 5.0e5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0e6, 1.0e6, 1.0e6},
        // 射线方向为零向量
        {"zero_dir", -10.0, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 非单位盒 + 斜方向
        {"oblique", -3.0, 1.5, -3.0, 1.0, 0.25, 1.0, 0.0, 0.0, 0.0, 2.0, 3.0, 2.0},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (size_t i = 0; i < rayCases().size(); ++i) {
        std::fprintf(out, "%s@%zu\n", rayCases()[i].id, i);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t at = requested.rfind('@');
    if (at == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string caseId = requested.substr(0, at);
    const size_t index = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (index >= rayCases().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const RayCase& c = rayCases()[index];
    if (caseId != c.id) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    // 直接调用生产代码，不在这里重写求交逻辑。
    const bool hit =
        math::rayIntersectsAABB(c.ox, c.oy, c.oz, c.dx, c.dy, c.dz, c.minX, c.minY, c.minZ, c.maxX, c.maxY, c.maxZ);
    return json{{"unit", requested}, {"hit", hit}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(ray_aabb, &writeList, &runLine)

} // namespace mc::diff
