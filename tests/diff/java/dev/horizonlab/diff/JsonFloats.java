package dev.horizonlab.diff;

import com.google.gson.JsonObject;

/**
 * oracle 输出浮点结果的统一编码。
 *
 * <p>有限值写 JSON number，交给编排层按容差比较——项目不对齐浮点精度，末位差异不是缺陷。
 *
 * <p>非有限值必须写成字符串 {@code "nan"} / {@code "inf"} / {@code "-inf"}：Gson 的
 * {@code addProperty(double)} 会直接写出裸的 {@code NaN}/{@code Infinity}，那不是合法 JSON，
 * 编排层解析时会立刻报错；nlohmann 遇到同样的值会写 {@code null}，两侧语义也对不上。
 * 用这三个字符串，两侧才落在同一个可比较的写法上。
 *
 * <p>探针侧对应 {@code probes/probe.hpp} 的 {@code encodeFloat}，两边的判定必须一致。
 */
public final class JsonFloats {
    private JsonFloats() {}

    /** 把浮点值编码进 {@code key}，有限值用数字、非有限值用字符串。 */
    public static void add(JsonObject output, String key, double value) {
        if (Double.isNaN(value)) {
            output.addProperty(key, "nan");
        } else if (Double.isInfinite(value)) {
            output.addProperty(key, value > 0 ? "inf" : "-inf");
        } else {
            output.addProperty(key, value);
        }
    }
}