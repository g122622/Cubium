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
import net.minecraft.world.level.block.state.BlockState;

/**
 * 方块硬度与爆炸抗性的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockHardnessOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <ul>
 *   <li>硬度：{@code BlockState.getDestroySpeed(null, null)}（原版的默认状态取值）；
 *   <li>抗性：{@code Block.getExplosionResistance()}。
 * </ul>
 *
 * 两者都以 f32 位模式输出（{@code 0x%08x}），跨语言比浮点必须比位。
 */
public final class BlockHardnessOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockHardnessOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                units.add(id.toString());
            }
            units.sort(String::compareTo);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockHardnessOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        Block block = id == null
            ? null
            : BuiltInRegistries.BLOCK.get(id).map(h -> h.value()).orElse(null);
        if (block == null) {
            out.addProperty("missing", true);
            return out;
        }

        BlockState state = block.defaultBlockState();
        out.addProperty("hardness", bits(state.getDestroySpeed(null, null)));
        out.addProperty("resistance", bits(block.getExplosionResistance()));
        return out;
    }

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}
