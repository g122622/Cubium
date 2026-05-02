<screen id="mainMenu" title="Minecraft Reborn">
    <text id="title" text="Minecraft Reborn" pos="200,50" size="200,40" align="center"/>
    <text id="version" text="v0.1.0" pos="200,90" size="200,20" align="center"/>

    <container id="buttonContainer" pos="200,150" size="200,150">
        <button id="singlePlayer" text="Singleplayer" pos="0,0" size="180,20" on:click="onSinglePlayer"/>
        <button id="multiPlayer" text="Multiplayer" pos="0,30" size="180,20" on:click="onMultiPlayer" active="false"/>
        <button id="options" text="Options..." pos="0,60" size="180,20" on:click="onOptions"/>
        <button id="quit" text="Quit Game" pos="0,90" size="180,20" on:click="onQuit"/>
    </container>
</screen>
