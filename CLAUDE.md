# flashfloppy (fork)

Notes for a Claude Code session working in this fork of FlashFloppy. The upstream is https://github.com/keirf/flashfloppy, the remote `upstream`; the owner's fork is https://github.com/mike-shevchenko/flashfloppy, the remote `origin`. This file is the owner's, not upstream's: it is tracked on the branch `private` alone, and must not get into a pull request.

## What the fork is for

A universal firmware for Gotek floppy emulators with the AT32F415 (SFRKC30.AT2, QFN32) and the AT32F435 (SFRKC30.AT4.35): the Apple II mode becomes a mode that the firmware detects by itself, so that one firmware serves both an Agat-7 and a Shugart host such as the Pentagon or the ZX Evolution. Then the useful parts of Oleg Odintsov's firmware "307", and last the owner's own improvements (display, Agat-7 font). The plan is `private/ADR-universal-firmware.md`; the bench notes, the wiring and the history of the hardware are in `C:\Retro\comps\agat\FloppyEmu 140\gotek-agat-summary.md`.

## Branches

The model is the one of the owner's fork of bitsnpicas (`C:\github\forks\bitsnpicas\CLAUDE.md`):

- `master` only ever follows the upstream; no commit of the owner's goes there.
- One branch for each pull request. A fix to a feature is committed on the branch of that feature. A branch is stacked on another only when it needs it.
- A branch keeps a clean history: a fix or a small improvement of a feature is folded into the commit that introduced the feature, and a branch is rewritten for it along the way, without asking, even when it is on `origin` already (the owner's word of 2026-10-06; it overrides the global rule of asking before a rebase). Each commit still builds by itself. After such a rewrite, `mine` is built anew from `master` as below.
- `private`: what is the owner's alone and never goes upstream, which is this file, the build scripts `build.sh` and `ffemu/build.sh`, the launcher `ffemu.sh`, and the directory `private/`.
- `mine`: `master` with one merge of each of the other branches, `private` last, so that the first parent of its tip has every change and nothing of `private`. It is what the owner builds and runs, and what the work tree normally has checked out. Nothing is committed on it directly: after a commit on another branch, that branch is merged into `mine` again (`git merge --no-ff --no-edit`).

`rerere.enabled` is set in this clone, so a conflict resolved in one merge is resolved by itself in the next.

When the upstream accepts a pull request: update `master`; rebase each branch that is left onto it (commits accepted as they were drop out; for changed ones name the range with `rebase --onto`); delete the accepted branch; then build `mine` anew, by resetting it to `master` and merging the branches that are left, `private` among them. All of this rewrites history, so it is done only when the owner asks, with the commits named first.

The upstream's own tradition: since 2021 Keir Fraser applies the commits of contributors onto `master` linearly, as their committer; before that he merged pull requests on GitHub. So a branch for a pull request stays linear on `master`, each of its commits builds by itself, and its titles and code follow the upstream: `component: Capitalized title` with no article and no period, four spaces, K&R braces, `/* */` comments, nothing of the upstream reformatted or moved, so that a rebase stays cheap.

## State, 2026-10-07

The first four branches below are on `origin` as well, pushed by the owner.

- `out-of-tree-build`, from `master` at `483077a`: "build: Support out-of-tree builds via O=<path>". `make O=<dir>` puts `out/` and `ext/` under `<dir>` and only reads the source tree; without `O` nothing changes. Meant for a pull request.
- `portable-pointers`, on top of `out-of-tree-build`, though it does not need it and can move onto `master` when it goes upstream: "Do not assume 32-bit pointers in hardware-independent code" and "Provide C fallbacks for ARM assembly in hardware-independent code". Both leave every ARM binary of the firmware byte for byte as it was; they let the hardware-independent code build for a 64-bit host.
- `ffemu`, on top of `portable-pointers`: the emulator of the firmware's user interface in the directory `ffemu/`, one commit for its initial release. Fork-only for now; it may become a pull request later. `ffemu/README.md` describes it.
- `private`, on top of `out-of-tree-build`, since its `build.sh` uses `O=`: the ADR, the build scripts, the launcher and this file, in one commit.
- `fixes`, from `master`: fixes of bugs in the firmware itself, found along the way, one commit each, to go upstream as one pull request. So far "OLED: Fix buffer overrun when FF.CFG changes oled-font". Not on `origin`.
- `mine`: `out-of-tree-build`, `portable-pointers`, `ffemu`, `fixes` and `private` merged onto `master` in that order.

The branches the ADR plans next (D9): the Apple II mode as an auto-detected mode, for the first pull request about it; the features of 307, on top of it; the owner's own improvements, fork-only.

## Building and testing

- The firmware is built on the Ubuntu VM, from the sources on Windows: `ssh ubuntu 'bash /c/github/forks/flashfloppy/build.sh'` makes `dist` into `~/retro/flashfloppy-build/` (`U:\retro\flashfloppy-build\` on Windows), with the short hash of `HEAD` as the version. The tools are in `~/retro/flashfloppy-tools/`, put there by `private/set_up_tools.sh`. The build directory may be deleted for a clean build.
- `ffemu` is built in MSYS2 with `ffemu/build.sh`, into `C:\github\forks\flashfloppy-build\out\ffemu\ffemu.exe`, which also runs in Git Bash; it needs MSYS2's `ncurses-devel`. The owner wants it rebuilt there after every change to it, for his own testing. The same script builds it on the VM, into `~/retro/flashfloppy-build/`: `ssh ubuntu 'bash /c/github/forks/flashfloppy/ffemu/build.sh'`.
- Without a terminal, `ffemu` is tested by commands on its standard input (see its README), or under a tmux server of the test's own in MSYS2 (`tmux -L <name> -f /dev/null`, `capture-pane`, `kill-server` at the end). A leftover `ffemu` is stopped by its PID only: the owner may be running one of his own.
- The test devices: the 435, which works; the 415 has a dead output inverter (U1) and waits for repair.
