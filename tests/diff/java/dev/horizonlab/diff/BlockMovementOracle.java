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
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;

/**
 * 方块对实体移动的作用 + 行为开关的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockMovementOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/block_movement.cpp 同构：
 *
 * <pre>
 *   { "block": "minecraft:honey_block", "speedFactor": "0x3ecccccd", "jumpFactor": "0x3f000000",
 *     "ticksRandomly": false, "signalSource": false, "analogOutput": false }
 * </pre>
 *
 * <p>浮点用位模式而不是十进制：两侧各自的格式化会让 1.5f 与 1.4999999f 都被印成
 * 看起来相同的字符串，位模式比较才是逐位的。
 */
public final class BlockMovementOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockMovementOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                names.add(id.toString());
            }
            names.sort(Comparator.naturalOrder());
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockMovementOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("block").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("block", name);

        Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(name)).orElse(null);
        if (block == null) {
            result.addProperty("missing", true);
            return result;
        }

        BlockState state = block.defaultBlockState();
        result.addProperty("speedFactor", bits(block.getSpeedFactor()));
        result.addProperty("jumpFactor", bits(block.getJumpFactor()));
        result.addProperty("ticksRandomly", state.isRandomlyTicking());
        result.addProperty("signalSource", state.isSignalSource());
        result.addProperty("analogOutput", state.hasAnalogOutputSignal());
        return result;
    }
}