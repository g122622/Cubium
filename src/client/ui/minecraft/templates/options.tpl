<screen id="options" title="Options">
    <text id="title" text="Options" align="center" size="200,30"/>

    <container id="sliderContainer" layout="flex-column" gap="10" size="240,60" align-items="center">
        <slider id="musicVolume" size="200,20" range="0,100" value="70"/>
        <slider id="soundVolume" size="200,20" range="0,100" value="90"/>
    </container>

    <button id="done" text="Done" size="100,20" on:click="onClose"/>
</screen>
