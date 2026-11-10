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
import java.io.BufferedWriter;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.WorldgenRandom;

/**
 * 林地府邸布局网格的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MansionGridOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入一行一个样本：种子。输出基础网格、三楼网格与三层房间网格的全部数值。
 *
 * <p>{@code WoodlandMansionPieces.Grid} 与 {@code SimpleGrid} 都是包私有类，字段也私有，
 * 因此全部经反射访问——被测侧是同一套算法的 C++ 移植，反射只是取数据，不改变算法。
 */
public final class MansionGridOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private MansionGridOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: MansionGridOracle run <input> <output>");
        }
        try (BufferedWriter writer = Files.newBufferedWriter(Path.of(args[2]), StandardCharsets.UTF_8)) {
            for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
                if (line.isBlank()) {
                    continue;
                }
                writer.write(GSON.toJson(runSample(JsonParser.parseString(line).getAsJsonObject())));
                writer.newLine();
            }
        }
    }

    private static JsonObject runSample(JsonObject input) {
        final long seed = input.get("seed").getAsLong();
        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");

        try {
            Class<?> gridClass =
                    Class.forName("net.minecraft.world.level.levelgen.structure.structures.WoodlandMansionPieces$MansionGrid");
            Constructor<?> ctor = gridClass.getDeclaredConstructor(RandomSource.class);
            ctor.setAccessible(true);
            // 生产用的是 `WorldgenRandom`（BitRandomSource），不是 `java.util.Random`。
            RandomSource random = new WorldgenRandom(new LegacyRandomSource(seed));
            Object grid = ctor.newInstance(random);

            Field baseField = gridClass.getDeclaredField("baseGrid");
            Field thirdField = gridClass.getDeclaredField("thirdFloorGrid");
            Field roomsField = gridClass.getDeclaredField("floorRooms");
            baseField.setAccessible(true);
            thirdField.setAccessible(true);
            roomsField.setAccessible(true);

            result.add("base", dumpGrid(baseField.get(grid)));
            result.add("third", dumpGrid(thirdField.get(grid)));
            JsonArray floors = new JsonArray();
            Object[] roomGrids = (Object[]) roomsField.get(grid);
            for (Object roomGrid : roomGrids) {
                floors.add(dumpGrid(roomGrid));
            }
            result.add("rooms", floors);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("MansionGridOracle: 无法读取布局网格", error);
        }
        return result;
    }

    /** 把一个 `SimpleGrid` 序列化成二维数组。 */
    private static JsonArray dumpGrid(Object grid) throws ReflectiveOperationException {
        Class<?> gridClass = grid.getClass();
        Field dataField = gridClass.getDeclaredField("grid");
        Field widthField = gridClass.getDeclaredField("width");
        Field heightField = gridClass.getDeclaredField("height");
        dataField.setAccessible(true);
        widthField.setAccessible(true);
        heightField.setAccessible(true);

        int width = widthField.getInt(grid);
        int height = heightField.getInt(grid);
        int[][] cells = (int[][]) dataField.get(grid);

        JsonArray rows = new JsonArray();
        for (int y = 0; y < height; y++) {
            JsonArray row = new JsonArray();
            for (int x = 0; x < width; x++) {
                row.add(cells[x][y]);
            }
            rows.add(row);
        }
        return rows;
    }
}