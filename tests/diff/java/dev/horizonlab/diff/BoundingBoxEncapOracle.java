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
import java.util.Optional;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Vec3i;
import net.minecraft.world.level.levelgen.structure.BoundingBox;

/**
 * {@code BoundingBox} 封装族与构造函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BoundingBoxEncapOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版构造器与 {@link BoundingBox#fromCorners(Vec3i, Vec3i)} /
 * {@link BoundingBox#infinite()} / {@link BoundingBox#encapsulating(BoundingBox, BoundingBox)} /
 * {@link BoundingBox#encapsulatingPositions(Iterable)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>空集合输出 `"&lt;empty&gt;"`；整数用 `minint` / `maxint` 表示极值。
 *
 * <p>单元表必须与 probes/bounding_box_encap.cpp 逐项一致。
 */
public final class BoundingBoxEncapOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 空集合标记。 */
    private static final String EMPTY_MARK = "<empty>";

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "ctor|0|0|0|1|1|1",
                    "ctor|5|5|5|0|0|0",
                    "ctor|0|0|0|0|0|0",
                    "ctor|-5|-5|-5|5|5|5",
                    "ctor|0|0|0|1|1|0",
                    "ctor|1|0|0|0|1|1",
                    "fromCorners|0|0|0|3|2|4",
                    "fromCorners|3|2|4|0|0|0",
                    "fromCorners|-1|-2|-3|1|2|3",
                    "infinite",
                    "encapBox|0|0|0|1|1|1|2|2|2|3|3|3",
                    "encapBox|0|0|0|1|1|1|0|0|0|1|1|1",
                    "encapBox|-5|-5|-5|0|0|0|3|3|3|8|8|8",
                    "encapPos|0,0,0",
                    "encapPos|0,0,0;3,2,4",
                    "encapPos|3,2,4;0,0,0",
                    "encapPos|-1,-2,-3;1,2,3;0,0,0",
                    "encapPos|-");

    private BoundingBoxEncapOracle() {}

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
                "usage: BoundingBoxEncapOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 解析整数，支持 `minint` / `maxint`。 */
    private static int parseInt(String text) {
        return switch (text) {
            case "minint" -> Integer.MIN_VALUE;
            case "maxint" -> Integer.MAX_VALUE;
            default -> Integer.parseInt(text);
        };
    }

    /** 渲染盒子为 `minX,minY,minZ,maxX,maxY,maxZ`。 */
    private static String render(BoundingBox box) {
        return box.minX() + "," + box.minY() + "," + box.minZ() + ","
                + box.maxX() + "," + box.maxY() + "," + box.maxZ();
    }

    /** 拆分 `x,y,z;x,y,z` 形式的位置列表。 */
    private static List<BlockPos> parsePositions(String text) {
        List<BlockPos> points = new ArrayList<>();
        if (text.equals("-")) {
            return points;
        }
        for (String part : text.split(";", -1)) {
            String[] numbers = part.split(",", -1);
            if (numbers.length != 3) {
                throw new IllegalArgumentException("bounding_box_encap: 位置需要三坐标: " + part);
            }
            points.add(new BlockPos(
                    (int) Long.parseLong(numbers[0]), (int) Long.parseLong(numbers[1]),
                    (int) Long.parseLong(numbers[2])));
        }
        return points;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "ctor" -> out.addProperty("value", render(new BoundingBox(
                    parseInt(parts[1]), parseInt(parts[2]), parseInt(parts[3]),
                    parseInt(parts[4]), parseInt(parts[5]), parseInt(parts[6]))));
            case "fromCorners" -> out.addProperty("value", render(BoundingBox.fromCorners(
                    new Vec3i(parseInt(parts[1]), parseInt(parts[2]), parseInt(parts[3])),
                    new Vec3i(parseInt(parts[4]), parseInt(parts[5]), parseInt(parts[6])))));
            case "infinite" -> out.addProperty("value", render(BoundingBox.infinite()));
            case "encapBox" -> out.addProperty("value", render(BoundingBox.encapsulating(
                    new BoundingBox(parseInt(parts[1]), parseInt(parts[2]), parseInt(parts[3]),
                            parseInt(parts[4]), parseInt(parts[5]), parseInt(parts[6])),
                    new BoundingBox(parseInt(parts[7]), parseInt(parts[8]), parseInt(parts[9]),
                            parseInt(parts[10]), parseInt(parts[11]), parseInt(parts[12])))));
            case "encapPos" -> {
                Optional<BoundingBox> box =
                        BoundingBox.encapsulatingPositions(parsePositions(parts[1]));
                out.addProperty("value", box.map(BoundingBoxEncapOracle::render)
                        .orElse(EMPTY_MARK));
            }
            default -> throw new IllegalArgumentException(
                    "bounding_box_encap: 无法识别的单元: " + unit);
        }

        return out;
    }
}