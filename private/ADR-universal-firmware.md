# ADR: universal FlashFloppy firmware with an auto-detected Apple2 mode

// Working document of the fork, kept on the `private` branch in `private/`; not part of any pull request.

Status: accepted 2026-10-05; step 1 done 2026-10-06; since 2026-10-08 ffemu runs the firmware's own floppy interface code, a test bench for steps 2 and 3 (D10); step 2 coded and tested in ffemu 2026-10-08, on the branch `apple2-auto`, its detection confirmed on the Agat the same day, the Pentagon and the write test still to do; step 3 coded 2026-10-08 on `apple2-307`, untested on hardware. Baseline: upstream master 483077a (v3.44 + 4 commits, 2025-12-15), fork `mike-shevchenko/flashfloppy`, work tree `C:\github\forks\flashfloppy\`.

## 1. Context

Upstream FlashFloppy builds three firmwares from one tree, chosen at compile time by `TARGET`: `shugart` (the main one), `apple2-mode` and `quickdisk`. The Apple2 firmware appeared in v3.43 (a32be60, +182 lines in 7 files) and is the Shugart firmware with about a dozen `#if TARGET == TARGET_apple2` sites and only the HFE image handler. A Gotek moved between an Apple-compatible host (here: Agat-7 with the 140K controller) and a Shugart host (here: Pentagon) has to be reflashed each time, and nothing on the display tells which firmware is installed.

Oleg Odintsov's "307" firmware (agat-hardware SVN, `gotek-sa390/`) solved the same problem in January 2024 with a runtime mode `interface = sa390`. It is based on upstream master 69e2c54 (2023-10-29, 15 commits after v3.41), differs from it by about 690 diff lines in 12 files plus `src/image/nib.c` (158 lines), and uses its own wiring: PH0 on PB14 (PA10 on QFN32) and PH1 on IDC-34.32.

Hardware in hand: a Gotek SFRKC30.AT4.35 (AT32F435, LQFP64), working, the test device for now; a Gotek SFRKC30.AT2 (AT32F415, QFN32) with a dead U1 inverter, to be repaired and tested later.

Goal: one firmware for both MCU families that behaves as today's Shugart firmware on a Shugart host and as today's Apple2 firmware on an Apple2 host, with no reflashing and no configuration, and that later absorbs the useful parts of 307.

## 2. Decisions

### D1. Build on upstream; the shugart target becomes the universal one

The work is done on upstream master, not on Oleg's tree. The `shugart` target gains the Apple2 mode at run time. The `apple2-mode` and `quickdisk` targets keep building and keep their behavior: `apple2-mode` stays as the same code with the mode fixed at compile time. That keeps the pull request additive, gives upstream a fallback, and gives us an A/B reference built from the same source.

No field is added to `struct ff_cfg`, so the flash configuration layout does not change.

### D2. One predicate replaces the compile-time tests

A single predicate, constant in the `apple2-mode` and `quickdisk` targets and a variable in the `shugart` target, replaces `TARGET == TARGET_apple2` at these sites:

- `inc/floppy.h`: `WDATA_TOGGLE`; `in_da_mode()` (no direct-access mode in Apple mode).
- `src/floppy.c`: WRPROT polarity in `drive_change_output()`; JC strap handling (see D4).
- `src/floppy_generic.c`: RDATA pulse width, 1000 ns against 400 ns; the TMR1_CC interrupt enable.
- `src/gotek/floppy.c`: phase pins and `POLL_step()` against the TIM2 STEP interrupt and `IRQ_STEP_changed()`; `floppy_ribbon_is_reversed()`; pin configuration in `board_floppy_init()`.
- `src/gotek/board.c`: the QFN32 Select button on PA10 in `board_get_buttons()`; pull-ups on PA9 and PA10.
- `src/image/image.c`: the image type table, `image_valid()` and `image_open()` (HFE only in Apple mode, as upstream does; without this an .img on the stick would be served to an Apple host as MFM). An HFE encoding probe at mount, with a size filter in Apple2 mode, is written and parked on the branch `hfe-probe` since 2026-10-08: it does not fit the STM32F105/AT32F415 debug build. HFE stays the one format both hosts share, so an HFE of the wrong kind is served as noise until it is taken in.

