# GIMP Smart Sharpening
Smart Sharpening plug-in for GIMP

## Description
This plug-in implements a smart sharpening filter for the edges and a noise reduction filter for the rest of the space. It implements the following operations step by step:
1. Optionally changes the saturation of the image
2. Decomposes the image into LAB channels
3. Uses the L channel to create a sharpening mask using the following operations:
   - Detects edges
   - Applies the Gaussian blur filter to detected edges
   - Uses blurred edges as a mask
4. Selects an area in the L channel according to the resulting mask
5. Reduces noise in the space outside the selection
6. Applies sharpening to the selected space
7. Composes the image from LAB channels
## Interface
![Plug-in Dialog](https://github.com/v-lavrentikov/gimp-smart-sharpening/assets/2562499/a3537597-8e00-436c-ac79-c8034b4a4058)
## Compilation
This repository contains `Makefile` for building the plug-in. It contains two options:
- `gimptool` builds the plugin using the gimptool utility. This method can also be used for macOS with GIMP installed from a package manager such as `macports` or `homebrew`
- `macos` builds the plugin for macOS when GIMP is installed from the `*.dmg` package. But this method still requires the `gimp2-devel` package installed from a package manager such as `macports` or `homebrew`
## Project Support
You can support this project by donating to the following Ethereum wallet:

ethereum:0x0468DcdE81b69b87ea0A546faA6c5aae2F4FE30b

![ethereum:0x0468DcdE81b69b87ea0A546faA6c5aae2F4FE30b](https://github.com/v-lavrentikov/gimp-smart-sharpening/assets/2562499/0351dc36-dfa2-46af-9dac-19cd145a297e)

