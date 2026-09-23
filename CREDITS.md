# Credits

Agatha Browser is built on other people's work. This file records what it uses and who
wrote it.

## Switch-NewPipe

Everything YouTube in this app — search, subscriptions, library, the video pages and
playback — comes from [Switch-NewPipe](https://github.com/mirusu400/switch-newpipe) by
**mirusu400**, released under GPL-3.0. Agatha Browser is a modified version of that
project: it adds a home screen, bookmark cards and the system web browser, and it wires
YouTube links into the existing player.

Because Agatha Browser contains that code, it is also released under GPL-3.0.

Changes made relative to upstream Switch-NewPipe:

- Added the Agatha Browser home screen (`src/activity/agatha_activity.cpp`,
  `resources/xml/activity/agatha.xml`) with editable bookmark cards.
- Added the Nintendo Switch web applet as the browser for non-YouTube pages, with
  YouTube links routed to the built-in player instead.
- Fixed the sidebar width: the XML attribute was applied only when set before
  `sidebarPosition`, so the sidebar stayed at its default width and tab labels were
  cut off.
- Added the Rubik font and a `RubikLabel` view for the home screen.
- Enabled screenshots and video capture.
- Rebranded the app name, icon and build output.

## Libraries

- [borealis](https://github.com/xfangfang/borealis) — the UI framework (via Switch-NewPipe)
- [libnx](https://github.com/switchbrew/libnx) and [devkitPro](https://devkitpro.org/) —
  Switch toolchain, and the web applet used for browsing
- [mpv](https://mpv.io/) and [FFmpeg](https://ffmpeg.org/) — video playback
- [lunasvg](https://github.com/sammycage/lunasvg), [QuickJS](https://bellard.org/quickjs/),
  and the other dependencies vendored by Switch-NewPipe

## Assets

- **Rubik** by Hubert and Fischer, Meir Sadan and contributors, licensed under the
  SIL Open Font License 1.1.
- App artwork: original artwork for this project.

## Not affiliated

This project is not affiliated with, endorsed by, or connected to Nintendo, YouTube or
Google. NewPipe (the Android project that inspired Switch-NewPipe) is a separate project
and is not involved in this one either.
