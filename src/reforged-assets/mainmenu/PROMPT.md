# Main menu artwork (2026-09-10)

Background generated with the built-in imagegen tool. The supplied mainmenu.png
mockup guided composition. Reference-image access failed in the image tool's
filesystem sandbox, so the final generation used the text description below.
No menu labels, logo or interactive affordances are baked into the background.
Logo and material template reuse the accepted server UI v65 assets.

## Generation prompt

Generate a 16:9 photorealistic WWII videogame main menu BACKGROUND ONLY.
Overhead cinematic still life on an old dark wooden table: a worn bolt-action
rifle occupies leftmost quarter vertically, a leather map case left-center,
folded cream North African military map across center with tiny natural map
printing TOUJANE, open brass compass upper middle, scattered brass rifle
cartridges lower center-left, small faded black-and-white photograph of soldiers
lower middle, dark notebook and dog tags upper center-right. Warm muted sepia,
realistic aged tactile materials, dark gold highlights, deep charcoal shadows.
Entire right 40 percent very dark quiet negative space with only faint table
texture for interactive menu overlay. Scene edge to edge. NO logo, NO UI, NO
buttons, NO captions, NO slogans, NO borders or divider lines. High resolution
2560x1440. Historical WWII equipment only.

## Runtime conversion

The reviewed builder resizes to power-of-two native IWI textures with Lanczos:
background 2048x1024, logo 512x256. Native fullscreen rects restore the intended
16:9 scene and 2:1 logo proportions at the reference aspect ratio. Existing CoD2
fullscreen scaling follows the display at other aspect ratios. No video or
per-frame decoding is involved. No generated image metadata enters the IWD.
