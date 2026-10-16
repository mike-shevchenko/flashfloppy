# FlashFloppy

*The #1 floppy emulator.*

![CI Badge][ci-badge]
![Downloads Badge][downloads-badge]
![Version Badge][version-badge]

<img src="https://raw.githubusercontent.com/wiki/keirf/flashfloppy/assets/banner.jpg">

**FlashFloppy** is a floppy-drive emulator for the ubiquitous
[**Gotek**][Gotek-Compatibility] hardware. Connect to retro machines just
like a real floppy drive but use disk images on a modern USB stick!
- Say goodbye to old and unreliable floppy disks
- Download and play from the disk-image archives for your retro machines

**FlashFloppy** is the **#1 floppy emulator**:
- Supports a [massive range][Host-Platforms] of retro computers, synths, and machinery
- Directly reads and writes [many image formats][Image-Formats]
- [Flexible track layout][Track-Layouts] for Raw Sector Images
- [Extremely configurable][FF.CFG-Configuration-File]

**FlashFloppy** is **Free and Open-Source Software**.

## Download
- [**Download FlashFloppy**][Downloads]

## Documentation
- [**Read the GitHub Wiki**](https://github.com/keirf/flashfloppy/wiki)

## Apple2 mode

The multi firmware, in alt/multi/ of a release, serves an Apple2
host, an Agat or a Pravetz with a Disk II controller among them, and a
Shugart host as well: it watches the four stepper phase inputs for
200 ms at boot and switches to Apple2 mode when an Apple2 controller
drives them, else it works as the shugart firmware does. In Apple2
mode it serves 140K disks from HFE, .nib, .nic, .dsk, .do and .po
images. The shugart firmware has no Apple2 mode, and the apple2
firmware, in alt/apple2/, has that mode alone, with HFE images only.

Detected automatically:
- The host, at boot: Apple2 when at least three of the four phase
  inputs are low for a quarter of the window, Shugart otherwise.
- The wiring of Oleg Odintsov's Gotek firmware 307 for the Agat, with
  phase 1 on pin 32 of the floppy connector, as well as the standard
  one; not in the debug build for the STM32F105 and AT32F415.
- The sector order of a .dsk: ProDOS order if a ProDOS volume
  directory starts in block 2 where ProDOS order puts it, DOS 3.3
  order otherwise. A .do is always in DOS 3.3 order and a .po in
  ProDOS order, so renaming the file overrides a wrong guess.
- The 128-byte trailer that names the disk at the end of some Agat
  .dsk images, which is left as it is.
- Images that cannot be Apple2 disks, by their size alone: an HFE over
  1.25 MB and a sector image of other than 35 to 40 tracks are left
  out of the list, so 840K Agat disks do not show in Apple2 mode.
- An HFE whose header names an encoding that does not suit the mode,
  GCR in Shugart mode or any other in Apple2 mode, is refused.

Deliberately left out, as rare, or because the guess would be made
from the disk's contents with no way to override it:
- An HFE whose header leaves the encoding unknown, as Greaseweazle
  writes Apple2 images, is served in either mode; the logfile build
  notes when its first track looks like the other encoding. Telling
  the encoding from the data would misjudge damaged disks.
- A 160K PC image named .dsk is listed in Apple2 mode and served as
  noise.
- A sector image keeps no volume number: its sectors carry 254, the
  one DOS 3.3 formats with, and software that wants another fails.
- A sector image holds standard 16-sector tracks only. Half tracks,
  nonstandard sectors and other copy protection need .nib, .nic or
  HFE; a write to a sector image that is not a standard data field is
  dropped, with a "Bad CRC" line in the log.
- DOS 3.2's 13-sector disks are not supported.
- In Shugart mode .dsk keeps its usual meaning.

## Redistribution

Most code is public domain; the rest is MIT/BSD or Open Source friendly
(see the [COPYING](COPYING) file).

The source code, and all binary releases, are freely redistributable
in any form. Because FlashFloppy includes third-party code, you should
include or link to the [COPYING](COPYING) file in any redistribution.

This means, for example, you are free to sell and profit from Gotek
devices programmed with a binary release of FlashFloppy. However **at
a minimum** you should incorporate or link to the [COPYING](COPYING)
file on your selling page. For example:
- FlashFloppy is free software. For more information see the
  [license](COPYING).

[Gotek-Compatibility]: https://github.com/keirf/flashfloppy/wiki/Gotek-Compatibility
[Host-Platforms]: https://github.com/keirf/flashfloppy/wiki/Host-Platforms
[Image-Formats]: https://github.com/keirf/flashfloppy/wiki/Image-Formats
[Track-Layouts]: https://github.com/keirf/flashfloppy/wiki/Track-Layouts
[FF.CFG-Configuration-File]: https://github.com/keirf/flashfloppy/wiki/FF.CFG-Configuration-File
[Downloads]: https://github.com/keirf/flashfloppy/wiki/Downloads

[ci-badge]: https://github.com/keirf/flashfloppy/workflows/CI/badge.svg
[downloads-badge]: https://img.shields.io/github/downloads/keirf/flashfloppy/total
[version-badge]: https://img.shields.io/github/v/release/keirf/flashfloppy
