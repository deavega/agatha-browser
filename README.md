# Agatha Browser

A homebrew browser and YouTube client for the Nintendo Switch, in a single NRO.

Agatha Browser opens news sites and the wider web through the console's own browser,
and plays YouTube through a built-in client, so you never leave the app. It is built on
[Switch-NewPipe](https://github.com/mirusu400/switch-newpipe) by mirusu400, which
provides everything YouTube-related. See [CREDITS.md](CREDITS.md).

## Features

- **Bookmark cards** on the home screen. Five slots, each editable on the console with
  the system keyboard, saved to the SD card.
- **Any URL**, typed in with the system keyboard.
- **YouTube built in**: search, subscriptions, library and playback, with no ads and no
  Google account required.
- **YouTube links just work.** Tap a video link inside an article and that video opens
  in the player, because the system browser cannot run YouTube's site.
- **Screenshots and video capture** are enabled (see Limitations).

## Install

1. Download `AgathaBrowser.nro` from the
   [Releases](../../releases) page, or from the Actions tab if you build it yourself.
2. Copy it to `sdmc:/switch/AgathaBrowser/AgathaBrowser.nro`.
3. On the console, hold **R** while launching any game to open the Homebrew Menu, then
   start Agatha Browser.

Launching through a game (title override) matters: the system browser and screen
capture are only available to homebrew running in application mode.

## Controls

| Button | Action |
| --- | --- |
| D-Pad / Left Stick | Move between cards and buttons |
| A | Open the selected bookmark or button |
| X | Edit the selected bookmark (name, then URL) |
| − (Minus) | Reset the selected bookmark to its default |
| B | Back; from the home screen, quit |
| Touch | Tap a card or button directly |

Bookmarks are stored at `sdmc:/config/AgathaBrowser/bookmarks.txt`, one `name|url` per
line, so you can also edit them from a computer.

## Build

The build runs in Docker and produces the NRO. Nothing needs to be installed on the host
besides Docker and the devkitPro container the script pulls.

```bash
git clone --recursive https://github.com/deavega/agatha-browser.git
cd agatha-browser
./build.sh
```

The result is `cmake-build-switch/AgathaBrowser.nro`.

On macOS, build the resource helper inside the container first, because `build.sh`
otherwise builds a macOS binary that cannot run in the Linux container, and it also
calls `nproc`, which macOS does not have:

```bash
docker run --rm -v "$PWD:/work" -w /work devkitpro/devkita64 bash -lc '
  cmake -S vendor/borealis/library/lib/extern/libromfs/generator -B .build-libromfs-generator
  cmake --build .build-libromfs-generator -j4
  cp .build-libromfs-generator/libromfs-generator vendor/borealis/libromfs-generator'
```

You can also build it on GitHub: open the Actions tab and run **Agatha Browser Build**,
then download the `agatha-browser` artifact.

## Limitations

- The console's browser uses an old engine. Most news sites render fine; heavy web apps
  may not.
- Video capture also depends on the game you launch through supporting it.
- Capture may be blocked while the system browser is open. That is the browser's own
  behaviour, not Agatha's.

## License

GPL-3.0, inherited from Switch-NewPipe. See [LICENSE.MD](LICENSE.MD).

Not affiliated with Nintendo, YouTube or Google.
