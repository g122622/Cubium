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
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * VoxelShape 查询类接口的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShapeQueryOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 覆盖 getCoords / findIndex / isCubeLike / isCubeLikeAlong / toAabbs。
 *
 * 单元表必须与 probes/shape_query.cpp 逐项一致。
 */
public final class ShapeQueryOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "coords|0,0,0,1,1,1|x",
                    "coords|0,0,0,1,1,1|y",
                    "coords|0,0,0,1,1,1|z",
                    "coords|0.25,0,0.25,0.75,0.5,0.75|x",
                    "coords|0.25,0,0.25,0.75,0.5,0.75|y",
                    "coords|0.25,0,0.25,0.75,0.5,0.75|z",
                    "coords|-1,-2,-3,4,5,6|x",
                    "iscube|0,0,0,1,1,1|x",
                    "iscube|0.25,0,0.25,0.75,0.5,0.75|x",
                    "iscube|0,0,0,1,0.5,1|x",
                    "iscube|0,0,0,0.5,1,1|z",
                    "iscube|0,0,0,1,1,1|all",
                    "iscube|0.25,0,0.25,0.75,0.5,0.75|all",
                    "iscube|0,0,0,1,0.5,1|all",
                    "findindex|0,0,0,1,1,1|x|0.5",
                    "findindex|0,0,0,1,1,1|x|0",
                    "findindex|0,0,0,1,1,1|x|1",
                    "findindex|0,0,0,1,1,1|x|-0.5",
                    "findindex|0,0,0,1,1,1|x|1.5",
                    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0",
                    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0.25",
                    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0.5",
                    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0.75",
                    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|1",
                    "findindex|0.25,0,0.25,0.75,0.5,0.75|y|0.5",
                    "toaabbs|0,0,0,1,1,1",
                    "toaabbs|0.0625,0,0.0625,0.9375,0.09375,0.9375",
                    "toaabbs|0.25,0,0.25,0.75,0.5,0.75",
                    "toaabbs|-1,-2,-3,4,5,6");

    private ShapeQueryOracle() {}

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
                "usage: ShapeQueryOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /**
     * 原版把 isCubeLike / isCubeLikeAlong / findIndex 声明为 protected，
     * 外部无法直接调用，这里用反射绕过访问控制。
     *
     * 本项目把这三个方法放成了 public——这是可见性差异，不是行为差异；
     * 测试的目的是比行为，因此这里刻意越过访问控制。
     */
    private static Object invokeProtected(VoxelShape shape, String name, Class<?>[] types, Object[] args) {
        try {
            var method = VoxelShape.class.getDeclaredMethod(name, types);
            method.setAccessible(true);
            return method.invoke(shape, args);
        } catch (ReflectiveOperationException ex) {
            throw new IllegalStateException("shape_query: reflection failed for " + name, ex);
        }
    }

    private static double[] parseNumbers(String text, int expected, String unit) {
        String[] parts = text.split(",", -1);
        if (parts.length != expected) {
            throw new IllegalArgumentException(
                    "shape_query: expected " + expected + " numbers: " + unit);
        }
        double[] values = new double[expected];
        for (int i = 0; i < expected; i++) {
            values[i] = Double.parseDouble(parts[i]);
        }
        return values;
    }

    private static net.minecraft.core.Direction.Axis parseAxis(String text) {
        return switch (text) {
            case "x" -> net.minecraft.core.Direction.Axis.X;
            case "y" -> net.minecraft.core.Direction.Axis.Y;
            case "z" -> net.minecraft.core.Direction.Axis.Z;
            default -> throw new IllegalArgumentException("shape_query: unknown axis: " + text);
        };
    }

    private static String formatCoordinate(double value) {
        return String.format(Locale.ROOT, "%.6f", value);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("shape_query: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String mode = parts[0];
        double[] box = parseNumbers(parts[1], 6, unit);
        VoxelShape shape = Shapes.box(box[0], box[1], box[2], box[3], box[4], box[5]);

        if (mode.equals("coords")) {
            if (parts.length != 3) {
                throw new IllegalArgumentException("shape_query: coords needs axis: " + unit);
            }
            var coords = shape.getCoords(parseAxis(parts[2]));
            StringBuilder joined = new StringBuilder();
            for (int i = 0; i < coords.size(); i++) {
                if (i > 0) {
                    joined.append(',');
                }
                joined.append(formatCoordinate(coords.getDouble(i)));
            }
            out.addProperty("value", joined.toString());
            return out;
        }

        if (mode.equals("iscube")) {
            if (parts.length != 3) {
                throw new IllegalArgumentException("shape_query: iscube needs axis: " + unit);
            }
            if (parts[2].equals("all")) {
                out.addProperty("value", (boolean) invokeProtected(shape, "isCubeLike", new Class<?>[] {}, new Object[] {}));
            } else {
                out.addProperty("value", (boolean) invokeProtected(shape, "isCubeLikeAlong",
                        new Class<?>[] {net.minecraft.core.Direction.Axis.class},
                        new Object[] {parseAxis(parts[2])}));
            }
            return out;
        }

        if (mode.equals("findindex")) {
            if (parts.length != 4) {
                throw new IllegalArgumentException("shape_query: findindex needs axis and coord: " + unit);
            }
            out.addProperty("value", (int) invokeProtected(shape, "findIndex",
                    new Class<?>[] {net.minecraft.core.Direction.Axis.class, double.class},
                    new Object[] {parseAxis(parts[2]), Double.parseDouble(parts[3])}));
            return out;
        }

        if (mode.equals("toaabbs")) {
            if (parts.length != 2) {
                throw new IllegalArgumentException("shape_query: toaabbs needs one box: " + unit);
            }
            List<AABB> boxes = shape.toAabbs();
            StringBuilder joined = new StringBuilder();
            for (int i = 0; i < boxes.size(); i++) {
                if (i > 0) {
                    joined.append(';');
                }
                AABB bb = boxes.get(i);
                joined.append(formatCoordinate(bb.minX)).append(',').append(formatCoordinate(bb.minY)).append(',')
                        .append(formatCoordinate(bb.minZ)).append(',').append(formatCoordinate(bb.maxX)).append(',')
                        .append(formatCoordinate(bb.maxY)).append(',').append(formatCoordinate(bb.maxZ));
            }
            out.addProperty("value", joined.toString());
            return out;
        }

        throw new IllegalArgumentException("shape_query: unrecognized unit: " + unit);
    }
}