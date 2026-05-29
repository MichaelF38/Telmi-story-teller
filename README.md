![Total Download](https://img.shields.io/github/downloads/DantSu/Telmi-story-teller/total.svg) [![v1.10.1 download](https://img.shields.io/github/downloads/DantSu/Telmi-story-teller/1.10.1/total.svg)](https://github.com/DantSu/Telmi-story-teller/releases/tag/1.10.1)

<p align="center"><img src="https://dantsu.com/files/Telmi_1280.png" alt="Telmi OS splash screen" /></p>

# Telmi - An open source story teller and MP3 player for Miyoo Mini

Telmi OS is an open source story teller and lite MP3 player for Miyoo Mini and Miyoo Mini Plus.
Telmi OS is for children 3~4 years old and older.

The story teller is compatible with stories exported from [STUdio](https://github.com/DantSu/studio).

[Learn more about Telmi](https://telmi.fr)

## Installation

Download the latest version of [Telmi Sync](https://telmi.fr/#download) and install Telmi OS on your SD card from Telmi Sync.

# Add stories and music on Telmi OS

<p align="center"><img src="https://dantsu.com/files/Telmi_MiyooPC.jpg" alt="Telmi OS - Telmi Sync" /></p>

Download the latest version of [Telmi Sync](https://telmi.fr/#download) and install it (Supported OS : Windows, MacOS, Linux).
Then plug SD card of Telmi OS on your computer, it will be reconized by Telmi Sync. You can now transfer stories and music to Telmi OS.

# Buttons roles

<p align="center"><img src="https://telmi.fr/img/miyoo-screen-stories-en.png" alt="Telmi OS - buttons roles selecting story" /><img src="https://telmi.fr/img/miyoo-screen-story-en.png" alt="Telmi OS - buttons roles reading story" /><img src="https://telmi.fr/img/miyoo-screen-music-en.png" alt="Telmi OS - buttons roles playing music" /><img src="https://telmi.fr/img/miyoo-screen-album-en.png" alt="Telmi OS - buttons roles selecting music album" /></p>

# Youtube videos

### Visitez notre chaîne Youtube pour découvrir et tout apprendre sur les fonctionnalités de Telmi

<p align="center"><a href="https://www.youtube.com/@Telmi-x2w" target="_blank"><img src="https://dantsu.com/files/Telmi_Youtube.png" alt="Vidéo d'installation et d'utilisation de Telmi OS et Telmi Sync" width="340" /></a></p>

# Discord

Tu parles français et tu veux discuter avec moi ? Demander de l'aide ? Poser des questions ?
Rendez vous sur le discord de [Telmi](https://discord.gg/ZTA5FyERbg).

# Local host build (Linux PC)

For testing the UI directly on Linux (no Miyoo Mini Plus required), a host build
is provided. It links against the system SDL2 libraries and stubs out the
hardware-only code paths (framebuffer, GPIO, `/dev/mi_ao`, rumble). Input is
read from the SDL window via the keyboard.

Install dependencies (Debian/Ubuntu):

```bash
sudo apt install build-essential pkg-config \
    libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev \
    libsdl2-mixer-dev libsdl2-gfx-dev
```

Build and run:

```bash
make host        # builds build_host/storyTeller, creates /mnt/SDCARD symlink (sudo)
make host-run    # runs it
```

`make host-setup` symlinks `/mnt/SDCARD` to the local `build/` directory so the
hard-coded resource paths work. Drop your `Stories/` and `Music/` content into
`build/Stories` and `build/Music` (which are exposed as `/mnt/SDCARD/Stories`
and `/mnt/SDCARD/Music`).

Keyboard mapping (matches the Miyoo Mini layout):

| Miyoo button | Key            |
| ------------ | -------------- |
| D-pad        | Arrow keys     |
| A / B        | Space / L-Ctrl |
| X / Y        | L-Shift / L-Alt|
| L1 / R1      | E / T          |
| L2 / R2      | Tab / Backspace|
| Start        | Enter          |
| Select       | R-Ctrl         |
| Menu         | Escape         |
| Volume -/+   | PageDn / PageUp|
| Power        | End (long-press > 1s exits) |

Clean the host build with `make host-clean`.
