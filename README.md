# Octopath Traveler 2 Fix
[![Patreon-Button](https://github.com/Lyall/Octopath2Fix/assets/695941/c6769afd-e0f9-41d0-88aa-0de18ed0f908)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/Octopath2Fix/total.svg)](https://github.com/Lyall/Octopath2Fix/releases)

This is a fix for ultrawide/narrower displays in Octopath Traveler 2.

## Features
- Removes pillarboxing at ultrawide or non 16:9 resolutions.
- Corrects FOV at ultrawide or non 16:9 resolutions.
- 16:9 centered HUD with fixed UI elements.
- 120 FPS cap removal.

## Installation
- Grab the latest release of Octopath2Fix from [here.](https://github.com/Lyall/Octopath2Fix/releases)
- Extract the contents of the release zip in to the the game folder. e.g. ("**steamapps\common\Octopath_Traveler2**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="d3d11=n,b" %command%` to the launch options.

## Configuration
- See **Octopath2Fix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

- "Porthole" view through buildings is offset. (Thanks THE-GaYmer for pointing this out!)
- In battle, cursors and damage numbers may be somewhat offset.

## Screenshots

| ![ezgif-1-adaa3f0c3c](https://user-images.githubusercontent.com/695941/226225010-073a0360-98bb-4e88-bfef-bcdf3f74abae.gif) |
|:--:|
| Disabled pillarboxing. |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
