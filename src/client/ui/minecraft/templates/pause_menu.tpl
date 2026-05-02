<screen id="pause" title="Game Paused" modal="true">
    <container id="overlay" pos="0,0" size="100%,100%" background-color="#B4000000"/>

    <text id="title" text="Game Paused" pos="200,100" size="200,30" align="center"/>

    <container id="buttonContainer" pos="200,180" size="200,120">
        <button id="resume" text="Back to Game" pos="0,0" size="180,20" on:click="onResume"/>
        <button id="options" text="Options..." pos="0,30" size="180,20" on:click="onOptions"/>
        <button id="quit" text="Save and Quit to Title" pos="0,60" size="180,20" on:click="onSaveAndQuit"/>
    </container>
</screen>
