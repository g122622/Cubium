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
import net.minecraft.world.level.LevelHeightAccessor;

/**
 * {@code LevelHeightAccessor} 高度范围与原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.LevelHeightOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>用 {@link LevelHeightAccessor#create(int, int)} 构造实例，直接调用接口的 `default` 方法：
 * {@code getMaxY} / {@code getMinSectionY} / {@code getMaxSectionY} / {@code getSectionsCount} /
 * {@code isInsideBuildHeight} / {@code isOutsideBuildHeight} / {@code getSectionIndex} /
 * {@code getSectionIndexFromSectionY} / {@code getSectionYFromSectionIndex}。
 *
 * <p>本能力不需要注册表，因此不调用 `DataPackRegistries.load()`——启动快很多。
 *
 * <p>单元表必须与 probes/level_height.cpp 逐项一致。
 */
public final class LevelHeightOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "range|-64|384",
                    "range|0|256",
                    "range|0|256",
                    "range|0|16",
                    "range|-16|16",
                    "range|-16|32",
                    "range|0|17",
                    "range|5|17",
                    "range|-5|100",
                    "inside|-64|384|-64",
                    "inside|-64|384|319",
                    "inside|-64|384|-65",
                    "inside|-64|384|320",
                    "inside|-64|384|0",
                    "inside|-64|384|63",
                    "inside|-64|384|64",
                    "outside|-64|384|-64",
                    "outside|-64|384|319",
                    "outside|-64|384|-65",
                    "outside|-64|384|320",
                    "both|-64|384|-64",
                    "both|-64|384|-65",
                    "both|-64|384|0",
                    "both|0|256|256",
                    "both|0|256|255",
                    "both|-5|17|-6",
                    "both|-5|17|11",
                    "sectionIndex|-64|384|-64",
                    "sectionIndex|-64|384|-1",
                    "sectionIndex|-64|384|0",
                    "sectionIndex|-64|384|15",
                    "sectionIndex|-64|384|16",
                    "sectionIndex|-64|384|319",
                    "sectionIndex|0|256|0",
                    "sectionIndex|0|256|-1",
                    "sectionYToIndex|-64|384|-4",
                    "sectionYToIndex|-64|384|0",
                    "sectionYToIndex|-64|384|19",
                    "sectionYToIndex|0|256|0",
                    "indexToSectionY|-64|384|0",
                    "indexToSectionY|-64|384|23",
                    "indexToSectionY|0|256|0");

    private LevelHeightOracle() {}

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
                "usage: LevelHeightOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 3) {
            throw new IllegalArgumentException("level_height: 单元至少三段: " + unit);
        }

        int minY = Integer.parseInt(parts[1]);
        int height = Integer.parseInt(parts[2]);
        LevelHeightAccessor accessor = LevelHeightAccessor.create(minY, height);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "range" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("level_height: range 需要 2 个参数: " + unit);
                }
                out.addProperty("value",
                        accessor.getMaxY() + "," + accessor.getMinSectionY() + ","
                                + accessor.getMaxSectionY() + "," + accessor.getSectionsCount());
            }
            case "inside" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException("level_height: inside 需要 3 个参数: " + unit);
                }
                out.addProperty("value",
                        accessor.isInsideBuildHeight(Integer.parseInt(parts[3])) ? "true" : "false");
            }
            case "outside" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException("level_height: outside 需要 3 个参数: " + unit);
                }
                out.addProperty("value",
                        accessor.isOutsideBuildHeight(Integer.parseInt(parts[3])) ? "true" : "false");
            }
            case "both" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException("level_height: both 需要 3 个参数: " + unit);
                }
                int y = Integer.parseInt(parts[3]);
                out.addProperty("value",
                        "inside=" + (accessor.isInsideBuildHeight(y) ? "true" : "false")
                                + ",outside=" + (accessor.isOutsideBuildHeight(y) ? "true" : "false"));
            }
            case "sectionIndex" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException(
                            "level_height: sectionIndex 需要 3 个参数: " + unit);
                }
                out.addProperty("value", accessor.getSectionIndex(Integer.parseInt(parts[3])));
            }
            case "sectionYToIndex" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException(
                            "level_height: sectionYToIndex 需要 3 个参数: " + unit);
                }
                out.addProperty("value",
                        accessor.getSectionIndexFromSectionY(Integer.parseInt(parts[3])));
            }
            case "indexToSectionY" -> {
                if (parts.length != 4) {
                    throw new IllegalArgumentException(
                            "level_height: indexToSectionY 需要 3 个参数: " + unit);
                }
                out.addProperty("value",
                        accessor.getSectionYFromSectionIndex(Integer.parseInt(parts[3])));
            }
            default -> throw new IllegalArgumentException("level_height: 无法识别的单元: " + unit);
        }

        return out;
    }
}