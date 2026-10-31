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
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.world.level.levelgen.Beardifier;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.TerrainAdjustment;
import net.minecraft.world.level.levelgen.structure.pools.JigsawJunction;
import net.minecraft.world.level.levelgen.structure.pools.StructureTemplatePool;

/**
 * 原版结构地形适配采样器（Beardifier）的参照实现。
 *
 * <p>直接构造 1.21.11 的 {@link Beardifier} 并调用 {@code compute}，不经过任何地形生成流程：
 * 被测对象是一个纯函数「给定片段与连接点，某坐标处的密度偏移是多少」。语料里的
 * {@code affected} 为空时按原版 {@code Beardifier.EMPTY} 的构造形态传 {@code null}，
 * 从而同时覆盖「没有影响范围包围盒」与「有包围盒但列表为空」两种形态。
 *
 * <p>两个子命令：
 *
 * <ul>
 *   <li>{@code list <units.txt>}：单元来自共同语料，原版侧不从注册表枚举；
 *   <li>{@code run <input.jsonl> <output.jsonl>}：每行一个场景，输出每个采样点的
 *       {@code compute} 结果。
 * </ul>
 */
public final class StructureDensityOracle {
    private static final Gson GSON = new Gson();

    private StructureDensityOracle() {}

    /** 单个场景：片段列表 + 连接点列表 + 可选的影响范围包围盒 + 采样坐标。 */
    private record Scenario(List<Beardifier.Rigid> pieces, List<JigsawJunction> junctions, List<int[]> points) {}

    private static BoundingBox box(JsonElement element) {
        if (element == null || element.isJsonNull()) {
            return null;
        }
        JsonArray array = element.getAsJsonArray();
        return new BoundingBox(
                array.get(0).getAsInt(),
                array.get(1).getAsInt(),
                array.get(2).getAsInt(),
                array.get(3).getAsInt(),
                array.get(4).getAsInt(),
                array.get(5).getAsInt());
    }

    private static Scenario parse(JsonObject input) {
        List<Beardifier.Rigid> pieces = new ArrayList<>();
        for (JsonElement element : input.getAsJsonArray("pieces")) {
            JsonArray array = element.getAsJsonArray();
            pieces.add(new Beardifier.Rigid(
                    box(array.get(0)),
                    TerrainAdjustment.valueOf(array.get(1).getAsString()),
                    array.get(2).getAsInt()));
        }
        List<JigsawJunction> junctions = new ArrayList<>();
        for (JsonElement element : input.getAsJsonArray("junctions")) {
            JsonArray array = element.getAsJsonArray();
            junctions.add(new JigsawJunction(
                    array.get(0).getAsInt(),
                    array.get(1).getAsInt(),
                    array.get(2).getAsInt(),
                    0,
                    StructureTemplatePool.Projection.RIGID));
        }
        List<int[]> points = new ArrayList<>();
        for (JsonElement element : input.getAsJsonArray("points")) {
            JsonArray array = element.getAsJsonArray();
            points.add(new int[] {array.get(0).getAsInt(), array.get(1).getAsInt(), array.get(2).getAsInt()});
        }
        return new Scenario(pieces, junctions, points);
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 2 && args[0].equals("list")) {
            throw new IllegalArgumentException("StructureDensityOracle: 单元由共同语料决定，不支持 list");
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: StructureDensityOracle run <input> <output>");
        }
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        StringBuilder output = new StringBuilder();
        for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject input = JsonParser.parseString(line).getAsJsonObject();
            Scenario scenario = parse(input);
            BoundingBox affected = box(input.get("affected"));
            Beardifier sampler = new Beardifier(scenario.pieces(), scenario.junctions(), affected);

            JsonArray samples = new JsonArray();
            for (int[] point : scenario.points()) {
                // 逐点单独构造一个函数上下文，保证每个采样点都在同一起点上求值。
                DensityFunction.FunctionContext context = new DensityFunction.SinglePointContext(point[0], point[1], point[2]);
                samples.add(sampler.compute(context));
            }
            JsonObject answer = new JsonObject();
            answer.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
            answer.add("samples", samples);
            output.append(GSON.toJson(answer)).append('\n');
        }
        Files.writeString(Path.of(args[2]), output, StandardCharsets.UTF_8);
    }
}