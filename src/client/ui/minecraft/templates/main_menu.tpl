<screen id="mainMenu" title="Minecraft Reborn">
    <text id="title" text="Minecraft Reborn" align="center" size="200,40"/>
    <text id="version" text="v0.1.0" align="center" size="200,20"/>

    <container id="buttonContainer" layout="flex-column" gap="10" size="200,140" align-items="center">
        <button id="singlePlayer" text="Singleplayer" size="200,20" on:click="onSinglePlayer"/>
        <button id="multiPlayer" text="Multiplayer" size="200,20" on:click="onMultiPlayer" active="false"/>
        <button id="options" text="Options..." size="200,20" on:click="onOptions"/>
        <button id="quit" text="Quit Game" size="200,20" on:click="onQuit"/>
    </container>
</screen>