Known trap: `exti_irqs[]` is a static table and `floppy_init_irqs()` deliberately fires each listed interrupt once. With both the TMR1_CC entry (Apple) and the TMR2 entry (Shugart) linked in, the entry of the inactive mode must be skipped, or `IRQ_wdata_capture()` would flip the capture polarity once in Shugart mode.

### D3. Mode is auto-detected at boot from the four phase inputs

The four inputs that carry the phases in upstream's Apple2 wiring are sampled once per millisecond during the existing 200 ms "5v settle" delay in `main()`, with the internal pull-ups on PA9 and PA10 enabled first. The mode is Apple2 if and only if every one of the four lines was low in at least a quarter of the samples. The detection adds no boot time and runs before `floppy_init()` and before the first `board_get_buttons()`.

What each input is, by installation:

| Input | Apple2 wiring | Shugart, LQFP48 and LQFP64 | Shugart, QFN32 |
|---|---|---|---|
| PA10 | PH0 | unconnected, pulled up | KC30 Select: low while the encoder is pressed |
| PA9 | PH1 | unconnected, pulled up | JC strap: low when the jumper is fitted |
| PB0 | PH2 | DIR: any static level | DIR: any static level |
| PA1 | PH3 | STEP: high, low only in short pulses | STEP: high, low only in short pulses |

Why this is sound:

- An Apple2 host holds all four phases low at power-on. Verified for the Agat 140K controller in the agat-fdc-140-105 board file: the phase flip-flops D6 and D21 have /S on ~Resetx and the phases are taken from /Q. For the Disk II card (74LS259 cleared by its power-on circuit) this is from memory and not verified.
- During a seek at most two adjacent phases are on, so each line is still low at least half of the time; during the boot ROM's recalibration, three quarters of the time. The quarter threshold therefore also survives a firmware reset in the middle of a seek.
- On LQFP boards nothing can pull PA9 or PA10 low in a Shugart installation, so a false Apple2 detection is not possible there.
- On QFN32 a false Apple2 detection needs the encoder held, the JC jumper fitted, DIR low and STEP low for a quarter of the window, all at once. STEP pulses are microseconds long, so in practice this is only a reversed ribbon, where nothing works anyway and the only loss is the "ribbon reversed" message.
- A false Shugart detection needs a phase stuck high for three quarters of the window, or the cable plugged in after power-on.

The sample counts are logged with `printk()`, so a logfile build shows the margin on real hardware. Window length and threshold are tunables to confirm in step 2.

The detected mode is not carried across `system_reset()`; every boot detects again. Revisit only if testing shows a wrong detection after a self-reset.

