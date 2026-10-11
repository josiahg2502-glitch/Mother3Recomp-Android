# Emerald Widescreen Experiment 0.2.0

Requires **EmeraldRecomp v0.0.6 or later** and the original USA Emerald ROM
(SHA-1 `f3ae088181bf583e55daf962a92bb46f4f1d07b7`). This data-only package
selects the trusted renderer compiled into that executable. Earlier game
releases do not contain the renderer; installing this package cannot add it.

In the launcher, open **Mods**, enable **Overworld Widescreen (Experimental)**,
choose **Fit to window**, **16:9**, **21:9** or **32:9**, then apply and play.
The package is already bundled with v0.0.6. The separate `.gbamod` archive can
also be installed through the launcher's mod installer. It starts disabled.

This expands overworld scenery and NPCs at the original pixel scale.
Native gameplay and saves are preserved. The renderer
fills portrait windows in Fit mode, retains scenery during door
animations, keeps connected-map scenery visible across route boundaries,
and anchors overworld menus to the viewport edges. The released
v0.0.5 executable supports horizontal expansion only and cannot enable this
package's newer renderer. Update the game executable as well as the mod.
Battles and unsupported scenes remain centered at their native 3:2 aspect.
Unloaded NPCs in the current and connected maps remain visible as idle poses
at their map or last observed positions. Walking, trainer activation and
scripts keep the game's normal active area. Story flags and live actors take
precedence; special field effects and disguised actors are not expanded.
Neighboring scenery uses compatible tilesets already loaded by the game;
NPC previews likewise require a compatible resident palette. No extra actors
are inserted into the game's simulation.

No ROM, BIOS, save, game graphics or native library is included in this package.
Licensed under PolyForm Noncommercial 1.0.0; see LICENSE in this package.
