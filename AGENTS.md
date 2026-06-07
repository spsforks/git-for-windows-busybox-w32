# Agent Instructions for busybox-w32 (Git for Windows fork)

This is Git for Windows' fork of `busybox-w32`, the native Win32 port of
BusyBox by Ron Yorston (originally by Nguyễn Thái Ngọc Duy). Upstream lives at
https://github.com/rmyorston/busybox-w32 (typically configured as the
`upstream` remote); our fork is https://github.com/git-for-windows/busybox-w32
(the `origin` remote). The default branch on our side is `main`; upstream's is
`master`.

The fork carries roughly 60 downstream commits on top of `master`, organized as
a "thicket" of topic branches that are merged together via the merging-rebase
workflow inherited from `git-for-windows/git` (see `git-for-windows/git`'s
`AGENTS.md`, section "Merging-Rebases").  A "Start the merging-rebase to ..."
marker commit separates upstream history from downstream patches. Topic
branches with names like `cygpath`, `pretend-applets-in-/bin`, `build-fixes`,
and `clangarm64-and-i686-build-fixes` are merged into `main` rather than
flattened, so each logical feature stays self-contained.

## Why BusyBox in Git for Windows

The original motivation, from 2018
(https://github.com/git-for-windows/git/issues/1439), was to replace MSYS2's
`bash` and friends in MinGit. MSYS2 is a Cygwin derivative, so every child
process goes through Cygwin's `fork()` emulation, which pins the runtime's
address range and is famously slow and fragile; unrelated software upgrades on
a user's machine routinely break it.  BusyBox-w32 is a single Win32 binary that
bundles `ash`, `sed`, `awk`, and the rest of the POSIX userland with zero MSYS2
runtime dependency, which made it an attractive way to ship a fast, robust
MinGit without the fork-emulation footgun.

The plan to make BusyBox-based MinGit the default did not work out.  The
upstream `busybox-w32` project is a single-maintainer effort with no CI, and
tags are sometimes cut without running the project's own test suite, which has
caused real regressions to ship in releases we consumed; many of the patches in
our thicket exist precisely because upstream PRs stalled or were declined.

The current scope is therefore narrow: **BusyBox is only used in the
MinGit-BusyBox variant**, not in the full Git for Windows installer, portable
Git, or Git SDK. The full Git for Windows continues to use MSYS2 `bash` and
friends. See https://github.com/git-for-windows/git/issues/5395 for the broader
ongoing effort to slim down what Git for Windows ships of its own.

## How this repository fits into Git for Windows

The Git-for-Windows-side integration consists of three pieces:

1. **This repository.** The C sources for `busybox.exe`, plus the
   `mingw64_defconfig` / `mingw32_defconfig` / `mingw64a_defconfig` targets and
   the win32 compatibility layer under `win32/`.

2. **The `mingw-w64-busybox` package** in `git-for-windows/MINGW-packages`
   (typically one directory up from this checkout's parent). The `PKGBUILD`
   pulls sources from `git+https://github.com/git-for-windows/busybox-w32` and
   ships the built `busybox.exe` plus hard-linked applets under
   `$MINGW_PREFIX/libexec/busybox/`. There are no `.patch` files in the
   package: all downstream changes live in this Git repository.

3. **The `busybox-w32` topic branch** in `git-for-windows/git`'s merging-rebase
   thicket. As of the v2.54 cycle it contains two small commits that teach Git
   itself how to find a BusyBox applet when a regular `PATH` lookup fails:
   - `mingw: explicitly specify with which cmd to prefix the cmdline`
   - `mingw: when path_lookup() failed, try BusyBox`

These three components are independent: a change to `busybox.exe` behavior only
ships to users after this repo is tagged or its tip moves, the
`mingw-w64-busybox` package is rebuilt via the `MINGW-packages-curl` PKGBUILD,
and that package is then bundled into a fresh MinGit-BusyBox installer by
`build-extra`.

## Known-broken areas (as of June 2026)

Open MinGit-BusyBox bugs worth knowing about before touching related
code paths:

- `git push` panics with `-c: applet not found` and `can't create
  /dev/tty: nonexistent directory`
  (https://github.com/git-for-windows/git/issues/6107).
- `!` aliases do not work in MinGit-BusyBox builds
  (https://github.com/git-for-windows/git/issues/5184).
- `git clone` from a local folder is broken
  (https://github.com/git-for-windows/git/issues/5469).
- `busybox.exe --list` is preferable to `--help` for discovery
  (https://github.com/git-for-windows/git/issues/4440).
- `git` over a proxied SSH server fails
  (https://github.com/git-for-windows/git/issues/3285).

When investigating one of these, reproduce against a fresh MinGit-BusyBox
download rather than a hand-built `busybox.exe`; the shipped artifact may lag
this branch's `HEAD` by one or more package rebuilds.

## Repository layout

The cwd here (`src/busybox-w32` or `src/playground/`) is the live working clone
of `git-for-windows/busybox-w32`. Inside the `mingw-w64-busybox` package
directory you will find:

- `src/busybox-w32/` is what `makepkg-mingw` actually extracts and
  compiles. **Do not edit it by hand**; it gets reset every time
  `makepkg-mingw` runs the `prepare()` step.
- `src/playground/` (does not always exist, is typically a secondary worktree
  of `src/busybox-w32`) is where the actual development and testing happens.
  New patches start here.

Within this checkout, the Windows-specific code lives in `win32/` (threading
shims, console handling, path conversion, process spawn, etc.). The
MinGW-targeted configurations are `configs/mingw32_defconfig`,
`configs/mingw64_defconfig`, and `configs/mingw64a_defconfig`, picked by the
PKGBUILD's `prepare()` based on `$CARCH`.

## Building

### From the package (full installable result)

From PowerShell in the Git for Windows SDK:

    $env:MSYSTEM = "MINGW64"
    $env:PATH = "<MSYS-root>\mingw64\bin;<MSYS-root>\usr\bin;$env:PATH"
    $env:HOME = $env:USERPROFILE
    bash -c "cd /usr/src/MINGW-packages-curl/mingw-w64-busybox && \
             makepkg-mingw -s --nocheck 2>&1"

Note: the `"MINGW64"` part will change to `"UCRT64"` some time in 2026, see
https://www.msys2.org/news/#2026-03-15-deprecating-the-mingw64-environment for
details.

Do not use `bash -l` or `bash --login`; login shells sometimes seem to hang
when run via AI agents' PowerShell.  **Never pass `-C` or `--cleanbuild` to
`makepkg-mingw`.** Both flags wipe `$srcdir`, which would destroy the
playground (`src/playground/`); the loss is unrecoverable. If a genuinely clean
build is needed, move the playgrounds aside first and ask the user.

For other shared package conventions (the `updpkgsums` / `core.autoCRLF=false`
dance, the `--skippgpcheck` flag, running the test suite from an already-built
`src/build-MINGW64/` tree without rebuilding, the `$^O = 'cygwin'` Perl
subtlety) see `MINGW-packages`' `AGENTS.md`. Those notes apply here too.

### Standalone (just `busybox.exe`)

For tight iterate-and-test loops you usually do not need a full
package build. From this directory in a Git for Windows SDK bash:

    make mingw64_defconfig
    make -j$(nproc) busybox.exe

To switch architectures, replace `mingw64_defconfig` with `mingw32_defconfig`
(i686) or `mingw64a_defconfig` (aarch64).  This is what the GitHub Actions
workflow at `.github/workflows/build.yml` runs on every push.

The clang-based toolchains warn (and `-Werror` then fails) about compile-only
flags that the clang driver forwards to the link step.  The PKGBUILD adds
`-Wno-unused-command-line-argument` for clang to work around this; the
standalone build is unaffected because it uses gcc by default.

## Testing

`testsuite/` contains BusyBox's own test suite. Many tests cover POSIX behavior
that does not (and never will) work the same way on Windows: case-insensitive
filesystems, `mkfifo`, exact `ls -l` columns, octal `printf` escapes, `od`
formatting edge cases.  Existing patches in this fork mark or skip those
individually rather than excluding whole files; follow the same pattern when a
new test trips on a Windows-specific quirk.

To run a single test from this directory:

    cd testsuite && ./runtest <name>

To run everything (slow):

    ./runtest

CI (`.github/workflows/build.yml`) currently builds the artifact but does not
run the test suite in every workflow file; some downstream patches add a
separate test-suite job. Check the workflow file before assuming a test is
gating.

## Patching workflow

Because the `mingw-w64-busybox` PKGBUILD consumes this repository directly via
`git+`, the playground-with-tarballs ritual described in
`MINGW-packages-curl/AGENTS.md` does not apply here. The flow is much simpler:

1. Branch off `main` for the topic.
2. Commit changes here. Use `fixup!` / `amend!` and `git rebase -i
   --autosquash` to keep the series clean rather than stacking new commits onto
   an existing patch.
3. Push the topic branch to `git-for-windows/busybox-w32` and open a PR against
   `main`.
4. Once merged, bump the `pkgver` in
   `MINGW-packages-curl/mingw-w64-busybox/PKGBUILD` (it is computed from `git
   rev-list --count HEAD` plus the short OID, so any new commit produces a new
   version), refresh `sha256sums`, and rebuild.

To see what this fork carries on top of upstream:

    git log --no-merges upstream/master..main

To bring in a new upstream release, follow the merging-rebase workflow from
`git-for-windows/git/AGENTS.md` ("Starting a Merging-Rebase"). The
`merging-rebase-to-FRP-<n>-g<oid>` branch naming convention is the same one
used in `git-for-windows/git`.

## What is in scope vs. out of scope

In scope:

- Windows portability fixes in `win32/` and applets.
- Defconfig and build adjustments under `configs/` and `Makefile*`.
- Test-suite changes that mark POSIX-only behavior as skipped on Windows.
- New applets when Git or MinGit needs them (e.g. the existing `cygpath` applet
  was added downstream).

Out of scope without explicit user input:

- Sending patches upstream to `rmyorston/busybox-w32`. The upstream project's
  review cadence and CI gaps mean this often produces more friction than
  progress. If the nature of the contribution suggests low friction, it is okay
  to open PRs there, of course. Just don't stress about it if Git for Windows
  has patches on top that the upstream maintainer is likely to ignore or
  reject.
- Touching the full Git for Windows installer or SDK to use BusyBox.  That goal
  exists (issue 1439) but has been deprioritized; the current production target
  is MinGit-BusyBox only.
- Bumping `busybox` consumption in any other Git-for-Windows-org repository
  without first verifying that the corresponding package and MinGit-BusyBox
  artifact have been refreshed.
