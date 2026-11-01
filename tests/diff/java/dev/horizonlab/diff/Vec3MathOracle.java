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
import net.minecraft.world.phys.Vec3;

/**
 * {@code Vec3} 向量运算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.Vec3MathOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Vec3#normalize()} / {@link Vec3#dot(Vec3)} /
 * {@link Vec3#cross(Vec3)} / {@link Vec3#scale(double)} / {@link Vec3#length()} /
 * {@link Vec3#lengthSqr()} / {@link Vec3#horizontalDistance()}，不重写公式。
 *
 * <p>单元表必须与 probes/vec3_math.cpp 逐项一致。
 */
public final class Vec3MathOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "normalize|3,4,0",
                    "normalize|1,0,0",
                    "normalize|1,1,1",
                    "normalize|0,0,0",
                    "normalize|1e-06,0,0",
                    "normalize|9.9e-06,0,0",
                    "normalize|1e-05,0,0",
                    "normalize|1.1e-05,0,0",
                    "normalize|5e-06,0,0",
                    "normalize|-3,-4,0",
                    "dot|1,2,3|4,5,6",
                    "dot|0,0,0|1,1,1",
                    "dot|-1,-2,-3|4,5,6",
                    "dot|1,0,0|0,1,0",
                    "cross|1,0,0|0,1,0",
                    "cross|1,2,3|4,5,6",
                    "cross|0,0,0|1,2,3",
                    "cross|1,1,1|1,1,1",
                    "length|3,4,0",
                    "length|1,1,1",
                    "length|0,0,0",
                    "lengthsqr|3,4,0",
                    "lengthsqr|1,1,1",
                    "hdist|3,4,0",
                    "hdist|3,100,4",
                    "hdist|0,5,0",
                    "hdistsqr|3,100,4",
                    "scale|1,2,3|2",
                    "scale|1,2,3|0",
                    "scale|1,2,3|-1",
                    "scale|1,2,3|0.5");

    private Vec3MathOracle() {}

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
                "usage: Vec3MathOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Vec3 parseVec(String text) {
        String[] parts = text.split(",");
        if (parts.length != 3) {
            throw new IllegalArgumentException("vec3_math: 向量应为 3 个分量: " + text);
        }
        return new Vec3(
                Double.parseDouble(parts[0]),
                Double.parseDouble(parts[1]),
                Double.parseDouble(parts[2]));
    }

    private static JsonArray vecToJson(Vec3 v) {
        JsonArray array = new JsonArray();
        array.add(formatNumber(v.x));
        array.add(formatNumber(v.y));
        array.add(formatNumber(v.z));
        return array;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length < 2) {
            throw new IllegalArgumentException("vec3_math: 单元至少需要「模式|向量」: " + unit);
        }

        String mode = parts[0];
        Vec3 v = parseVec(parts[1]);
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (mode.equals("normalize") && parts.length == 2) {
            result.add("result", vecToJson(v.normalize()));
            return result;
        }
        if (mode.equals("dot") && parts.length == 3) {
            result.addProperty("value", formatNumber(v.dot(parseVec(parts[2]))));
            return result;
        }
        if (mode.equals("cross") && parts.length == 3) {
            result.add("result", vecToJson(v.cross(parseVec(parts[2]))));
            return result;
        }
        if (mode.equals("length") && parts.length == 2) {
            result.addProperty("value", formatNumber(v.length()));
            return result;
        }
        if (mode.equals("lengthsqr") && parts.length == 2) {
            result.addProperty("value", formatNumber(v.lengthSqr()));
            return result;
        }
        if (mode.equals("hdist") && parts.length == 2) {
            result.addProperty("value", formatNumber(v.horizontalDistance()));
            return result;
        }
        if (mode.equals("hdistsqr") && parts.length == 2) {
            result.addProperty("value", formatNumber(v.horizontalDistanceSqr()));
            return result;
        }
        if (mode.equals("scale") && parts.length == 3) {
            result.add("result", vecToJson(v.scale(Double.parseDouble(parts[2]))));
            return result;
        }

        throw new IllegalArgumentException("vec3_math: 无法识别的单元: " + unit);
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