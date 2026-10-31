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
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.core.Holder;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.alchemy.PotionBrewing;
import net.minecraft.world.item.alchemy.PotionContents;

/**
 * 酿造配方的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PotionBrewingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/potion_brewing.cpp 同构：对「药水 × 试剂」逐个回答三件事——该物品是不是酿造
 * 材料、这组输入能否酿造、酿造后得到哪种药水。
 */
public final class PotionBrewingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private PotionBrewingOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), "", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: PotionBrewingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        PotionBrewing brewing = PotionBrewing.bootstrap(net.minecraft.world.flag.FeatureFlags.DEFAULT_FLAGS);
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject caseObject = JsonParser.parseString(line).getAsJsonObject();
            text.append(GSON.toJson(resolve(brewing, caseObject))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject resolve(PotionBrewing brewing, JsonObject input) {
        String potionId = input.get("potion").getAsString();
        String reagentId = input.get("reagent").getAsString();

        var potionHolder = BuiltInRegistries.POTION.get(
                        net.minecraft.resources.ResourceKey.create(
                                net.minecraft.core.registries.Registries.POTION, Identifier.parse(potionId)))
                .orElse(null);
        var reagentItem = BuiltInRegistries.ITEM.get(Identifier.parse(reagentId)).orElse(null);
        if (potionHolder == null || reagentItem == null) {
            JsonObject missing = new JsonObject();
            missing.addProperty("unit", input.get("unit").getAsString());
            missing.addProperty("missing", true);
            return missing;
        }

        ItemStack reagentStack = new ItemStack(reagentItem.value());
        ItemStack potionStack = new ItemStack(Items.POTION);
        potionStack.set(DataComponents.POTION_CONTENTS, new PotionContents(potionHolder));

        JsonObject result = new JsonObject();
        result.addProperty("unit", input.get("unit").getAsString());
        result.addProperty("isReagent", brewing.isIngredient(reagentStack));
        boolean canBrew = brewing.hasMix(potionStack, reagentStack);
        result.addProperty("canBrew", canBrew);

        if (canBrew) {
            ItemStack brewed = brewing.mix(potionStack, reagentStack);
            PotionContents contents = brewed.get(DataComponents.POTION_CONTENTS);
            if (contents == null || contents.potion().isEmpty()) {
                result.add("output", JsonNull.INSTANCE);
            } else {
                var key = contents.potion().get().unwrapKey().orElse(null);
                result.addProperty("output", key == null ? "minecraft:" : key.identifier().toString());
            }
        } else {
            result.add("output", JsonNull.INSTANCE);
        }
        return result;
    }
}