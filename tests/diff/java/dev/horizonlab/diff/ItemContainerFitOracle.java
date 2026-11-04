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

/**
 * 物品能否放进容器类物品的原版 oracle（{@code Item#canFitInsideContainerItems}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemContainerFitOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>规则：
 *
 * <ul>
 *   <li>{@code Item} 默认返回 true
 *   <li>{@code BlockItem} 覆写：<b>潜影盒返回 false</b>，其余仍为 true
 * </ul>
 *
 * <p>它防的是递归存储：潜影盒能装物品，如果潜影盒也能装潜影盒，就能做出无限嵌套
 * 的容器 —— 既是无限容量漏洞，也是存档体积炸弹。
 *
 * <p>判据挂在 {@code BlockItem} 上而不是按物品 id 白名单，所以任何新增的潜影盒变体
 * 都会自动被覆盖。
 */
public final class ItemContainerFitOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemContainerFitOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
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
        System.err.println("usage: ItemContainerFitOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        Item item = id == null ? null : BuiltInRegistries.ITEM.get(id).map(h -> h.value()).orElse(null);
        if (item == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("fits", item.canFitInsideContainerItems());
        return out;
    }
}
