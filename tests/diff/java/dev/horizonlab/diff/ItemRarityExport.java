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
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.Rarity;

/**
 * 导出所有物品的稀有度（非 common 的才输出，common 是默认值）。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemRarityExport &lt;output.json&gt;
 * </pre>
 *
 * 只导出非默认项：全量 1505 条里绝大多数是 common，导出它们只会让生成代码变得臃肿，
 * 而且一旦原版把某个物品从 uncommon 改回 common，删掉一条比改一条更容易看清。
 */
public final class ItemRarityExport {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemRarityExport() {}

    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("usage: ItemRarityExport <output.json>");
            System.exit(2);
        }

        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        List<String> names = BuiltInRegistries.ITEM.keySet().stream().map(Identifier::toString).sorted().toList();
        JsonArray entries = new JsonArray();
        for (String name : names) {
            Item item = BuiltInRegistries.ITEM.getOptional(Identifier.parse(name)).orElseThrow();
            Rarity rarity = item.components().getOrDefault(DataComponents.RARITY, Rarity.COMMON);
            if (rarity == Rarity.COMMON) {
                continue;
            }
            JsonObject entry = new JsonObject();
            entry.addProperty("item", name);
            entry.addProperty("rarity", rarity.name().toLowerCase());
            entries.add(entry);
        }

        Files.writeString(Path.of(args[0]), GSON.toJson(entries), StandardCharsets.UTF_8);
        System.out.println("exported " + entries.size() + " non-common rarities");
    }
}