Limitation: in `level=debug` builds PA9 and PA10 are the serial console and upstream moves PH0 and PH1 to the rotary pins PA6 and PA15. Detection there uses those pins and is weaker, since an encoder can rest with both contacts closed; a false Apple2 then still needs DIR and STEP low for a quarter of the window. When it finds an Apple2 it clears `has_kc30_header`, so the KC30 encoder is off, as in upstream's apple2 debug build; an encoder on the original LQFP64 header keeps working. A debug build must be wired that way: with the standard wiring, PH1 would drive against the console's TX and PH0 pulses on RX would trigger the crash-on-input of the debug console. Debug builds are a developer tool; diagnostics on the real wiring come from `level=logfile` builds. Considered and set aside on 2026-10-08: sensing a serial adapter at boot (PA10 pulled down stays high only with an adapter's idle TX on it) and choosing the pin set by that, or compiling logging into every build behind a run-time flag, which would cost the STM32F105 and AT32F415 some 6.5 KB of the 8.9 KB left for the 307 features.

### D4. No configuration key

Auto-detection is the only means of selecting the mode. In Apple2 mode, `interface = jc` resolves to Shugart without reading the JC strap, which on QFN32 is PA9 = PH1; pins 2 and 34 are not connected on an Apple cable, so this is not observable. Everything else in FF.CFG keeps its upstream meaning.

Fallback, recorded in case testing or upstream review demands a switch: the existing key, `interface = apple2`, persisted like any other value and applied by one `system_reset()` when it changes, as `display-type` is applied today.

### D5. Wiring schemes

Step 2 reads exactly upstream's pins (PA10, PA9, PB0, PA1). That covers two of the three schemes with no extra code:

- Standard (FlashFloppy wiki): PH0 to RX, PH1 to TX, PH2 to 18, PH3 to 20.
- Combined (2026-10-05 mapping): as standard, plus PH1 also on IDC-34.32 and, on the 435, PH0 also on PB14 (U8 pin 2, shorted to RX). The extra connections land on inputs: pin 32 is SIDE, which is masked for one-sided images; PB14 is an idle input on standard boards.

The 307-only scheme (PH0 on PB14, or PA10 on QFN32; PH1 on IDC-34.32 only) arrives in step 3. The owner's adapter board of 2026-10-08, between the Agat's IDC-20 and the Gotek's IDC-34, is the combined scheme on one connector: PH0 on IDC-34 pin 4, PH1 on 16 and again on 32, PH2 on 18, PH3 on 20, DE on 10, WR, WRQ, WRP and RD on 22, 24, 28 and 30, pin 5 a ground that can be pulled as a key; pins 4 and 16 reach RX and TX by wires on the Gotek. It serves stock apple2, 307 and the combined firmware alike. ffemu's Apple2 type drives exactly that at the MCU since step 3, hard-coded; the 307-only scheme is not emulated, since 307 is meant to fade once this firmware is out. It is detected at the same moment by the same rule applied to the alternative pin set, tried only when the standard set does not match. The two sources are selected, never ANDed: with standard wiring pin 32 may carry HDSEL from a IIgs through a DB-19 adapter, and an AND would then kill PH1. In 307 wiring the head is forced to side 0, since pin 32 is a phase. 307 wiring is accepted only on standard boards; on the enhanced boards PB14 is the SD card's MISO.

### D6. Bootloader is not touched

On QFN32 the stock bootloader reads PA10 low as a held Select button and stays in update mode, so the AT2 board keeps needing upstream's apple2 at2-bootloader, exactly as today. The 435 is unaffected. A universal at2 bootloader that applies the D3 rule is a possible follow-up, outside these branches.

### D7. The detected mode is visible

With no key, the display is the only way to see what was detected. Done in step 2: the LCD/OLED banner gains the suffix " A2" after the version in Apple2 mode, in the manner of the existing " QD" and " Log" suffixes (so `483077a A2  32kB` on the second line); the 7-segment banner shows "A2", or "A-2" on three digits, in place of "FF" and "F-F" (upstream's middle character on three digits tells the MCU: "-" for the STM32F105 and AT32F415, "z" for the AT32F435). The logfile build shows " A2L" and "A2L" in place of " Log" and "LOG" (the owner's choice of 2026-10-08), and its log says `Apple2 mode: on (phase inputs low 200/200/200/200 of 200)`. The "2" follows ffemu's "Apple2".

### D8. Build environment

Builds run on the Ubuntu 22.04 VM, the same release as upstream CI. Sources stay on Windows and are edited there; the VM sees them as `/c/github/forks/flashfloppy/` (vmhgfs). Two directories live on the VM, both visible from Windows under `U:\retro\`:

- `~/retro/flashfloppy-build/` holds nothing but the build and may be deleted at any time for a clean rebuild.
- `~/retro/flashfloppy-tools/` holds the tools, installed without root and without touching the system; deleting it uninstalls them.

`private/set_up_tools.sh` fills the tools directory and can be rerun:

- Arm's GNU Arm Embedded Toolchain 10.3-2021.07 tarball, the same release as jammy's `gcc-arm-none-eabi` 15:10.3-2021.07-4 that CI uses. It is fetched from Arm's storage host armkeil.blob.core.windows.net, because developer.arm.com does not resolve from this network, and checked against a pinned SHA-256.
- Ubuntu packages `srecord`, `libsrecord0`, `python3-crcmod` and `python3-intelhex`, fetched with `apt-get download` and unpacked with `dpkg -x` into `sysroot/`, not installed.
- `env.sh`, which puts all of it on `PATH`, `LD_LIBRARY_PATH` and `PYTHONPATH`.

`build.sh`, at the top of the tree, builds the tree it belongs to, as an out-of-tree build: `make -C <source tree> O=<build directory>`, so everything the build writes lands in `out/` and `ext/` of the build directory and the source tree is only read. With no arguments it makes `dist`, the complete release tree in `out/flashfloppy-<hash>/`. The `O=` variable is our addition to the upstream build system, which builds only into the source root: `Makefile` takes `$(O)/out` and `$(O)/ext` for its two directories, defaulting to the source root, and `scripts/srcdir.py` returns an absolute source path when the object directory is outside the tree and the same relative path as before when it is inside. It went into the `fixes` pull request, #1073, on 2026-10-07. Two earlier layouts were rejected: a build directory of symbolic links to the source entries, because `make dist` copies `examples` with `cp -a`, which copies the link, and then generates a file through it into the source tree; and an rsync mirror of the sources, which worked but kept a second copy of them.

Not needed: `stm32flash` (serial flashing). Firmware reaches the Goteks as a .upd file on the USB stick.

### D9. Branches, commits and rebasing

- Branch 1, for the first pull request, off upstream master: the Apple2 mode as an auto-detected mode. Commit 1 is a pure refactoring that introduces the predicate, still constant in every target. Commit 2 adds detection in the shugart target, with the banner indication and the track number, folded in on 2026-10-08 at the owner's word since the feature is incomplete without them and small enough to review as one; the documentation is ffemu's README, the wiki being upstream's. Done 2026-10-08 as `apple2-auto`, since 2026-10-09 the lower part of the one branch `apple2-mode` with the tag `apple2-as-mode` on its last commit, stacked on `ffemu` (then named `ffemu-fdd`, its stack being three branches at the time) rather than off master, since its ffemu commits, the FDD type dialog and the universal wiring, need both; the firmware commits touch `gotek/floppy.c` in hunks apart from those of the C fallbacks commit, so they can be rebased onto master for the pull request. The predicate is `apple2_mode` in `inc/floppy.h`: `TRUE` in the apple2 target and its bootloader, a variable in the shugart target, `FALSE` elsewhere; `exti_irq_wanted()` in `gotek/floppy.c` keeps the WDATA-capture and STEP interrupts each to its mode (the trap of D2). Commit 1 leaves the bootloader, quickdisk, bl_update and io_test images byte-identical; the shugart ones grow by 40 bytes and the apple2 ones by 300, the latter for the STEP handler that the vector table keeps linked. Documentation went into ffemu's README only; the wiki is upstream's.
- Branch 2, for the second pull request, on top of branch 1: the 307 features, one commit each. Done 2026-10-08 as `apple2-307`, since 2026-10-09 the upper part of `apple2-mode`: the image track on the displays, `.nib` and `.nic` images read and written, the phase indicator in ffemu's notation, the 307 wiring as the second pin set. The owner's decisions of that day: `.nic` too, since the Agat community uses the SDISK// format; the phase indicator with digits and dots as in ffemu, not 307's letters and segments; writes for both nibble formats, which 307 only pretended. The write paths decode the sampled bit cells into nibbles as the Disk II controller does, since neither format can hold a raw bit stream: `.nib` has no sync bits, `.nic` has fixed places for its fields. Not taken from 307, as planned: EXTI stepping, the SELECT polling hack, `board_update()`, `board_ja_strapped()`.
- Branch 3, fork-only, on top of branch 2: display improvements, the Agat-7 font and whatever else is not for upstream.
- Branch `private`: the `private/` directory with this document and the tools script, the build scripts `build.sh` at the top of the tree and `ffemu/build.sh`, and the notes for Claude Code sessions. It never goes upstream.
- Branch `mine`: upstream master with a merge of every other branch, `private` last. It is what is built and tested; nothing is committed on it directly. `CLAUDE.md` on `private` describes this model and the state of every branch.

Besides these, as of 2026-10-08: `fixes`, pull request #1073, with fixes found along the way, `O=` (D8) and the portability of the hardware-independent code to a 64-bit host, was merged upstream on 2026-10-07, so upstream master, now `822b6e3`, is the base of everything and the branch is gone; `ffemu`, pull request #1074, a draft while it is reworked, which since 2026-10-09 holds all of ffemu (before that `ffemu-fdd` and `ffemu-usb` on top of it held what came after the initial commit), among it the firmware commit of D10, and the ffemu release 1.2 at its tip. `mine` merges them all, and `CLAUDE.md` keeps their state.

Commit titles and code on every branch follow upstream's style: `apple2: Build only the HFE image handler`.

To keep rebasing cheap and the diff reviewable: conditions are replaced in place and upstream code is not moved, renamed or reformatted; upstream's coding style is followed (4 spaces, K&R braces, `/* */` comments); new code goes into few contiguous hunks, and fork-only features into files of their own where possible.

Flash is the scarce resource on the F105/AT415: 128 - 32 (bootloader) - 2 (configuration page) = 94 KB for the firmware. The headroom printed by `scripts/check_hex.py` is recorded for every commit. Baseline at 483077a, in bytes:

| Target | F105/AT415 prod | F105/AT415 logfile | AT32F435 prod | AT32F435 logfile |
|---|---|---|---|---|
| shugart | 9800 | 3808 | 126540 | 120628 |
| apple2 | 32020 | 27236 | 148620 | 143864 |

The logfile build of the shugart target on the F105/AT415 is the tightest: 3808 bytes must hold the Apple2 mode and, later, whatever of 307 is to be debuggable on the 415.

After branch 1 (`apple2-auto`, now the tag `apple2-as-mode`, 2026-10-08), in bytes:

| Target | F105/AT415 prod | F105/AT415 logfile | AT32F435 prod | AT32F435 logfile |
|---|---|---|---|---|
| shugart | 8908 | 2860 | 125732 | 119720 |
| apple2 | 31768 | 26988 | 148372 | 143616 |

The Apple2 mode cost the shugart target some 890 bytes in prod and 950 in logfile on the F105/AT415; the apple2 target gained 250 bytes.

After branch 2 (`apple2-307`, now the tip of `apple2-mode`, 2026-10-08), in bytes:

| Target | F105/AT415 prod | F105/AT415 logfile | AT32F435 prod | AT32F435 logfile |
|---|---|---|---|---|
| shugart | 6996 | 660 | 123792 | 117548 |
| apple2 | 29396 | 24408 | 146024 | 141080 |

Branch 2 cost the shugart target some 1900 bytes in prod and 2200 in logfile on the F105/AT415, half of it the two image handlers and their write paths. The logfile build of the shugart target is nearly full there; the apple2 target took `mfm.o` for the raw-bitcell flux code.

### D10. ffemu as a test bench

ffemu, the emulator of the firmware's user interface on a PC (`ffemu/`, pull request #1074, and the fork's releases), runs the firmware's own floppy interface code since 2026-10-08: `src/floppy.c` with `floppy_generic.c` and `gotek/floppy.c`, against models of the pins, of their EXTI interrupts and of the timer that captures STEP. Its keyboard drives the host computer's signals, among them the four Apple2 stepper phases on the release's pins, active high, which it takes through the states that DOS 3.3 RWTS goes through; its Apple2 flavor, `ffemu-apple2`, builds the apple2 target. So the detection of D3 can be tried on phase patterns before the Agat, the indication of D7 seen on the emulated display, and the phase indicator of step 3 developed there. It does not replace the hardware: no data flows, the timing is the host's, and the window and threshold of D3 must still come from the Agat's logged counts.

To let ffemu build it, `gotek/floppy.c` has C fallbacks for its ARM-only code under `#if defined(__arm__)`, in the commit "gotek/floppy: Provide C fallbacks for ARM-only code" on `ffemu`; every firmware image stays byte-identical. Branch 1 changes that file too, which is one reason it was stacked on `ffemu` (D9). Since branch 1, ffemu has an FDD type dialog on key 8, `Step/Dir` or `Apple2`, which decides which pins the keyboard drives and power-cycles the device; the Status window labels the drive with the mode the firmware detected, so that the detection is seen against the type chosen.

## 3. Steps and acceptance

1. Build the baseline. Set up D8, build every target for both MCUs at 483077a, record the headroom figures. Flash the self-built `shugart` firmware on the 435 and test it on a Shugart host (ZX Evolution); flash the self-built `apple2-mode` firmware and boot KLAD140.hfe on the Agat. This proves the toolchain before any source change. Done 2026-10-06 up to the tests on hardware: every target builds without warnings, and the tag v3.44 built with these tools matches the official 3.44 release byte for byte in every .upd, .hex and .dfu, except for the build date and time string and the checksum that follows from it.
2. Branch 1. After commit 1 the `apple2-mode` and `shugart` firmwares behave as in step 1. After commit 2 the one `shugart` firmware boots the Agat (read, and the BASIC write test on a copy of an image) and the Pentagon without reflashing, on the combined wiring; the logged sample counts show the margin. The `apple2-mode` firmware still works. Coded 2026-10-08; in ffemu the shugart firmware detects the Apple2 type (all four phases low, 200 of 200 samples), steps on the phases and mounts HFE only, and detects the Step/Dir type (PA9 and PA10 high) and steps on STEP. On the Agat with the 435, the same day: the detection logged 200/200/200/200 on every boot (open question 1); the FF.CFG on that stick still says `interface = sa390`, a 307 key this firmware does not know, which falls back to `jc` and is harmless, since the JC strap is not read in Apple2 mode. The Pentagon or ZX Evolution test, and the BASIC write test, are still to do.
3. Branch 2. From 307: the .nib handler (read path), the phase indicator on LCD and 7-segment, the real track number (`drive.cyl / 2`) and the 307 wiring. Not taken: its EXTI stepping without debounce and its write path. Coded 2026-10-08 with `.nic` and write paths of its own (D9); in ffemu every format mounts and steps, and the phases show as `*.12. T:00.0` on the emulated OLED. To test on the Agat: boot from a `.nib` and a `.nic`; the BASIC write test on a copy of each, then the files read back on the PC (`nic2dsk` must still accept the `.nic`); the 7-segment indicator needs a device with that display.
4. Branch 3. To be specified when reached.

The 415 joins the tests after its U1 is replaced. Until then the QFN32-specific paths (Select on PA10, JC strap on PA9, the at2-bootloader) are covered only by reading the code.

## 4. Open questions

1. Window length and threshold of D3: confirm from logged counts on the Agat, including a cold power-on and a reset during a seek. Measured 2026-10-08 on the Agat-7 with the 435 on the combined wiring, logfile build `dd41f3f`: five boots, four cold power-ons of Agat and Gotek together and one power cycle of the Gotek alone (its +5 V pulled for a moment) during the DOS boot, all logged `Apple2 mode: on (phase inputs low 200/200/200/200 of 200)`; the boot ROM's recalibration never overlapped the window. A sixth line, `off (0/0/0/0)`, is the first boot after flashing, with the floppy cable not driven. So the margin is the full window, the threshold is four times below the worst count seen, and both stay as they are; a reset in the middle of a seek was not caught and remains covered by the reasoning of D3 only.
2. Settled 2026-10-08 in D7: " A2" on the banner, "A2" or "A-2" on the 7-segment display, "A2L" in logfile builds; whether the three-digit form keeps upstream's MCU character is the owner's call.
6. Debug builds put phases 0 and 1 on the KC30 rotary pins, since PA9 and PA10 are the console, and the detection then disables the encoder. The owner's wish of 2026-10-08: detect the UART instead; what exactly to detect, and where, is to be clarified.
3. Settled 2026-10-06: the work tree normally has `mine` checked out, which has the build scripts as well as every branch (D9).
4. Settled 2026-10-08: `.nib` and `.nic` have write paths of their own in `apple2-mode`, the sampled bit cells decoded into nibbles; untested on hardware. A read-only variant, `AM_RDO` set in the handler's open and no `write_track`, was the intermediate state while the write paths were written, and is how a handler without a write path should be mounted; it was squashed away at the owner's wish, so the branch has one commit for both formats.
5. Nobody here has a real Apple2, IIc or IIgs. A pull request needs testers for those, in particular for the power-on state of the phases and for HDSEL on pin 32.

## 5. References

- `C:\Retro\comps\agat\FloppyEmu 140\gotek-agat-summary.md`: bench notes, wiring, the 415 diagnosis.
- `C:\Retro\comps\agat\agat-hardware\gotek-sa390\`: the 307 sources (git-svn clone).
- Upstream branch `apple2-po` (b65de11, 2025-03-07, "wip"): the start of .po/.do/.dsk handlers, still a copy of `adf.c`; shows upstream's direction, nothing to reuse.
- FlashFloppy wiki: Apple-II, Hardware-Mods, FF.CFG-Configuration-File
