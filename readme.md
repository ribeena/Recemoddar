# Recemoddar
A proof of concept modder tool for Recettear. This hooks a patched Recettear
to allow for loading files from a `/mods/` folder in your recettear directory.

Currently in development with `FancyScreenPatchForRecettear` to make a seemless
experience, if you are an early adopter, use the unreleased version with Black Borders disabled
(`.\FancyScreenPatchForRecettear\Install-FancyScreenPatchForRecettear.ps1 -SkipConfigurator -DisableBlackBars $True -CheatEngineTablePath C:\Shared\SteamGames\common\Recettear\Recettear.CT`).

## Getting started
You must have installed [FancyScreenPatchForRecettear](https://github.com/just-harry/FancyScreenPatchForRecettear) to
start - settings used for this version of the modding tool;
- screen resolution: 1920x1080
- frame rate: 60 fps
- install DxWrapper (needed for creating the hooks)
- Float Interpolation Settings - all disabled
- Texture filtering settings - all nearest neighbor but:
	- Flora: Anisotropic
	- MeshTextures: Anisotropic
	- ChestTextures: Anisotropic

At this stage, anything outside of these patches hasn't been tested - so memory
patches could cause issues and not work! This is also only been tested with
the English verion.

Now edit the `DxWrapper.ini`;
```
[General]
LoadCustomDllPath=Recemoddar.dll

[Compatibility]
D3d8to9=1
EnableD3d9Wrapper=0

[d3d9]
EnableVSync=1
```

Place the `Recemoddar.dll` and `recemoddar.ini` into the recettear.exe directory.

You won't see anything until you have [replacements](https://www.nexusmods.com/recettearanitemshopstale/mods/2) in `mods` directory!

## What can it do?

### .x files replacements
You can place `.x` files into `/mods/xfile/` or `/mods/xfile2/` and it will
load those instead. Refer to [RecettearXTools](https://github.com/ribeena/RecettearXTools) for tools
to help with modding the `.x` files of Recettear.

### 3d model images up to 4 times the size
Using [recettear repacker](https://github.com/UnrealPowerz/recettear-repacker) you can extract the
expected `.tga` and `.bmp` files, these generally go into `/mods/xfile/`,
and add either `_2x` or `_4x` to the end of the file to indicate the upscaled
amount.

Recettear seems to support `.dds` files for this, so it'll check for those too.
DDS DXT1/3 seems to work fine for `.bmp` and DDS DXT5 for `.tga`, but more testing
could be done - these will significant reduce fileszie but may affect quality.

### UI image replacement
Using the same format as above, adding `_2x` on any UI graphics mostly work.
You can also include images without the `_2x` if just replacing the graphics.

### Events
You can replace the `.ivt` files in the `mods/iv/` folder, using the same
structure as `recettear repacker` exports. This allows for spelling corrections,
or just general hijinks.

The `Event` settings in the ini will automatically move characters off screen 
properly, using anything beyond 340 as _offscreen_. `EventOffscreenAdjustment=-1` 
will calculate automatically but setting this to 200 or more might help any errors.

Custom `.ivt` file would allow for better layouts - they can be simple editted in 
notepad and are self explanatory with the exception of the Japanese comments 
after "//", whch you can ignore or remove. If you adjsut these layouts, set
`EventOffscreenTestValue` to the new range, or `0` to disable. 

### Settings
You can adjust the `recemoddar.ini` to indicate any backgrounds that have
been made widescreen and are 2x - this works well with some graphics,
but others are still covered by black letterboxing.

You can also adjust some offsets which might help with layout issues. 
letterboxed when set to `1` will try to honor letter boxing (largely 
untested currently).

Sample ini file
```
[General]
LetterBoxed=0
CharacterOffsets=32
EventOffscreenAdjustment=-1
EventOffscreenTestValue=340
EventVerticalOffset=-10

[WidescreenBackgrounds]
bmp/ivent/bg_guild.bmp=6
bmp/ivent/kuro.tga=8
bmp/ivent/bg_myroom.bmp=6
bmp/ivent/bg_myroom2.bmp=6
bmp/ivent/bg_myroom3.bmp=6
bmp/ivent/bg_hiro1.bmp=6
bmp/ivent/bg_hiro2.bmp=6
bmp/ivent/bg_hiro3.bmp=6
bmp/ivent/bg_kyoukai.bmp=6
bmp/ivent/bg_ichiba.bmp=6
bmp/ivent/bg_ichiba2.bmp=6
bmp/ivent/bg_sakaba.bmp=6
bmp/worldmap_night.bmp=6
bmp/worldmap_nomal.bmp=6
bmp/worldmap_yugata.bmp=6
bmp/boukenguild_bg.bmp=6
bmp/pause_bg_rete.tga=6

[Debug]
ImagesSearched=0
ImagesFound=0
EventFiles=0
```
_Only 2x graphics are currently supported for WidescreenBackgrounds._

#### Debugging messages
You can get debugging messages by setting `ImagesSearched=1` etc - youll need
to use something like [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview)
to see the debugging messages.

## BUGS!
Yes - proof of concept means bugs!
- `setDDTexture...` error happens because the patches cause memory leaks, save
  and restart your game! Thanks to "Just Harry"s memory sluething this takes longer
- `OUT_OF_MEMORY` see above - too late to save now!
- To allow external file loading the function at 0x00471b24 is ovveridden and
  uses 0x0047193C to load the files - these call a later function with param_6 
  set to 1 instead of 0... unsure what effect this will have!
- wide screen fix calculation might be off for anything other than 1920x1080