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
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.Direction;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * 实体碰撞扫描的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CollideSweepOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>轴序直接调用原版 {@link Direction#axisStepOrder(Vec3)}；位移解算直接调用原版
 * {@link Shapes#collide(Direction.Axis, AABB, Iterable, double)}，不重写公式。
 *
 * <p>单元表必须与 probes/collide_sweep.cpp 逐项一致。
 */
public final class CollideSweepOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "axis|1|0|0",
                    "axis|0|0|1",
                    "axis|1|0|1",
                    "axis|0.5|0|1",
                    "axis|1|0|0.5",
                    "axis|-1|0|0",
                    "axis|0|0|-1",
                    "axis|-1|0|1",
                    "axis|1|0|-1",
                    "axis|0|1|0",
                    "axis|0|0|0",
                    "collide|1|0|0|0,0,0,0.6,1.8,0.6",
                    "collide|1|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2",
                    "collide|0.3|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2",
                    "collide|1|0|0|0,0,0,0.6,1.8,0.6|0.6,0,-1,2,2,2",
                    "collide|0|-1|0|0,0,0,0.6,1.8,0.6|-2,-1,-2,2,0,2",
                    "collide|0|1|0|0,0,0,0.6,1.8,0.6|-2,1.8,-2,2,4,2",
                    "collide|1|0|1|0,0,0,0.6,1.8,0.6|1,0,-2,2,2,2",
                    "collide|1|0|1|0,0,0,0.6,1.8,0.6|-2,0,1,2,2,2",
                    "collide|2|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2|3,0,-1,4,2,2",
                    "collide|1|0|0|0,0,0,0.6,1.8,0.6|1,0,5,2,2,6",
                    "collide|0.0000001|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2",
                    "collide|-1|0|0|0,0,0,0.6,1.8,0.6|-2,0,-1,-1,2,2");

    private CollideSweepOracle() {}

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
                "usage: CollideSweepOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("collide_sweep: 盒子应为 6 个值: " + text);
        }
        return new AABB(
                Double.parseDouble(parts[0]),
                Double.parseDouble(parts[1]),
                Double.parseDouble(parts[2]),
                Double.parseDouble(parts[3]),
                Double.parseDouble(parts[4]),
                Double.parseDouble(parts[5]));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("collide_sweep: 单元至少两段: " + unit);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (parts[0].equals("axis") && parts.length == 4) {
            Vec3 delta = new Vec3(
                    Double.parseDouble(parts[1]),
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]));
            StringBuilder order = new StringBuilder();
            for (Direction.Axis axis : Direction.axisStepOrder(delta)) {
                switch (axis) {
                    case X -> order.append('x');
                    case Y -> order.append('y');
                    case Z -> order.append('z');
                }
            }
            result.addProperty("value", order.toString());
            return result;
        }

        if (parts[0].equals("collide") && parts.length >= 5) {
            double dx = Double.parseDouble(parts[1]);
            double dy = Double.parseDouble(parts[2]);
            double dz = Double.parseDouble(parts[3]);
            AABB start = parseBox(parts[4]);

            List<VoxelShape> shapes = new ArrayList<>();
            for (int i = 5; i < parts.length; i++) {
                shapes.add(Shapes.create(parseBox(parts[i])));
            }

            // 复刻 Entity.collideWithShapes 的逐轴推进。
            Vec3 delta = new Vec3(dx, dy, dz);
            AABB current = start;
            for (Direction.Axis axis : Direction.axisStepOrder(delta)) {
                double d = switch (axis) {
                    case X -> dx;
                    case Y -> dy;
                    case Z -> dz;
                };
                if (d == 0.0) {
                    continue;
                }
                double moved = Shapes.collide(axis, current, shapes, d);
                current = switch (axis) {
                    case X -> current.move(moved, 0.0, 0.0);
                    case Y -> current.move(0.0, moved, 0.0);
                    case Z -> current.move(0.0, 0.0, moved);
                };
            }

            result.addProperty("dx", formatNumber(current.minX - start.minX));
            result.addProperty("dy", formatNumber(current.minY - start.minY));
            result.addProperty("dz", formatNumber(current.minZ - start.minZ));
            result.addProperty("maxX", formatNumber(current.maxX));
            result.addProperty("maxY", formatNumber(current.maxY));
            result.addProperty("maxZ", formatNumber(current.maxZ));
            return result;
        }

        throw new IllegalArgumentException("collide_sweep: 无法识别的单元: " + unit);
    }

    private static String formatNumber(double value) {
        String text = String.format("%.9g", value);
        int expPos = -1;
        for (int i = 0; i < text.length(); i++) {
            char ch = text.charAt(i);
            if (ch == 'e' || ch == 'E') {
                expPos = i;
                break;
            }
        }
        String mantissa = expPos < 0 ? text : text.substring(0, expPos);
        String suffix = expPos < 0 ? "" : text.substring(expPos);
        if (mantissa.indexOf('.') >= 0) {
            while (mantissa.endsWith("0")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
            if (mantissa.endsWith(".")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
        }
        return mantissa + suffix;
    }
}