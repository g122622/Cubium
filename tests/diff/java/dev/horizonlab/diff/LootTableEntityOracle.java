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

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.EntitySpawnReason;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.storage.loot.LootParams;
import net.minecraft.world.level.storage.loot.parameters.LootContextParams;

/**
 * 生物类战利品表在「击杀」上下文下的抽取，原版参照实现。
 *
 * <pre>
 *   java dev.horizonlab.diff.LootTableEntityOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/loot_table_entity.cpp 同构：`entity` 是被击杀实体、`attacker` 是击杀者、
 * `looting` 是击杀者主手的掠夺等级、`onFire` 表示被击杀实体着火。
 *
 * <p>原版没有「掠夺等级」这种上下文键：`enchanted_count_increase` 函数会读
 * {@code ATTACKING_ENTITY} 主手武器的掠夺附魔。因此这里把掠夺附魔真实地写到
 * 击杀者主手上——同样是真实游戏里的形态。
 */
public final class LootTableEntityOracle {
    private LootTableEntityOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var data = LootServerData.get();
            StringBuilder text = new StringBuilder();
            for (String id : data.tableIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LootTableEntityOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /**
     * 原版建不出实例的被击杀实体：玩家没有可供 {@code EntityType#create} 使用的工厂
     * （服务端由登录流程构造 ServerPlayer，必须挂在真实服务端上），因此以玩家为
     * this_entity 的表两侧都无从比较，直接跳过而不是伪造一个半初始化的玩家。
     */
    private static final java.util.Set<String> UNSPAWNABLE_VICTIMS = java.util.Set.of("minecraft:player");

    private static void run(Path input, Path output) throws IOException {
        ServerLevel level = LootLevel.get().level;
        // 键是「表#场景」，表名从键里取；场景只改上下文，不改表。
        // 单元里的玩家表在 oracle 侧无法构造上下文：输出 `missing` 会让它变成差异，
        // 所以由 harness 侧用参考表剔除（见 loot_context.ts 的 ENTITY_ORACLE_MISSING）。
        LootRoll.run(input, output, "table", (table, request, result) -> {
            String victimId = request.has("entity") ? request.get("entity").getAsString() : "minecraft:pig";
            if (UNSPAWNABLE_VICTIMS.contains(victimId)) {
                result.addProperty("missing", true);
                return;
            }
            Entity victim = spawn(victimId, level);
            if (victim == null) {
                result.addProperty("missing", true);
                return;
            }
            Entity attacker = request.has("attacker") ? spawn(request.get("attacker").getAsString(), level) : null;
            equipLooting(attacker, request.has("looting") ? request.get("looting").getAsInt() : 0);
            if (request.has("attackerCaptain") && request.get("attackerCaptain").getAsBoolean()) {
                wearOminousBanner(attacker);
            }
            if (request.has("onFire") && request.get("onFire").getAsBoolean()) {
                victim.setRemainingFireTicks(100);
            }

            result.add("samples", LootRoll.samples(table, random -> new LootParams.Builder(level)
                .withParameter(LootContextParams.THIS_ENTITY, victim)
                .withParameter(LootContextParams.ORIGIN, LootTableOracle.origin(request))
                .withParameter(LootContextParams.DAMAGE_SOURCE, victim.damageSources().magic())
                .withOptionalParameter(LootContextParams.ATTACKING_ENTITY, attacker)
                .create(table.getParamSet())));
        });
    }

    /**
     * 让袭击者成为队长：原版 {@code Raider.isCaptain()} 的唯一判据就是「头戴不祥旗帜」，
     * 这里写入的正是 {@code Raid.getOminousBannerInstance} 生成的那一份。
     */
    private static void wearOminousBanner(Entity attacker) {
        if (!(attacker instanceof net.minecraft.world.entity.raid.Raider raider)) {
            return;
        }
        var banner = net.minecraft.world.entity.raid.Raid.getOminousBannerInstance(
                LootServerData.get().registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.BANNER_PATTERN));
        raider.setItemSlot(net.minecraft.world.entity.EquipmentSlot.HEAD, banner);
    }

    /** 按名字生成实体；位置固定在 (0.5, 64, 0.5)，与 C++ 探针一致。 */
    private static Entity spawn(String id, ServerLevel level) {
        EntityType<?> type = BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(id)).orElse(null);
        if (type == null) {
            return null;
        }
        return type.create(level, EntitySpawnReason.EVENT);
    }

    /** 把掠夺附魔写到击杀者主手上（原版 `enchanted_count_increase` 就是这么读的）。 */
    static void equipLooting(Entity attacker, int looting) {
        if (attacker == null || looting <= 0 || !(attacker instanceof net.minecraft.world.entity.LivingEntity living)) {
            return;
        }
        var lookup = LootServerData.get().registryAccess()
            .lookupOrThrow(net.minecraft.core.registries.Registries.ENCHANTMENT);
        var lootingEnchantment = lookup.getOrThrow(net.minecraft.resources.ResourceKey.create(
            net.minecraft.core.registries.Registries.ENCHANTMENT, Identifier.parse("minecraft:looting")));
        ItemStack weapon = living.getMainHandItem().copy();
        if (weapon.isEmpty()) {
            weapon = new ItemStack(BuiltInRegistries.ITEM.getOptional(Identifier.parse("minecraft:diamond_sword")).orElseThrow());
        }
        weapon.enchant(lootingEnchantment, looting);
        living.setItemSlot(net.minecraft.world.entity.EquipmentSlot.MAINHAND, weapon);
    }
}