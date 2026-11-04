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
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;

/**
 * 全部物品堆叠上限的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemMaxStackSizeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 直接遍历 {@code BuiltInRegistries.ITEM}，对每个物品新建一个数量 1 的 {@link ItemStack}
 * 再取 {@code getMaxStackSize()} —— 走的是原版真实方法，不是查表。
 */
public final class ItemMaxStackSizeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemMaxStackSizeOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
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
        System.err.println(
            "usage: ItemMaxStackSizeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        Item item = id == null
            ? null
            : BuiltInRegistries.ITEM.get(id).map(h -> h.value()).orElse(null);
        if (item == null) {
            out.addProperty("missing", true);
            return out;
        }

        ItemStack stack = new ItemStack(item, 1);
        out.addProperty("maxStackSize", stack.getMaxStackSize());
        out.addProperty("damageable", stack.isDamageableItem());
        return out;
    }
}
