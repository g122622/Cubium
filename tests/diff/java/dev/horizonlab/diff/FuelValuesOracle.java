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
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.level.block.entity.FuelValues;

/**
 * 燃料燃烧时长的原版 oracle（{@code net.minecraft.world.level.block.entity.FuelValues}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.FuelValuesOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.FuelValuesOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>1.21.11 把燃料表收进了 {@link FuelValues}：用标签聚合（{@code ItemTags.LOGS} 一条
 * 覆盖全部原木）再 {@code remove(NON_FLAMMABLE_WOOD)} 剔除绯红/诡异木。这里直接调用
 * {@code burnDuration}，让原版自己算，探针那边同样只调服务端入口——两边都不复述规则。
 *
 * <p>探针与 oracle 都必须用 **1.21.11 的默认基准 200 tick**，否则两侧整体差一个倍数。
 */
public final class FuelValuesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private FuelValuesOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : itemIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: FuelValuesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static List<String> itemIds() {
        // 必须先触发 bootstrap（`LootServerData.get()` 会跑 Bootstrap.bootStrap），
        // 再碰 BuiltInRegistries：静态初始化阶段读它会抛 "Not bootstrapped"。
        LootServerData.get();
        List<String> ids = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
            ids.add(id.toString());
        }
        ids.sort(String::compareTo);
        return ids;
    }

    /**
     * 原版默认燃料表。
     *
     * <p>与玩家实际使用的同一份：服务端在启动时用
     * {@code FuelValues.vanillaBurnTimes(registries, enabledFeatures)} 建它，基准 200。
     */
    private static FuelValues fuelValues() {
        var server = LootLevel.get().level.getServer();
        return FuelValues.vanillaBurnTimes(server.registryAccess(), server.getWorldData().enabledFeatures());
    }

    private static void run(Path input, Path output) throws IOException {
        FuelValues values = fuelValues();
        var itemLookup = LootServerData.get().registries.compositeAccess().lookupOrThrow(Registries.ITEM);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("item").getAsString();

            JsonObject result = new JsonObject();
            result.addProperty("item", unit);

            Item item = itemLookup.get(ResourceKey.create(Registries.ITEM, Identifier.parse(unit)))
                .map(holder -> holder.value())
                .orElse(null);
            if (item == null || item == Items.AIR) {
                result.addProperty("ok", false);
                result.addProperty("error", "unknown item");
            } else {
                result.addProperty("ok", true);
                result.addProperty("burnTime", values.burnDuration(new ItemStack(item, 1)));
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }
}