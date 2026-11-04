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
 * 方块爆炸抗性的原版 oracle（{@code Block#getExplosionResistance}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockBlastResistanceOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是方块 id。输出 f32 位模式（抗性在原版是 float），避免十进制展开制造伪差异。
 *
 * <p>这个值决定爆炸能否摧毁方块：阈值约等于 {@code 威力 / 抗性}。写错不会崩、不会报错，
 * 只会表现为「TNT 炸不动这堵墙」或「炸得比预期干净」。
 *
 * <p>注意原版很多方块的抗性<b>不来自自身属性，而是转交底层方块</b>：
 * 楼梯、台阶、活板门、栅栏、墙等都会 {@code return base.getExplosionResistance()}。
 */
public final class BlockBlastResistanceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockBlastResistanceOracle() {}

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
        System.err.println("usage: BlockBlastResistanceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        float resistance = block.getExplosionResistance();
        out.addProperty("bits", Float.floatToRawIntBits(resistance));
        return out;
    }
}
