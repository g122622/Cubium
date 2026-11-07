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
 * Vec3 的旋转与投影的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.Vec3RotateOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 单元表必须与 probes/vec3_rotate.cpp 逐项一致。
 */
public final class Vec3RotateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "xrot|0,1,0|0",
                    "xrot|0,1,0|1.5707963267948966",
                    "xrot|0,1,0|3.141592653589793",
                    "xrot|0,1,0|-1.5707963267948966",
                    "xrot|0,0,1|1.5707963267948966",
                    "xrot|1,2,3|0.7853981633974483",
                    "xrot|1,2,3|0",
                    "yrot|1,0,0|0",
                    "yrot|1,0,0|1.5707963267948966",
                    "yrot|1,0,0|3.141592653589793",
                    "yrot|1,0,0|-1.5707963267948966",
                    "yrot|0,0,1|1.5707963267948966",
                    "yrot|1,2,3|0.7853981633974483",
                    "zrot|1,0,0|0",
                    "zrot|1,0,0|1.5707963267948966",
                    "zrot|1,0,0|3.141592653589793",
                    "zrot|1,0,0|-1.5707963267948966",
                    "zrot|0,1,0|1.5707963267948966",
                    "zrot|1,2,3|0.7853981633974483",
                    "proj|1,2,3|1,0,0",
                    "proj|1,2,3|0,1,0",
                    "proj|1,2,3|1,1,1",
                    "proj|1,2,3|0,0,0",
                    "proj|0,0,0|1,2,3",
                    "proj|-1,-2,-3|1,0,0",
                    "horiz|1,2,3",
                    "horiz|0,5,0",
                    "horiz|-1,0,-3");

    private Vec3RotateOracle() {}

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
                "usage: Vec3RotateOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        String[] parts = text.split(",", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException("vec3_rotate: expected 3 components: " + text);
        }
        return new Vec3(Double.parseDouble(parts[0]), Double.parseDouble(parts[1]), Double.parseDouble(parts[2]));
    }

    private static String formatDouble(double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "Infinity" : "-Infinity";
        }
        String text = String.format(java.util.Locale.ROOT, "%.9g", value);
        int exponentIndex = text.indexOf('e');
        String mantissa = exponentIndex >= 0 ? text.substring(0, exponentIndex) : text;
        String exponent = exponentIndex >= 0 ? text.substring(exponentIndex) : "";
        if (mantissa.indexOf('.') >= 0) {
            int end = mantissa.length();
            while (end > 0 && mantissa.charAt(end - 1) == '0') {
                end--;
            }
            if (end > 0 && mantissa.charAt(end - 1) == '.') {
                end--;
            }
            mantissa = mantissa.substring(0, end);
        }
        return mantissa + exponent;
    }

    private static JsonArray vecToJson(Vec3 vec) {
        JsonArray array = new JsonArray();
        array.add(formatDouble(vec.x));
        array.add(formatDouble(vec.y));
        array.add(formatDouble(vec.z));
        return array;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("vec3_rotate: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String mode = parts[0];
        Vec3 vec = parseVec(parts[1]);

        switch (mode) {
            case "xrot" -> {
                out.add("result", vecToJson(vec.xRot((float) Double.parseDouble(parts[2]))));
                return out;
            }
            case "yrot" -> {
                out.add("result", vecToJson(vec.yRot((float) Double.parseDouble(parts[2]))));
                return out;
            }
            case "zrot" -> {
                out.add("result", vecToJson(vec.zRot((float) Double.parseDouble(parts[2]))));
                return out;
            }
            case "proj" -> {
                out.add("result", vecToJson(vec.projectedOn(parseVec(parts[2]))));
                return out;
            }
            case "horiz" -> {
                out.add("result", vecToJson(vec.horizontal()));
                return out;
            }
            default -> throw new IllegalArgumentException("vec3_rotate: unrecognized unit: " + unit);
        }
    }
}