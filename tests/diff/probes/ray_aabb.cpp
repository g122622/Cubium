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
 * 最后一次用 `<` 而前两次用 `>`，这种不对称是边界行为的关键。
 *
 * 用例取三组集合的**笛卡尔积**（盒 × 起点 × 方向），而不是手工挑几个组合——手挑容易
 * 漏掉「某个轴上刚好朝外」「某个轴恰好擦边」这类只在特定组合下才出现的路径。
 *
 * 单元格式：`<盒下标>|<起点下标>|<方向下标>@<组合序号>`
 */
struct BoxCase {
    const char* id;
    f64 minX, minY, minZ, maxX, maxY, maxZ;
};

struct PointCase {
    const char* id;
    f64 x, y, z;
};

const std::vector<BoxCase>& boxCases()
{
    static const std::vector<BoxCase> table = {
        // 单位立方体：常规情形
        {"unit", 0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        // 偏移且非单位：中心不在原点
        {"offset", -3.5, 2.25, 100.75, 1.5, 4.75, 104.25},
        // 扁平（y 零厚）：退化的一轴
        {"flat_y", 0.0, 0.5, 0.0, 1.0, 0.5, 1.0},
        // 薄片（x 零厚）
        {"flat_x", 0.5, 0.0, 0.0, 0.5, 1.0, 1.0},
        // 零体积的点
        {"point", 0.5, 0.5, 0.5, 0.5, 0.5, 0.5},
        // 大坐标：精度边界
        {"large", 1.0e6, -1.0e6, 1.0e6, 2.0e6, -5.0e5, 3.0e6},
        // 极小盒：半宽到 f64 精度边缘
        {"tiny", 0.0, 0.0, 0.0, 1.0e-9, 1.0e-9, 1.0e-9},
        // 各轴尺寸悬殊
        {"slab", -10.0, -0.5, -0.5, 10.0, 0.5, 0.5},
    };
    return table;
}

const std::vector<PointCase>& originCases()
{
    static const std::vector<PointCase> table = {
        {"origin", 0.0, 0.0, 0.0},
        // 单位盒的几何中心
        {"center", 0.5, 0.5, 0.5},
        // 各轴正负远离
        {"neg_x_far", -10.0, 0.5, 0.5},
        {"pos_x_far", 10.0, 0.5, 0.5},
        {"neg_y_far", 0.5, -10.0, 0.5},
        {"pos_y_far", 0.5, 10.0, 0.5},
        {"neg_z_far", 0.5, 0.5, -10.0},
        {"pos_z_far", 0.5, 0.5, 10.0},
        // 对角远点
        {"neg_all", -10.0, -10.0, -10.0},
        {"pos_all", 10.0, 10.0, 10.0},
        // 恰好在边界上
        {"on_face", 0.0, 0.5, 0.5},
        {"on_edge", 0.0, 0.0, 0.5},
        {"on_corner", 0.0, 0.0, 0.0},
        // 边界外一点点
        {"just_out_face", -1.0e-9, 0.5, 0.5},
        // 大坐标起点
        {"large_origin", 1.5e6, -7.5e5, 2.0e6},
        // 数值极小的偏移：起点几乎在中心
        {"near_center", 0.5 + 1.0e-12, 0.5, 0.5},
    };
    return table;
}

const std::vector<PointCase>& directionCases()
{
    static const std::vector<PointCase> table = {
        // 零向量：无法确定方向
        {"zero", 0.0, 0.0, 0.0},
        // 六向单位向量：只有两个方向能进盒
        {"pos_x", 1.0, 0.0, 0.0},
        {"neg_x", -1.0, 0.0, 0.0},
        {"pos_y", 0.0, 1.0, 0.0},
        {"neg_y", 0.0, -1.0, 0.0},
        {"pos_z", 0.0, 0.0, 1.0},
        {"neg_z", 0.0, 0.0, -1.0},
        // 对角：指向中心与背离中心
        {"diag_in", 1.0, 1.0, 1.0},
        {"diag_out", -1.0, -1.0, -1.0},
        // 主方向占优的斜向
        {"mostly_x", 1.0, 0.01, 0.01},
        {"mostly_y", 0.01, 1.0, 0.01},
        // 非归一化的大方向
        {"scaled", 100.0, -50.0, 25.0},
    };
    return table;
}

void writeList(std::FILE* out)
{
    size_t index = 0;
    for (size_t bi = 0; bi < boxCases().size(); ++bi) {
        for (size_t oi = 0; oi < originCases().size(); ++oi) {
            for (size_t di = 0; di < directionCases().size(); ++di) {
                std::fprintf(out, "%zu|%zu|%zu@%zu\n", bi, oi, di, index);
                ++index;
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t lastAt = requested.rfind('@');
    if (lastAt == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string triple = requested.substr(0, lastAt);

    // 解析 `<盒>|<起点>|<方向>`
    std::vector<size_t> parts;
    size_t start = 0;
    while (true) {
        const size_t pos = triple.find('|', start);
        const std::string piece = triple.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
        parts.push_back(static_cast<size_t>(std::stoul(piece)));
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    if (parts.size() != 3 || parts[0] >= boxCases().size() || parts[1] >= originCases().size() ||
        parts[2] >= directionCases().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const BoxCase& box = boxCases()[parts[0]];
    const PointCase& origin = originCases()[parts[1]];
    const PointCase& direction = directionCases()[parts[2]];

    // 直接调用生产代码，不在这里重写求交逻辑。
    const bool hit = math::rayIntersectsAABB(origin.x,
        origin.y,
        origin.z,
        direction.x,
        direction.y,
        direction.z,
        box.minX,
        box.minY,
        box.minZ,
        box.maxX,
        box.maxY,
        box.maxZ);
    return json{{"unit", requested}, {"hit", hit}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(ray_aabb, &writeList, &runLine)

} // namespace mc::diff
