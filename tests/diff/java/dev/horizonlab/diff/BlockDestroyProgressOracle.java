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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;

/**
 * 方块破坏进度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockDestroyProgressOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.BlockDestroyProgressOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/block_destroy_progress.cpp 同构：报 destroySpeed（即界面上的硬度）与
 * 「正确工具 / 错误工具」两种情形下每 tick 的破坏进度。
 *
 * <p>公式取自 vanilla {@code BlockBehaviour.getDestroyProgress:346-353}：
 * {@code speed / destroySpeed / (hasCorrectToolForDrops ? 30 : 100)}，其中
 * {@code destroySpeed == BlockStateBase.destroySpeed == properties.destroyTime}。
 *
 * <p>不构造玩家对象：玩家一侧的挖掘速度（工具值 + 效率附魔 + 水中惩罚）由
 * {@code MiningManagerTest} 与 {@code block_properties} 分别覆盖，这里只比「方块自己的除数」。
 */
public final class BlockDestroyProgressOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final float CORRECT_TOOL_DIVISOR = 30.0F;
    private static final float WRONG_TOOL_DIVISOR = 100.0F;

    private BlockDestroyProgressOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                names.add(id.toString());
            }
            names.sort(String::compareTo);
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
        System.err.println("usage: BlockDestroyProgressOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String blockId) {
        JsonObject result = new JsonObject();
        result.addProperty("block", blockId);

        Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(blockId)).orElse(null);
        if (block == null) {
            result.addProperty("missing", true);
            return result;
        }

        BlockState state = block.defaultBlockState();
        // BlockStateBase#getDestroySpeed(BlockGetter, BlockPos) 直接返回 properties.destroyTime，
        // 与方块位置无关；原版把「界面显示的硬度」也取的是这个值。
        float hardness = state.getDestroySpeed(null, null);
        if (hardness < 0.0F) {
            result.addProperty("hardness", floatBits(hardness));
            result.addProperty("correctToolProgress", floatBits(0.0F));
            result.addProperty("wrongToolProgress", floatBits(0.0F));
            return result;
        }
        if (hardness == 0.0F) {
            result.addProperty("hardness", floatBits(hardness));
            result.addProperty("correctToolProgress", floatBits(1.0F));
            result.addProperty("wrongToolProgress", floatBits(1.0F));
            return result;
        }

        // 直接用真实的钻石镐物品堆，不猜工具类型：
        //   工具速度 = ItemStack.getDestroySpeed(state)（Item.java:180-183：读 TOOL 组件的
        //   miningSpeed，方块不适用时就是 1.0）；
        //   是否「正确工具」 = Player.hasCorrectToolForDrops（Player.java:625-627：
        //   方块不要求正确工具时恒真，否则看 Item.isCorrectToolForDrops）。
        ItemStack diamondPickaxe = new ItemStack(Items.DIAMOND_PICKAXE);
        float toolSpeed = diamondPickaxe.getDestroySpeed(state);
        boolean correctTool = !state.requiresCorrectToolForDrops() || diamondPickaxe.isCorrectToolForDrops(state);

        float correct = toolSpeed / hardness / CORRECT_TOOL_DIVISOR;
        // 「错误工具」= 手上有精确采集附魔之外的空手情形：速度回落到 1.0，除数 100。
        float wrong = correctTool ? correct : 1.0F / hardness / WRONG_TOOL_DIVISOR;
        result.addProperty("hardness", floatBits(hardness));
        result.addProperty("correctToolProgress", floatBits(correct));
        result.addProperty("wrongToolProgress", floatBits(wrong));
        return result;
    }

    private static String floatBits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}