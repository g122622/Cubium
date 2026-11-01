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
import net.minecraft.world.level.levelgen.structure.BoundingBox;

/**
 * {@code BoundingBox} 相交/包含/合并的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BoundingBoxOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BoundingBox} 的构造器与
 * {@code intersects} / {@code isInside} / {@code encapsulating} / {@code move} /
 * {@code fromCorners}，不重写判定逻辑。
 *
 * <p>单元表必须与 probes/bounding_box.cpp 逐项一致。
 */
public final class BoundingBoxOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "inter|0,0,0,1,1,1|2,0,0,3,1,1",
                    "inter|0,0,0,1,1,1|0,2,0,1,3,1",
                    "inter|0,0,0,1,1,1|0,0,2,1,1,3",
                    "inter|0,0,0,1,1,1|1,0,0,2,1,1",
                    "inter|0,0,0,1,1,1|0,1,0,1,2,1",
                    "inter|0,0,0,1,1,1|0,0,1,1,1,2",
                    "inter|0,0,0,10,10,10|2,2,2,3,3,3",
                    "inter|2,2,2,3,3,3|0,0,0,10,10,10",
                    "inter|0,0,0,5,5,5|0,0,0,5,5,5",
                    "inter|0,0,0,5,5,5|4,10,10,9,15,15",
                    "inside|0,0,0,5,5,5|2|2|2",
                    "inside|0,0,0,5,5,5|0|0|0",
                    "inside|0,0,0,5,5,5|5|5|5",
                    "inside|0,0,0,5,5,5|6|2|2",
                    "inside|0,0,0,5,5,5|2|-1|2",
                    "inside|0,0,0,5,5,5|2|2|6",
                    "encap|0,0,0,1,1,1|5,5,5,6,6,6",
                    "encap|0,0,0,10,10,10|2,2,2,3,3,3",
                    "encap|0,0,0,5,5,5|0,0,0,5,5,5",
                    "move|0,0,0,5,5,5|1|2|3",
                    "move|0,0,0,5,5,5|0|0|0",
                    "move|0,0,0,5,5,5|-10|-10|-10",
                    "corners|0|0|0|5|5|5",
                    "corners|5|5|5|0|0|0",
                    "corners|5|0|3|0|5|1",
                    "ctor|5|0|0|0|0|0",
                    "ctor|0|0|0|5|0|0",
                    "ctor|5|5|5|0|0|0",
                    "ctor|1|2|3|4|5|6");

    private BoundingBoxOracle() {}

    public static void main(String[] args) throws IOException {
        DataPackRegistries.load();
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
                "usage: BoundingBoxOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static BoundingBox parseBox(String text) {
        String[] parts = text.split(",", -1);
        if (parts.length != 6) {
            throw new IllegalArgumentException("bounding_box: 盒子应为 6 个整数: " + text);
        }
        return new BoundingBox(
                Integer.parseInt(parts[0]),
                Integer.parseInt(parts[1]),
                Integer.parseInt(parts[2]),
                Integer.parseInt(parts[3]),
                Integer.parseInt(parts[4]),
                Integer.parseInt(parts[5]));
    }

    private static String boxToString(BoundingBox box) {
        return box.minX() + "," + box.minY() + "," + box.minZ() + ","
                + box.maxX() + "," + box.maxY() + "," + box.maxZ();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 3) {
            throw new IllegalArgumentException("bounding_box: 单元至少三段: " + unit);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        switch (parts[0]) {
            case "inter" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("bounding_box: inter 需要 2 个盒子: " + unit);
                }
                result.addProperty(
                        "value",
                        parseBox(parts[1]).intersects(parseBox(parts[2])) ? "true" : "false");
            }
            case "inside" -> {
                if (parts.length != 5) {
                    throw new IllegalArgumentException("bounding_box: inside 需要 4 个参数: " + unit);
                }
                result.addProperty(
                        "value",
                        parseBox(parts[1]).isInside(
                                Integer.parseInt(parts[2]),
                                Integer.parseInt(parts[3]),
                                Integer.parseInt(parts[4]))
                                ? "true" : "false");
            }
            case "encap" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("bounding_box: encap 需要 2 个盒子: " + unit);
                }
                result.addProperty(
                        "value",
                        boxToString(BoundingBox.encapsulating(
                                parseBox(parts[1]), parseBox(parts[2]))));
            }
            case "move" -> {
                if (parts.length != 5) {
                    throw new IllegalArgumentException("bounding_box: move 需要 4 个参数: " + unit);
                }
                BoundingBox box = parseBox(parts[1]);
                box.move(
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]),
                        Integer.parseInt(parts[4]));
                result.addProperty("value", boxToString(box));
            }
            case "corners" -> {
                if (parts.length != 7) {
                    throw new IllegalArgumentException("bounding_box: corners 需要 6 个参数: " + unit);
                }
                BlockPos a = new BlockPos(
                        Integer.parseInt(parts[1]),
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]));
                BlockPos b = new BlockPos(
                        Integer.parseInt(parts[4]),
                        Integer.parseInt(parts[5]),
                        Integer.parseInt(parts[6]));
                result.addProperty("value", boxToString(BoundingBox.fromCorners(a, b)));
            }
            case "ctor" -> {
                if (parts.length != 7) {
                    throw new IllegalArgumentException("bounding_box: ctor 需要 6 个参数: " + unit);
                }
                result.addProperty("value", boxToString(new BoundingBox(
                        Integer.parseInt(parts[1]),
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]),
                        Integer.parseInt(parts[4]),
                        Integer.parseInt(parts[5]),
                        Integer.parseInt(parts[6]))));
            }
            default -> throw new IllegalArgumentException("bounding_box: 无法识别的单元: " + unit);
        }

        return result;
    }
}