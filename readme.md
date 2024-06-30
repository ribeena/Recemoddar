# Recemoddar
A proof of concept modder tool for Recettear. This hooks a patched Recettear
to allow for loading files from a `/mods/` folder in your recettear directory.

## Getting started
You must have installed https://github.com/just-harry/FancyScreenPatchForRecettear to
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

You won't see anything until you have repalcements in `mods` directory!

## What can it do?

### .x files replacements
You can place `.x` files into `/mods/xfile/` or `/mods/xfile2/` and it will
load those instead. Refer to https://github.com/ribeena/RecettearXTools for tools
to help with modding the `.x` files of Recettear.

### 3d model images up to 4 times the size
Using https://github.com/UnrealPowerz/recettear-repacker you can extract the
expected `.tga` and `.bmp` files, these generally go into `/mods/xfile/`,
and add either `_2x` or `_4x` to the end of the file to indicate the upscaled
amount.

Recettear seems to support `.dds` files for this, so it'll check for those too.
DDS DXT1 seems to work fine for `.bmp` and DDS DXT5 for `.tga`, but more testing
could be done - these will significant reduce fileszie but may affect quality.

### UI image replacement
Using the same format as above, adding `_2x` on any UI graphics mostly work.
You can also include images without the `_2x` if just replacing the graphics.

#### Widescreen backgrounds
You can adjust the `recemoddar.ini` to indicate any backgrounds that have
been made widescreen and are 2x - this works well with some graphics,
but others are still covered by black letterboxing.

Sample ini file
```
[WidescreenBackgrounds]
bmp/ivent/bg_guild.bmp=6
bmp/ivent/bg_hiro1.bmp=6
bmp/ivent/bg_hiro2.bmp=6
bmp/ivent/bg_hiro3.bmp=6
bmp/worldmap_night.bmp=6
bmp/worldmap_nomal.bmp=6
bmp/worldmap_yugata.bmp=6
bmp/boukenguild_bg.bmp=6
```
_Only 2x graphics are currently supported for this._

## BUGS!
Yes - proof of concept means bugs!
- `setDDTexture...` error happens because the patches cause memory leaks, save
  and restart your game! Thanks to "Just Harry"s memory sluething, could be fixed!
- `OUT_OF_MEMORY` see above - too late to save now!
- Letterboxing covers widescreen backgrounds
- Characters sit too high at times