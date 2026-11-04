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
import net.minecraft.world.level.block.Block;

/**
 * 方块速度倍率与跳跃倍率的原版 oracle
 * （{@code Block#getSpeedFactor} / {@code Block#getJumpFactor}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockMovementFactorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>移动倍率：站在方块上时水平速度乘以它。灵魂沙、蜂蜜块都是 0.4。
 * <p>跳跃倍率：从方块起跳时初速度乘以它。<b>只有蜂蜜块是 0.5</b>。
 *
 * <p>其余全部方块是 1.0。两处易错：
 *
 * <ul>
 *   <li>把蜂蜜块的跳跃倍率也写成 1.0 —— 玩家能轻松跳出蜂蜜陷阱
 *   <li>把灵魂沙的跳跃倍率写成 0.5 —— 灵魂沙只减速不限制跳跃，
 *       写错会让沙漠神殿的灵魂沙跑酷几乎不可能
 * </ul>
 */
public final class BlockMovementFactorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockMovementFactorOracle() {}

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
        System.err.println("usage: BlockMovementFactorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        out.addProperty("speedBits", Float.floatToRawIntBits(block.getSpeedFactor()));
        out.addProperty("jumpBits", Float.floatToRawIntBits(block.getJumpFactor()));
        return out;
    }
}
