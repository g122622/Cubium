<screen id="loading" title="Loading">
    <container id="background" pos="0,0" size="100%,100%" background-color="#FF141414"/>

    <text id="title" text="Loading World..." pos="200,150" size="200,30" align="center"/>

    <container id="progressContainer" pos="150,200" size="300,30">
        <container id="progressBg" pos="0,0" size="300,20" background-color="#FF404040" border-color="#FF606060">
            <container id="progressFill" pos="0,0" bind:width="loading.progressWidth" size="0,20" background-color="#FF80C080"/>
        </container>
    </container>

    <text id="stageText" text="Preparing world..." pos="200,240" size="200,20" align="center" bind:text="loading.stage"/>
</screen>
