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
import java.util.function.Consumer;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;

/**
 * {@code BlockPos.breadthFirstTraversal} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPosBfsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BlockPos#breadthFirstTraversal}，访问顺序由传入的
 * `BiConsumer` 记录，不重写队列逻辑。
 *
 * <p>邻居按 {@link Direction#values()} 的顺序展开（`DOWN, UP, NORTH, SOUTH, WEST, EAST`），
 * 与探针一致。
 *
 * <p>单元表必须与 probes/blockpos_bfs.cpp 逐项一致。
 */
public final class BlockPosBfsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|100|free",
                    "1|100|free",
                    "2|100|free",
                    "2|1|free",
                    "2|2|free",
                    "2|3|free",
                    "2|7|free",
                    "2|8|free",
                    "1|7|free",
                    "2|100|skipX0",
                    "2|100|skipOdd",
                    "2|100|stopX1",
                    "3|1000|free",
                    "0|1|free");

    private BlockPosBfsOracle() {}

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
                "usage: BlockPosBfsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 规则名 → 节点状态判定。 */
    private static BlockPos.TraversalNodeStatus evaluate(String rule, BlockPos pos) {
        switch (rule) {
            case "free":
                return BlockPos.TraversalNodeStatus.ACCEPT;
            case "skipX0":
                return pos.getX() == 0
                        ? BlockPos.TraversalNodeStatus.SKIP
                        : BlockPos.TraversalNodeStatus.ACCEPT;
            case "stopX1":
                return pos.getX() == 1
                        ? BlockPos.TraversalNodeStatus.STOP
                        : BlockPos.TraversalNodeStatus.ACCEPT;
            case "skipOdd":
                return Math.floorMod(pos.getX(), 2) != 0
                        ? BlockPos.TraversalNodeStatus.SKIP
                        : BlockPos.TraversalNodeStatus.ACCEPT;
            default:
                throw new IllegalArgumentException("blockpos_bfs: 未知规则: " + rule);
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException("blockpos_bfs: 单元应为 '<maxDepth>|<maxCount>|<规则>': " + unit);
        }

        int maxDepth = Integer.parseInt(parts[0]);
        int maxCount = Integer.parseInt(parts[1]);
        String rule = parts[2];

        List<BlockPos> order = new ArrayList<>();

        // 访问顺序在 `nodeStatus` 里记录：它在 `consumer` 之前调用，此刻 `pos` 尚未被复用。
        // 若在 `consumer` 里记录，`BlockPos` 的复用会让所有条目指向最后一次写入的坐标。
        int count = BlockPos.breadthFirstTraversal(
                BlockPos.ZERO,
                maxDepth,
                maxCount,
                (pos, consumer) -> {
                    for (Direction direction : Direction.values()) {
                        consumer.accept(pos.relative(direction));
                    }
                },
                pos -> {
                    BlockPos.TraversalNodeStatus status = evaluate(rule, pos);
                    if (status != BlockPos.TraversalNodeStatus.SKIP
                            && status != BlockPos.TraversalNodeStatus.STOP) {
                        order.add(pos.immutable());
                    }
                    return status;
                });

        StringBuilder builder = new StringBuilder();
        for (BlockPos pos : order) {
            if (builder.length() > 0) {
                builder.append(';');
            }
            builder.append(pos.getX()).append(',').append(pos.getY()).append(',').append(pos.getZ());
        }
        if (builder.length() == 0) {
            builder.append('-');
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("count", count);
        result.addProperty("order", builder.toString());
        return result;
    }
}