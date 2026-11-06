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
import java.util.Locale;
import java.util.Optional;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * VoxelShape 的最近点与整平包裹盒的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShapeClosestOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 浮点输出保留 6 位小数比对。
 *
 * 单元表必须与 probes/shape_closest.cpp 逐项一致。
 */
public final class ShapeClosestOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "closest|0,0,0,1,1,1|2,0.5,0.5",
                    "closest|0,0,0,1,1,1|-1,0.5,0.5",
                    "closest|0,0,0,1,1,1|0.5,2,0.5",
                    "closest|0,0,0,1,1,1|0.5,0.5,-1",
                    "closest|0,0,0,1,1,1|2,2,2",
                    "closest|0,0,0,1,1,1|-1,-1,-1",
                    "closest|0,0,0,1,1,1|0.5,0.5,0.5",
                    "closest|0,0,0,1,1,1|0.1,0.9,0.5",
                    "closest|0,0,0,1,1,1|0,0.5,0.5",
                    "closest|0,0,0,1,1,1|0.5,1,0.5",
                    "closest|0.0625,0,0.0625,0.9375,0.09375,0.9375|0.5,0.5,0.5",
                    "closest|0.0625,0,0.0625,0.9375,0.09375,0.9375|0.5,-1,0.5",
                    "closest|0.25,0,0.25,0.75,0.5,0.75|0,0.25,0.5",
                    "closest|0.25,0,0.25,0.75,0.5,0.75|0.5,1,0.5",
                    "encap|0,0,0,1,1,1",
                    "encap|0.0625,0,0.0625,0.9375,0.09375,0.9375",
                    "encap|0.25,0,0.25,0.75,0.5,0.75",
                    "encap|0.375,0.25,0.375,0.625,1,0.625",
                    "encap|-1,-2,-3,4,5,6");

    private ShapeClosestOracle() {}

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
                "usage: ShapeClosestOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static double[] parseNumbers(String text, int expected, String unit) {
        String[] parts = text.split(",", -1);
        if (parts.length != expected) {
            throw new IllegalArgumentException(
                    "shape_closest: expected " + expected + " numbers: " + unit);
        }
        double[] values = new double[expected];
        for (int i = 0; i < expected; i++) {
            values[i] = Double.parseDouble(parts[i]);
        }
        return values;
    }

    private static String joinNumbers(double... values) {
        StringBuilder joined = new StringBuilder();
        for (int i = 0; i < values.length; i++) {
            if (i > 0) {
                joined.append(',');
            }
            joined.append(String.format(Locale.ROOT, "%.6f", values[i]));
        }
        return joined.toString();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("shape_closest: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String mode = parts[0];

        if (mode.equals("encap")) {
            if (parts.length != 2) {
                throw new IllegalArgumentException("shape_closest: encap needs one box: " + unit);
            }
            double[] box = parseNumbers(parts[1], 6, unit);
            VoxelShape shape = Shapes.box(box[0], box[1], box[2], box[3], box[4], box[5]);
            VoxelShape encap = shape.singleEncompassing();
            AABB bounds = encap.bounds();
            out.addProperty("value",
                    joinNumbers(bounds.minX, bounds.minY, bounds.minZ, bounds.maxX, bounds.maxY, bounds.maxZ));
            return out;
        }

        if (mode.equals("closest")) {
            if (parts.length != 3) {
                throw new IllegalArgumentException("shape_closest: closest needs box and point: " + unit);
            }
            double[] box = parseNumbers(parts[1], 6, unit);
            double[] point = parseNumbers(parts[2], 3, unit);
            VoxelShape shape = Shapes.box(box[0], box[1], box[2], box[3], box[4], box[5]);
            Optional<Vec3> result = shape.closestPointTo(new Vec3(point[0], point[1], point[2]));
            if (result.isEmpty()) {
                out.addProperty("value", "empty");
            } else {
                Vec3 vec = result.get();
                out.addProperty("value", joinNumbers(vec.x, vec.y, vec.z));
            }
            return out;
        }

        throw new IllegalArgumentException("shape_closest: unrecognized unit: " + unit);
    }
}