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
import net.minecraft.world.phys.AABB;

/**
 * {@code AABB} 几何运算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AabbOpsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接构造 {@link AABB} 并调用 {@link AABB#inflate(double, double, double)} /
 * {@link AABB#expandTowards(double, double, double)} / {@link AABB#intersect(AABB)} /
 * {@link AABB#minmax(AABB)} / {@link AABB#intersects(AABB)} / {@link AABB#move(double, double, double)}，
 * 不重写公式。
 *
 * <p>单元表必须与 probes/aabb_ops.cpp 逐项一致。
 */
public final class AabbOpsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0,0,0,1,1,1|inflate|1|1|1",
                    "0,0,0,1,1,1|inflate|0.5|0|0",
                    "0,0,0,1,1,1|inflate|-0.25|-0.25|-0.25",
                    "0,0,0,1,1,1|expand|1|0|0",
                    "0,0,0,1,1,1|expand|-1|0|0",
                    "0,0,0,1,1,1|expand|0|0|0",
                    "0,0,0,1,1,1|expand|2|-3|0.5",
                    "0,0,0,1,1,1|move|5|5|5",
                    "0,0,0,1,1,1|move|-0.5|2.25|0",
                    "0,0,0,2,2,2|0.5,0.5,0.5,3,3,3|intersect",
                    "0,0,0,1,1,1|5,5,5,6,6,6|intersect",
                    "0,0,0,2,2,2|0.5,0.5,0.5,3,3,3|minmax",
                    "0,0,0,1,1,1|5,5,5,6,6,6|minmax",
                    "0,0,0,2,2,2|1,1,1,3,3,3|intersects",
                    "0,0,0,1,1,1|1,1,1,2,2,2|intersects",
                    "0,0,0,1,1,1|5,5,5,6,6,6|intersects",
                    "0,0,0,10,10,10|2,2,2,3,3,3|intersects",
                    "0,0,0,1,1,1|1,0.2,0.2,2,0.8,0.8|intersects",
                    "0,0,0,1,1,1|0.2,0.2,5,0.8,0.8,6|intersects");

    private AabbOpsOracle() {}

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
                "usage: AabbOpsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        String[] parts = text.split(",");
        if (parts.length != 6) {
            throw new IllegalArgumentException("aabb_ops: 盒子应为 6 个分量: " + text);
        }
        return new AABB(
                Double.parseDouble(parts[0]),
                Double.parseDouble(parts[1]),
                Double.parseDouble(parts[2]),
                Double.parseDouble(parts[3]),
                Double.parseDouble(parts[4]),
                Double.parseDouble(parts[5]));
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
        String[] parts = unit.split("\\|");
        if (parts.length < 2) {
            throw new IllegalArgumentException("aabb_ops: 单元至少需要「盒子|模式」: " + unit);
        }

        AABB box = parseBox(parts[0]);
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        // 双盒子形式：`<box>|<box2>|<op>`，模式在最后一段。
        if (parts.length == 3) {
            AABB other = parseBox(parts[1]);
            if (parts[2].equals("intersect")) {
                result.add("result", boxToJson(box.intersect(other)));
                return result;
            }
            if (parts[2].equals("minmax")) {
                result.add("result", boxToJson(box.minmax(other)));
                return result;
            }
            if (parts[2].equals("intersects")) {
                result.addProperty("hit", box.intersects(other));
                return result;
            }
        }
        if (parts.length == 5) {
            double dx = Double.parseDouble(parts[2]);
            double dy = Double.parseDouble(parts[3]);
            double dz = Double.parseDouble(parts[4]);
            if (parts[1].equals("inflate")) {
                result.add("result", boxToJson(box.inflate(dx, dy, dz)));
                return result;
            }
            if (parts[1].equals("expand")) {
                result.add("result", boxToJson(box.expandTowards(dx, dy, dz)));
                return result;
            }
            if (parts[1].equals("move")) {
                result.add("result", boxToJson(box.move(dx, dy, dz)));
                return result;
            }
        }

        throw new IllegalArgumentException("aabb_ops: 无法识别的单元: " + unit);
    }

    private static String formatNumber(double value) {
        String text = String.format("%.6g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}