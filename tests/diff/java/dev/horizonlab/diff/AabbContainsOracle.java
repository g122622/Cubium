/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;
import net.minecraft.world.phys.AABB;

/**
 * {@code AABB} 包含判定与尺寸的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AabbContainsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link AABB#contains(double, double, double)} / {@link AABB#getSize()} /
 * {@link AABB#getXsize()} / {@link AABB#getYsize()} / {@link AABB#getZsize()} /
 * {@link AABB#deflate(double, double, double)} / {@link AABB#contract(double, double, double)} /
 * {@link AABB#hasNaN()} / {@link AABB#distanceToSqr(AABB)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>所有浮点按 `%.6g` 格式化后去掉尾随零，与其它浮点能力的契约一致。
 *
 * <p>单元表必须与 probes/aabb_contains.cpp 逐项一致。
 */
public final class AabbContainsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0,0,0,1,1,1|contains|0|0|0",
                    "0,0,0,1,1,1|contains|1|1|1",
                    "0,0,0,1,1,1|contains|0.999|0.999|0.999",
                    "0,0,0,1,1,1|contains|0.5|0.5|0.5",
                    "0,0,0,1,1,1|contains|-0.001|0.5|0.5",
                    "0,0,0,1,1,1|contains|0.5|-0.001|0.5",
                    "0,0,0,1,1,1|contains|0.5|0.5|-0.001",
                    "0,0,0,1,1,1|contains|1|0.5|0.5",
                    "0.5,0.5,0.5,1.5,2.5,3.5|contains|0.5|0.5|0.5",
                    "0.5,0.5,0.5,1.5,2.5,3.5|contains|1.5|2.5|3.5",
                    "0,0,0,1,1,1|getSize",
                    "0,0,0,1,2,3|getSize",
                    "0,0,0,1,2,3|getXsize",
                    "0,0,0,1,2,3|getYsize",
                    "0,0,0,1,2,3|getZsize",
                    "0.5,0.5,0.5,1.5,2.5,3.5|getSize",
                    "0,0,0,1,1,1|deflate|0.25|0.25|0.25",
                    "0,0,0,1,1,1|deflate|-0.5|-0.5|-0.5",
                    "0,0,0,2,2,2|deflate|0.5|0|0",
                    "0,0,0,1,1,1|contract|0.5|0|0",
                    "0,0,0,1,1,1|contract|-0.5|0|0",
                    "0,0,0,1,1,1|contract|0|0.25|0",
                    "0,0,0,2,2,2|contract|0.5|0.5|0.5",
                    "0,0,0,2,2,2|contract|-0.5|-0.5|-0.5",
                    "0,0,0,1,1,1|contract|0|0|0",
                    "0,0,0,1,1,1|hasNaN",
                    "0,0,0,1,1,1|distAabb|0.5,0.5,0.5,1.5,1.5,1.5",
                    "0,0,0,1,1,1|distAabb|2,0,0,3,1,1",
                    "0,0,0,1,1,1|distAabb|2,2,2,3,3,3",
                    "0,0,0,1,1,1|distAabb|5,5,5,6,6,6");

    private AabbContainsOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: AabbContainsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static AABB parseBox(String text) {
        String[] parts = text.split(",", -1);
        if (parts.length != 6) {
            throw new IllegalArgumentException("aabb_contains: 包围盒必须六段: " + text);
        }
        return new AABB(
                Double.parseDouble(parts[0]), Double.parseDouble(parts[1]),
                Double.parseDouble(parts[2]), Double.parseDouble(parts[3]),
                Double.parseDouble(parts[4]), Double.parseDouble(parts[5]));
    }

    /**
     * 按 `%.6g` 格式化后去掉尾随零。
     *
     * <p>与探针侧的 `formatNumber` 保持一致。
     */
    private static String formatNumber(double value) {
        String text = String.format(Locale.ROOT, "%.6g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            int end = text.length();
            while (end > 0 && text.charAt(end - 1) == '0') {
                end--;
            }
            if (end > 0 && text.charAt(end - 1) == '.') {
                end--;
            }
            text = text.substring(0, end);
        }
        return text;
    }

    private static JsonArray boxToJson(AABB box) {
        JsonArray array = new JsonArray();
        array.add(formatNumber(box.minX));
        array.add(formatNumber(box.minY));
        array.add(formatNumber(box.minZ));
        array.add(formatNumber(box.maxX));
        array.add(formatNumber(box.maxY));
        array.add(formatNumber(box.maxZ));
        return array;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("aabb_contains: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        AABB box = parseBox(parts[0]);
        String mode = parts[1];

        switch (mode) {
            case "contains" -> out.addProperty("value",
                    box.contains(
                            Double.parseDouble(parts[2]),
                            Double.parseDouble(parts[3]),
                            Double.parseDouble(parts[4])) ? "true" : "false");
            case "getSize" -> out.addProperty("value", formatNumber(box.getSize()));
            case "getXsize" -> out.addProperty("value", formatNumber(box.getXsize()));
            case "getYsize" -> out.addProperty("value", formatNumber(box.getYsize()));
            case "getZsize" -> out.addProperty("value", formatNumber(box.getZsize()));
            case "deflate" -> out.add("value", boxToJson(box.deflate(
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]),
                    Double.parseDouble(parts[4]))));
            case "contract" -> out.add("value", boxToJson(box.contract(
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]),
                    Double.parseDouble(parts[4]))));
            case "hasNaN" -> out.addProperty("value", box.hasNaN() ? "true" : "false");
            case "distAabb" -> out.addProperty("value",
                    formatNumber(box.distanceToSqr(parseBox(parts[2]))));
            default -> throw new IllegalArgumentException("aabb_contains: 无法识别的单元: " + unit);
        }

        return out;
    }
}