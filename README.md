# Octopath Traveler 2 Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)</br>
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/Octopath2Fix/total.svg)](https://github.com/Lyall/Octopath2Fix/releases)

This is a **work-in-progress** fix for ultrawide/narrower displays in Octopath Traveler 2.

## Caveats
- This fix is very much still a work in progress. **You will see bugs**.

## Features
- Removes pillarboxing at ultrawide or non 16:9 resolutions.
- Corrects FOV at ultrawide or non 16:9 resolutions.
- 16:9 centered HUD with fixed UI elements.
- 120 FPS cap removal.

## Installation
- Grab the latest release of Octopath2Fix from [here.](https://github.com/Lyall/Octopath2Fix/releases)
- Extract the contents of the release zip in to the game directory.<br />(e.g. "**steamapps\common\Octopath_Traveler2**" for Steam).

## Configuration
- See **Octopath2Fix.ini** to adjust settings for the fix.

## Linux/Steam Deck
- Make sure you set the Steam launch options to `WINEDLLOVERRIDES="d3d11.dll=n,b" %command%`
<img src="https://user-images.githubusercontent.com/695941/226513105-e2aedf8f-d596-4ffb-a121-ac020d9e867f.jpg" width="646" height="113" />

## Known Issues
Please report any issues you see. (**Check the known issues list first!**)
- Some cutscene fades may be 16:9 and not fill the entire screen.

## Screenshots

| ![ezgif-1-adaa3f0c3c](https://user-images.githubusercontent.com/695941/226225010-073a0360-98bb-4e88-bfef-bcdf3f74abae.gif) |
|:--:|
| Disabled pillarboxing. |

## Credits
[Flawless Widescreen](https://www.flawlesswidescreen.org/) for the LOD fix.<br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini parsing. <br />
[Loguru](https://github.com/emilk/loguru) for logging. <br />
[length-disassembler](https://github.com/Nomade040/length-disassembler) for length disassembly.
