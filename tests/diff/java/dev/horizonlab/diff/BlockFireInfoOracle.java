package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import it.unimi.dsi.fastutil.objects.Object2IntMap;
import java.io.IOException;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.FireBlock;

/**
 * 方块可燃性参数的原版 oracle（{@code FireBlock.setFlammable} 的两个值）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockFireInfoOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么用反射</h2>
 *
 * 原版的 {@code igniteOdds} / {@code burnOdds} 是 {@code FireBlock} 的 **private** 字段，
 * 读取它们的 {@code getIgniteOdds} / {@code getBurnOdds} 也是 private，而且带了
 * 「waterlogged 直接返回 0」的分支。这里直接读那两个 Map，拿到的是**注册的原始值**，
 * 不掺入含水判断——含水规则由 C++ 侧单独表达，两侧口径才对得上。
 *
 * 反射只读、不改任何状态。
 */
public final class BlockFireInfoOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockFireInfoOracle() {}

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
        System.err.println("usage: BlockFireInfoOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        Odds odds = Odds.load();
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString(), odds)));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit, Odds odds) {
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

        out.addProperty("encouragement", odds.ignite.getInt(block));
        out.addProperty("flammability", odds.burn.getInt(block));
        return out;
    }

    /** 反射拿到的两张「方块 -> 权重」表。 */
    private static final class Odds {
        final Object2IntMap<Block> ignite;
        final Object2IntMap<Block> burn;

        private Odds(Object2IntMap<Block> ignite, Object2IntMap<Block> burn) {
            this.ignite = ignite;
            this.burn = burn;
        }

        @SuppressWarnings("unchecked")
        static Odds load() {
            try {
                FireBlock fire = (FireBlock) Blocks.FIRE;
                Field igniteField = FireBlock.class.getDeclaredField("igniteOdds");
                Field burnField = FireBlock.class.getDeclaredField("burnOdds");
                igniteField.setAccessible(true);
                burnField.setAccessible(true);
                return new Odds(
                    (Object2IntMap<Block>) igniteField.get(fire),
                    (Object2IntMap<Block>) burnField.get(fire));
            } catch (ReflectiveOperationException e) {
                throw new IllegalStateException("cannot read FireBlock odds", e);
            }
        }
    }
}
