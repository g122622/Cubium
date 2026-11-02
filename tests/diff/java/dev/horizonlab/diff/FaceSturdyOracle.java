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
import net.minecraft.SharedConstants;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.EmptyBlockGetter;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.SupportType;

/**
 * {@code isFaceSturdy} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FaceSturdyOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元格式：{@code <block>|<Direction>|<SupportType>}，输出 {@code "true"} / {@code "false"}。
 *
 * <p>原版三种 {@code SupportType}：
 *
 * <pre>{@code
 * FULL   { return Block.isFaceFull(state.getBlockSupportShape(level, pos), dir); }
 * CENTER { return !Shapes.joinIsNotEmpty(faceShape, Block.column(2.0, 0.0, 10.0), ONLY_SECOND); }
 * RIGID  { return !Shapes.joinIsNotEmpty(faceShape,
 *              Shapes.join(Shapes.block(), Block.column(12.0, 0.0, 16.0), ONLY_FIRST), ONLY_SECOND); }
 * }</pre>
 *
 * <p下面三者互为「更宽松」的包含关系：{@code FULL ⊆ RIGID}、{@code CENTER ⊆ RIGID}？并非如此——
 * 它们是<b>三个独立条件</b>，一个方块可以满足 CENTER 而不满足 RIGID（如细柱子）。
 *
 * <p>本能力是<b>纯计算</b>：{@code isFaceSturdy} 只查方块自身的支撑形状，不需要邻域世界。
 * 用 {@code EmptyBlockGetter.INSTANCE} 即可。
 *
 * <p>本能力<b>需要 bootstrap</b>：要访问方块注册表。
 *
 * <p>单元表必须与 probes/face_sturdy.cpp 逐项一致（两侧都由注册表枚举，顺序需对齐）。
 */
public final class FaceSturdyOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 方向名（Java 常量名）。 */
    private static final List<String> DIR_NAMES =
            List.of("DOWN", "UP", "NORTH", "SOUTH", "WEST", "EAST");

    /** 支撑类型名。 */
    private static final List<String> SUPPORT_NAMES = List.of("FULL", "CENTER", "RIGID");

    private FaceSturdyOracle() {}

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            // 与 C++ 侧一致：按方块注册名排序，然后 6 方向 × 3 类型。
            List<String> blockNames =
                    BuiltInRegistries.BLOCK.keySet().stream()
                            .map(Identifier::toString)
                            .sorted()
                            .toList();
            StringBuilder text = new StringBuilder();
            for (String blockName : blockNames) {
                for (String dir : DIR_NAMES) {
                    for (String support : SUPPORT_NAMES) {
                        text.append(blockName)
                                .append('|')
                                .append(dir)
                                .append('|')
                                .append(support)
                                .append('\n');
                    }
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: FaceSturdyOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

    private static SupportType supportTypeOf(String name) {
        return switch (name) {
            case "FULL" -> SupportType.FULL;
            case "CENTER" -> SupportType.CENTER;
            case "RIGID" -> SupportType.RIGID;
            default -> throw new IllegalArgumentException("face_sturdy: 未知支撑类型: " + name);
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException(
                    "face_sturdy: 单元应为 <block>|<dir>|<type>，实际: " + unit);
        }

        Identifier id = Identifier.parse(parts[0]);
        Block block = BuiltInRegistries.BLOCK.getValue(id);
        if (block == null) {
            throw new IllegalArgumentException("face_sturdy: 未知方块: " + parts[0]);
        }

        Direction dir = Direction.valueOf(parts[1]);
        SupportType type = supportTypeOf(parts[2]);

        BlockState state = block.defaultBlockState();
        BlockPos pos = BlockPos.ZERO;
        boolean result = state.isFaceSturdy(EmptyBlockGetter.INSTANCE, pos, dir, type);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", result ? "true" : "false");
        return out;
    }
}