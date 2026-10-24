![ffemu on Ubuntu](https://github.com/mike-shevchenko/flashfloppy/releases/download/ffemu-v1.4/ffemu-ubuntu-screenshot.png?v=1)

Firmware for Gotek floppy emulators - **multi-mode for Apple2 / TR-DOS / IBM PC** hosts. Includes **ffemu** - a PC app emulating a Gotek: see how your USB drive will work on a real Gotek.

This is a fork of the original [FlashFloppy repo on GitHub](https://github.com/keirf/flashfloppy)

The features of this fork are often accepted into the FlashFloppy upstream, but can appear in stable releases with a delay, so enjoy the [releases of this fork](https://github.com/mike-shevchenko/flashfloppy/releases)

These releases can be easily identified by the letter `m` after the firmware version, both in the file names and on the Gotek display.

Here is [the original FlashFloppy readme on GitHub](https://github.com/keirf/flashfloppy)

---------------------------------------------------------------------------------------------------
## What's new

- **The "multi" firmware**, in `alt/multi/`. It auto-detects between the Apple2 and the regular modes - it watches the four stepper phase inputs for 200 ms at boot and switches to Apple2 mode when an Apple2 controller drives them. So one firmware serves a Shugart host, such as a ZX Spectrum with a Beta Disk interface, and an Apple2 host, such as an Agat or a Pravetz with a Disk II controller, with zero configuration. The main firmware and the separate Apple2 firmware, in `alt/apple2/`, are the upstream's, but for the small fixes below; all the new Apple2 features are in the multi firmware alone.
  - **Images:** HFE as before, and now also `.nib`, `.nic`, `.dsk`, `.do` and `.po`, all for read and **write.** A `.dsk` is taken in ProDOS order when a ProDOS volume directory starts in its block 2, in DOS 3.3 order otherwise; renaming it to `.do` or `.po` overrides the guess.
  - Images whose size no Apple2 disk has are left out of the list in Apple2 mode, and an HFE whose header names an encoding that does not suit the mode is refused.
  - **Both wirings of the phases** are taken: the standard one, and that of Oleg Odintsov's Gotek firmware 307 for Agat, with phase 1 on pin 32 of the floppy connector (the `gotek-sa390` project of https://svn.code.sf.net/p/agat-hardware/code).
  - **The displays show the stepper phases** beside the track: a spinner on the OLED, a square whose sides are the phases, so that a seek turns it round; the phases as digits on a character LCD; the lower segments of the first digit on a 3-digit LED display.
  - See the wiki's [Apple II](https://github.com/mike-shevchenko/flashfloppy/wiki/Apple-II) page for what is detected and what is left out on purpose. A quickstart guide for wiring a Gotek is given below.
- **The Apple2 bootloader as an update file**, for the AT32F415 in QFN32, such as the SFRKC30.AT2: `alt/apple2/at2-bootloader/flashfloppy-apple2-at2-bootloader-3.44m1.upd`, see below.
- **The version on a 7-segment display**, shown when the right button is pressed and released while no USB drive is mounted, presents all version details sequentially, in every firmware.
- **An unknown interface value in flash**, as Oleg Odintsov's firmware 307 writes for its `sa390` interface, is taken as `jc` by every firmware, which crashed on it before.
- **ffemu**, the emulator of the firmware's user interface for a PC: a third program, `ffemu-multi`, runs the multi firmware, and its FDD type dialog (key 8) makes it drive the floppy interface as a Shugart or an Apple2 host does. The Flash mem dialog (key 9) has a new button, **Save diff-from-defaults to FF.CFG**, which writes the options that differ from the defaults or from flash.
- **Unit tests** of the image handlers and of the floppy code, built and run on a PC by the plain `make`, in `tests/`.

---------------------------------------------------------------------------------------------------
## What this fork has that upstream lacks

### Kept on purpose, refused upstream

- **The version on 7-segment displays laid out at build time**, without decimal points: on 3 digits the letter part, such as `m1`, in one frame and dashes where a number leaves a digit free (` 3-`, `-44`), on 2 digits the `m` drawn across both. Offered as [#1084](https://github.com/keirf/flashfloppy/pull/1084); upstream chose its own, simpler change, [display: Improve printing of FF version number on 7-seg displays](https://github.com/keirf/flashfloppy/commit/f838a608).
- **Link-time optimization (LTO) in the STM32F105/AT32F415 logfile and debug builds**, which frees several hundred bytes where room is shortest, room the fork's own features need. Offered in [#1098](https://github.com/keirf/flashfloppy/pull/1098); upstream does not want LTO ("I don't want LTO"). It is in the branch `lto` until it is tested on a board with a host, not in a release yet.

### Not upstream yet, offered or waiting

Some of these are not in a release yet: they come with the next one.

- **The multi firmware**, described above, with its Apple2 images and both wirings of the phases. Its pull request follows once upstream has taken the smaller ones it builds on.
- **The open pull requests**: none at the moment; more to come.

---------------------------------------------------------------------------------------------------
## Installing

Connect the pins marked by the same labels on the diagrams below with Dupont wires or jumpers. The connections are valid for all of the Apple2, Shugart (e.g. TR-DOS) and IBM PC modes.

### Prepare Gotek model SFRKC30.AT2 (MCU 415)

Move the encoder's button from the KC30 header to jumper JA (which is Select too).

```
             J4          J5  JA  JC      S0             Encoder (view to the terminals):
---------------------   -----------------------------     . . .
| .   .   C   D   . |   | .  E1   .   .   S   .   . |    |^-^-^|
-----               |   |                           |   [|     |] 
    | .   .   .   . |   | D  E2   .   .   S   .   C |    |v-v-v|
    -----------------   -----------------------------    E1 . E2 
```

**An AT32F415 in QFN32, such as the SFRKC30.AT2, with an Apple2 host** needs the Apple2 bootloader, with the multi firmware as with the Apple2 one. Pin PA10, the KC30 header's Select pin, carries stepper phase 0 there, and the standard bootloader takes it for a held Select button, so at every power-on it stops in its update mode, showing "UPD" or "FF Update Flash".

To install the bootloader, unplug the host, put `alt/apple2/at2-bootloader/flashfloppy-apple2-at2-bootloader-3.44m1.upd` on the USB drive as its only `.upd` file and hold both buttons at power-on. This is needed only once.

NOTE: A power cut while the bootloader is being replaced leaves the board to be programmed over USB or a serial cable ([Firmware Programming](https://github.com/keirf/flashfloppy/wiki/Firmware-Programming)), from the `.dfu` or `.hex` in the same folder. The Apple2 bootloader serves a Shugart host as well; it only stops reading the KC30 header's Select pin.

In order to make such Gotek work with IBM PC, either the JC jumper must be installed (ATTENTION: the JC jumper may damage the Apple2/Agat), or the setting `interface = ibmpc` must be written to the flash memory: put `FF.CFG` with this line on a USB drive and power on the Gotek, then you can remove this file. Note that TR-DOS will work fine with or without JC and with or without this setting. Note that any `FF.CFG` that sets `interface =` will write the specified value back into flash, and the Factory Reset will also delete the setting.

### Prepare Gotek model SFRKC30.AT4.35 (MCU 435)

No special bootloader is needed. No encoder moving is needed. The JC jumper can be installed for all the mentioned modes; there is no need to write `interface = ibmpc` into flash then.

```
             J4          J5  JA  JC      S0          
---------------------   -----------------------------
| .   .   C   D   . |   | .   .   J   .   S   .   C |  Note the differences from 415:
-----               |   |                           |  - different position of C
    | .   .   .   . |   | D   .   J   .   S   .   . |  - added JC (J-J)
    -----------------   -----------------------------  - no encoder reconnection
```

### Cable for Apple2

A simple cable for one drive can be made from a 14 to 34 wire ribbon cable (some wires can be unused) with IDC-20 connector on the controller's side, and IDC-34 on the Gotek side.

For connecting multiple drives, see the details on the upstream wiki page [Apple II](https://github.com/keirf/flashfloppy/wiki/Apple-II).

```
Controller side, view to the pins at the Controller:
-----------------------------------------  
|PH0 PH1 PH2 PH3 WRQ  .  DE  RD  WR  WRP|  
|GND GND GND GND  .  +5   .   .   .   . |  
--------^----------___--------------^----

Gotek side, view to the pins at Gotek:
-----------------       ---------------------------------------------------------------------
| .   .   .  +5 |       | .  PH0  .   .  DE   .   .  PH1 PH2 PH3 WR  WRQ  .  WRP RD   .   . |
=======---=======       | .   .   .  GND GND GND GND  .   .   .   .   .   .   .   .   .   . |
                        --------^------------------------___----------------------------^----
```

### Cable for Agat

A simple cable for one drive can be made from a 12 to 34 wire ribbon cable with SNP34-30V (male) on the controller side (it can be soldered to, or non-loose Dupont wires can be put on its tail pins), and IDC-34 on the Gotek side.

Alternative - if you have an original Agat FDD cable, you can match it with an FDC-30 (ribbon cable connector for PCB soldering) by pushing FDC's tail pins into the female connector on the cable, and crimping both sides of the ribbon cable. The edge pairs of FDC's tail pins can be soldered short to prevent misalignment when mating.

```
Controller side, view to the tail pins of the connector on the cable:
 _-------------------------------------_  
|GND  .   .  PH0 PH1 PH2 PH3 +5   .   . |
| .   .   .   .   .   .   .   .   .   . |
|RD  WPR  .  WRQ  .   .   .  DE  WR  GND|
-----------------------------------------

Gotek side, view to the pins at Gotek:
-----------------       ---------------------------------------------------------------------
| .   .   .  +5 |       | .  PH0  .   .  DE   .   .  PH1 PH2 PH3 WR  WRQ  .  WRP RD   .   . |
=======---=======       | .   .   .  GND GND  .   .   .   .   .   .   .   .   .   .   .   . |
                        --------^------------------------___----------------------------^----
```

For connecting multiple drives, see the details in the specialized adapter board project `agat-disk-hub`: https://github.com/mike-shevchenko/agat-disk-hub

### Install the firmware

**The multi firmware:** `alt/multi/flashfloppy-multi-3.44m1.upd` from the zip, as any FlashFloppy update ([Firmware Update](https://github.com/keirf/flashfloppy/wiki/Firmware-Update)), for the STM32F105, the AT32F415 and the AT32F435: put the only `.upd` file on the USB drive and hold both buttons or press and hold the encoder at power-on. This will install the firmware. Its logfile build is in `alt/multi/logfile/`.

---------------------------------------------------------------------------------------------------
## ffemu

- **Linux** (x86-64, Ubuntu 22.04+): `ffemu-3.44m1-linux-x86_64.tar.gz`, `ffemu-apple2-3.44m1-linux-x86_64.tar.gz`, `ffemu-multi-3.44m1-linux-x86_64.tar.gz`
- **Windows** (Git Bash, MSYS2 or Cygwin): `ffemu-3.44m1-windows.exe`, `ffemu-apple2-3.44m1-windows.exe`, `ffemu-multi-3.44m1-windows.exe`

        ffemu [ <directory> | <usb-drive-image> | /dev/sd<usb-drive-dev>1 ]
        ffemu-apple2 [ <directory> | <usb-drive-image> | /dev/sd<usb-drive-dev>1 ]
        ffemu-multi [ <directory> | <usb-drive-image> | /dev/sd<usb-drive-dev>1 ]

**Without an argument, the physical USB drive** attached is taken. `ffemu` runs the main firmware, `ffemu-apple2` the separate Apple2 one and `ffemu-multi` the multi one. See [ffemu/README.md](https://github.com/mike-shevchenko/flashfloppy/blob/v3.44m1/ffemu/README.md) for the details.
