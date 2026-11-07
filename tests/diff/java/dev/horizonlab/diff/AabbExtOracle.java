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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * AABB 的中心点、角点与向上包含的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AabbExtOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 单元表必须与 probes/aabb_ext.cpp 逐项一致。
 */
public final class AabbExtOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "center|0,0,0,1,1,1",
                    "bottomcenter|0,0,0,1,1,1",
                    "minpos|0,0,0,1,1,1",
                    "maxpos|0,0,0,1,1,1",
                    "center|1,2,3,5,8,11",
                    "bottomcenter|1,2,3,5,8,11",
                    "minpos|1,2,3,5,8,11",
                    "maxpos|1,2,3,5,8,11",
                    "center|-5,-8,-11,-1,-2,-3",
                    "bottomcenter|-5,-8,-11,-1,-2,-3",
                    "minpos|-5,-8,-11,-1,-2,-3",
                    "maxpos|-5,-8,-11,-1,-2,-3",
                    "center|2,2,2,2,2,2",
                    "bottomcenter|2,2,2,2,2,2",
                    "center|0,5,0,1,5,1",
                    "bottomcenter|0,5,0,1,5,1");

    private AabbExtOracle() {}

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
                "usage: AabbExtOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static double[] parseNumbers(String text, int expected) {
        String[] parts = text.split(",", -1);
        if (parts.length != expected) {
            throw new IllegalArgumentException("aabb_ext: expected " + expected + " numbers: " + text);
        }
        double[] values = new double[expected];
        for (int i = 0; i < expected; i++) {
            values[i] = Double.parseDouble(parts[i]);
        }
        return values;
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

    private static String formatVector(Vec3 vec) {
        return formatDouble(vec.x) + "," + formatDouble(vec.y) + "," + formatDouble(vec.z);
    }

    private static String formatBox(AABB box) {
        return formatDouble(box.minX) + "," + formatDouble(box.minY) + "," + formatDouble(box.minZ) + ","
                + formatDouble(box.maxX) + "," + formatDouble(box.maxY) + "," + formatDouble(box.maxZ);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("aabb_ext: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String mode = parts[0];
        double[] v = parseNumbers(parts[1], 6);
        AABB box = new AABB(v[0], v[1], v[2], v[3], v[4], v[5]);

        switch (mode) {
            case "center" -> {
                out.addProperty("value", formatVector(box.getCenter()));
                return out;
            }
            case "bottomcenter" -> {
                out.addProperty("value", formatVector(box.getBottomCenter()));
                return out;
            }
            case "minpos" -> {
                out.addProperty("value", formatVector(box.getMinPosition()));
                return out;
            }
            case "maxpos" -> {
                out.addProperty("value", formatVector(box.getMaxPosition()));
                return out;
            }
            default -> throw new IllegalArgumentException("aabb_ext: unrecognized unit: " + unit);
        }
    }
}