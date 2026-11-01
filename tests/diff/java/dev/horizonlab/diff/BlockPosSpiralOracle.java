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
import net.minecraft.core.Direction;

/**
 * {@code BlockPos.spiralAround} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPosSpiralOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BlockPos#spiralAround(BlockPos, int, Direction, Direction)}，把迭代结果
 * 逐个输出。
 *
 * <p>本能力不需要注册表——{@code Direction} 是静态枚举。
 *
 * <p>方向取 {@code down}/{@code up}/{@code north}/{@code south}/{@code west}/{@code east}。
 * 输出为 `x,y,z` 用 `;` 连接。
 *
 * <p>单元表必须与 probes/blockpos_spiral.cpp 逐项一致。
 */
public final class BlockPosSpiralOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|0|0|0|east|south",
                    "1|0|0|0|east|south",
                    "2|0|0|0|east|south",
                    "3|0|0|0|east|south",
                    "1|0|0|0|north|up",
                    "2|0|0|0|north|up",
                    "1|10|64|-20|east|south",
                    "1|0|0|0|south|east");

    private BlockPosSpiralOracle() {}

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
                "usage: BlockPosSpiralOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按名字查方向。 */
    private static Direction directionByName(String name) {
        for (Direction direction : Direction.values()) {
            if (direction.getName().equals(name)) {
                return direction;
            }
        }
        throw new IllegalArgumentException("blockpos_spiral: 未知方向: " + name);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 6) {
            throw new IllegalArgumentException("blockpos_spiral: 单元需要 6 段: " + unit);
        }

        int radius = Integer.parseInt(parts[0]);
        BlockPos center = new BlockPos(
                Integer.parseInt(parts[1]), Integer.parseInt(parts[2]),
                Integer.parseInt(parts[3]));
        Direction first = directionByName(parts[4]);
        Direction second = directionByName(parts[5]);

        StringBuilder value = new StringBuilder();
        for (BlockPos pos : BlockPos.spiralAround(center, radius, first, second)) {
            if (value.length() > 0) {
                value.append(';');
            }
            value.append(pos.getX()).append(',').append(pos.getY()).append(',')
                    .append(pos.getZ());
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", value.toString());
        return out;
    }
}