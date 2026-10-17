<screen id="inventory" title="Inventory">
    <text id="playerName" bind:text="player.name" pos="10,10"/>
    <text id="health" bind:text="player.health" pos="10,30"/>
    <text id="hunger" bind:text="player.hunger" pos="10,50"/>

    <grid id="inventoryGrid" cols="9" rows="4" pos="10,80" size="300,140">
        <slot id="slot" on:click="onSlotClick"/>
    </grid>

    <grid id="hotbar" cols="9" rows="1" pos="10,230" size="300,35">
        <slot id="hotbarSlot" on:click="onHotbarClick"/>
    </grid>

    <button id="closeBtn" text="Close" on:click="onClose" pos="330,230" size="80,20"/>
</screen>
