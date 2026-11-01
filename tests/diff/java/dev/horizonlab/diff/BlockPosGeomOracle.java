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
import net.minecraft.core.BlockPos;
import net.minecraft.core.Vec3i;
import net.minecraft.world.level.block.Rotation;

/**
 * {@code BlockPos} 几何变换的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPosGeomOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BlockPos#rotate(Rotation)} / {@link BlockPos#cross(Vec3i)} /
 * {@link BlockPos#containing(double, double, double)} / {@link BlockPos#min} /
 * {@link BlockPos#max} / {@link BlockPos#multiply(int)} / {@link BlockPos#subtract(Vec3i)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/blockpos_geom.cpp 逐项一致。
 */
public final class BlockPosGeomOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "rotate|NONE|5,3,7",
                    "rotate|CLOCKWISE_90|5,3,7",
                    "rotate|CLOCKWISE_180|5,3,7",
                    "rotate|COUNTERCLOCKWISE_90|5,3,7",
                    "rotate|CLOCKWISE_90|-5,3,7",
                    "rotate|CLOCKWISE_90|5,3,-7",
                    "rotate|CLOCKWISE_90|0,0,0",
                    "rotate|COUNTERCLOCKWISE_90|1,2,3",
                    "rotate|CLOCKWISE_90|0,99,0",
                    "rotate|CLOCKWISE_180|0,-99,0",
                    "cross|1,0,0|0,1,0",
                    "cross|1,0,0|0,0,1",
                    "cross|0,1,0|0,0,1",
                    "cross|1,2,3|1,2,3",
                    "cross|2,3,4|5,6,7",
                    "cross|0,0,0|1,2,3",
                    "containing|1.9,2.1,3.5",
                    "containing|-0.5,-1.5,-0.1",
                    "containing|0.0,0.0,0.0",
                    "containing|-0.0,-0.0,-0.0",
                    "containing|5.999,6.001,-3.5",
                    "containing|-1.0,-2.0,-3.0",
                    "min|5,3,7|2,8,4",
                    "max|5,3,7|2,8,4",
                    "min|1,1,1|1,1,1",
                    "max|-5,-3,-7|-2,-8,-4",
                    "multiply|5,3,7|0",
                    "multiply|5,3,7|1",
                    "multiply|5,3,7|2",
                    "multiply|5,3,7|-1",
                    "multiply|-5,3,-7|-3",
                    "subtract|5,3,7|2,8,4",
                    "subtract|5,3,7|5,3,7",
                    "subtract|0,0,0|1,2,3");

    private BlockPosGeomOracle() {}

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
                "usage: BlockPosGeomOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static BlockPos parsePos(String text) {
        String[] parts = text.split(",", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException("blockpos_geom: 坐标必须三段: " + text);
        }
        return new BlockPos(
                Integer.parseInt(parts[0]), Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
    }

    private static String dumpPos(BlockPos pos) {
        return pos.getX() + "," + pos.getY() + "," + pos.getZ();
    }

    private static Rotation findRotation(String name) {
        for (Rotation rot : Rotation.values()) {
            if (rot.name().equals(name)) {
                return rot;
            }
        }
        throw new IllegalArgumentException("blockpos_geom: 未知旋转: " + name);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("blockpos_geom: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "rotate" -> out.addProperty("value",
                    dumpPos(parsePos(parts[2]).rotate(findRotation(parts[1]))));
            case "cross" -> out.addProperty("value",
                    dumpPos(parsePos(parts[1]).cross(parsePos(parts[2]))));
            case "containing" -> {
                String[] coords = parts[1].split(",", -1);
                if (coords.length != 3) {
                    throw new IllegalArgumentException(
                            "blockpos_geom: 坐标必须三段: " + parts[1]);
                }
                out.addProperty("value", dumpPos(BlockPos.containing(
                        Double.parseDouble(coords[0]),
                        Double.parseDouble(coords[1]),
                        Double.parseDouble(coords[2]))));
            }
            case "min" -> out.addProperty("value",
                    dumpPos(BlockPos.min(parsePos(parts[1]), parsePos(parts[2]))));
            case "max" -> out.addProperty("value",
                    dumpPos(BlockPos.max(parsePos(parts[1]), parsePos(parts[2]))));
            case "multiply" -> out.addProperty("value",
                    dumpPos(parsePos(parts[1]).multiply(Integer.parseInt(parts[2]))));
            case "subtract" -> out.addProperty("value",
                    dumpPos(parsePos(parts[1]).subtract(parsePos(parts[2]))));
            default -> throw new IllegalArgumentException("blockpos_geom: 无法识别的单元: " + unit);
        }

        return out;
    }
}