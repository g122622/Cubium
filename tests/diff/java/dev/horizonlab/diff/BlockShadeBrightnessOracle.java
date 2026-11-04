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
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.EmptyBlockGetter;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.core.BlockPos;

/**
 * 方块遮光亮度的原版 oracle（{@code BlockBehaviour#getShadeBrightness}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockShadeBrightnessOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>规则：碰撞形状是完整方块 -&gt; 0.2，否则 -&gt; 1.0。
 *
 * <p>有几个方块碰撞形状不完整却仍要返回 0.2（原版专门覆写过）：
 * 泥巴、灵魂沙是 14/16 的矮块但视觉实心；雪层满层返回 0.2、其余层数 1.0。
 * 反过来屏障/结构空位这类不可见方块返回 1.0。
 *
 * <p>按默认规则处理泥巴和灵魂沙，它们会看起来像「浮在地面上的亮块」。
 *
 * <p>本探针在无世界上下文中取值，对应原版实现里用默认 BlockGetter 的分支。
 */
public final class BlockShadeBrightnessOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockShadeBrightnessOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                units.add(id.toString());
            }
            java.util.Collections.sort(units);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockShadeBrightnessOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        Identifier id = Identifier.tryParse(unit);
        Block block = id == null ? null : BuiltInRegistries.BLOCK.get(id).map(h -> h.value()).orElse(null);
        if (block == null) {
            out.addProperty("missing", true);
            return out;
        }

        BlockState state = block.defaultBlockState();
        var getter = EmptyBlockGetter.INSTANCE;
        BlockPos pos = BlockPos.ZERO;

        // 原版 getShadeBrightness 是 protected，通过 BlockState 的公开转发取值
        float shade = state.getShadeBrightness(getter, pos);
        out.addProperty("bits", Float.floatToRawIntBits(shade));
        out.addProperty("skyDown", state.propagatesSkylightDown());
        return out;
    }
}
