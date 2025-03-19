
# SuperJuniorSameDuck: 
This is a work in progress! Mainly driven by development and testing needs.

A fork of SameBoy with various patches for MegaDuck development and emulation.
- Adds support for the MegaDuck Super Junior/Quique Laptop
  - Keyboard
  - RTC
  - Printers (Single and Double Pass)
  - SRAM Cart
  - Run Cart from ROM Slot
- Improved MegaDuck audio accuracy

Added CLI flags:
- `--megaduck_laptop`: Turn on  MegaDuck Super Junior/Quique Laptop emulation and SDL keyboard support (`F12` to toggle keyboard)
  -  If running the System ROM then also use `--force-mbc 0xE0`
  - Important translated key mappings:
    - `CTRL + F1-F10`: Sharp piano keys, (``CTRL + `,1-10,-,=,Backspace``): Primary piano keys
    - `Tab`: Help key
    - `Scroll Lock`: Printscreen key
- `--duck_printer_1pass` or `--duck_printer_2pass`: Turn on MegaDuck printer emulation
  - With keyboard support enabled, use `scroll lock` or `print screen` keys to emulate print screen key.
- `--workboy`: Turn on Workboy emulation and SDL keyboard support (`F12` to toggle keyboard)
- `--force-mbc <hex mbc number>`: Explicitly specify which MBC to use, do not use header or other detection. Allows using MegaDuck ROMs built for Game Boy MBCs (such as MBC5) as well as avoiding heuristics for other ROMs. 
  - `0xE0`:  MegaDuck Laptop System ROM MBC
    - 32k ROM bank size, reg addr `0x1000`, range 0-15
    - 4 x 8k plug-in cart SRAM banks, shares mbc bank switch register with ROM banks (uses Upper Nibble)
  - `0xE1`:  MegaDuck MD 1 (32K banks, rom bank switch reg addr `0xB000`, range 0-1)
  - `0xE2`:  MegaDuck MD 2 (16k banks, rom bank switch reg addr `0x0001`, range 1-3 or 1-7)-  


# Screenshots
![Spanish MegaDuck Laptop Super QuiQue System ROM running](/info/megaduck_systemrom_spanish.png)
![Bilder Lexikon ROM Cart running](/info/megaduck_bilderlexikon.png)
![Bilder Databank ROM Cart running](/info/megaduck_databank.png)
![Duck Duck Wordyl printout](/info/megaduck_wordyl_printout.png)
![DataBank Printout](/info/megaduck_databank_printout.png)


# Original Repo Readme Below
----------------
# SameBoy

SameBoy is an open source Game Boy (DMG) and Game Boy Color (CGB) emulator, written in portable C. It has a native Cocoa frontend for macOS, an SDL frontend for other operating systems, and a libretro core. It also includes a text-based debugger with an expression evaluator. Visit [the website](https://sameboy.github.io/).

## Features
Features common to both Cocoa and SDL versions:
 * Supports Game Boy (DMG) and Game Boy Color (CGB) emulation
 * Lets you choose the model you want to emulate regardless of ROM
 * High quality 96KHz audio
 * Battery save support
 * Save states
 * Includes open source DMG and CGB boot ROMs:
   * Complete support for (and documentation of) *all* game-specific palettes in the CGB boot ROM, for accurate emulation of Game Boy games on a Game Boy Color
   * Supports manual palette selection with key combinations, with 4 additional new palettes (A + B + direction)
   * Supports palette selection in a CGB game, forcing it to run in 'paletted' DMG mode, if ROM allows doing so.
   * Support for games with a non-Nintendo logo in the header
   * No long animation in the DMG boot
 * Advanced text-based debugger with an expression evaluator, disassembler, conditional breakpoints, conditional watchpoints, backtracing and other features
 * Extremely high accuracy
 * Emulates [PCM_12 and PCM_34 registers](https://github.com/LIJI32/GBVisualizer)
 * T-cycle accurate emulation of LCD timing effects, supporting the Demotronic trick, Prehistorik Man, [GBVideoPlayer](https://github.com/LIJI32/GBVideoPlayer) and other tech demos
 * Real time clock emulation
 * Retina/High DPI display support, allowing a wider range of scaling factors without artifacts
 * Optional frame blending (Requires OpenGL 3.2 or later)
 * Several [scaling algorithms](https://sameboy.github.io/scaling/) (Including exclusive algorithms like OmniScale and Anti-aliased Scale2x; Requires OpenGL 3.2 or later or Metal)

Features currently supported only with the Cocoa version:
 * Native Cocoa interface, with support for all system-wide features, such as drag-and-drop and smart titlebars
 * Game Boy Camera support
 
[Read more](https://sameboy.github.io/features/).

## Compatibility
SameBoy passes all of [blargg's test ROMs](http://gbdev.gg8.se/wiki/articles/Test_ROMs#Blargg.27s_tests), all of [mooneye-gb's](https://github.com/Gekkio/mooneye-gb) tests (Some tests require the original boot ROMs), and all of [Wilbert Pol's tests](https://github.com/wilbertpol/mooneye-gb/tree/master/tests/acceptance). SameBoy should work with most games and demos, please [report](https://github.com/LIJI32/SameBoy/issues/new) any broken ROM. The latest results for SameBoy's automatic tester are available [here](https://sameboy.github.io/automation/).

## Contributing
SameBoy is an open-source project licensed under the Expat license (with an additional exception for the iOS folder), and you're welcome to contribute by creating issues, implementing new features, improving emulation accuracy and fixing existing open issues. You can read the [contribution guidelines](CONTRIBUTING.md) to make sure your contributions are as effective as possible.

## Compilation
SameBoy requires the following tools and libraries to build:
 * clang (Recommended; required for macOS) or GCC
 * make
 * macOS Cocoa frontend: macOS SDK and Xcode (For command line tools and ibtool)
 * SDL frontend: libsdl2
 * [rgbds](https://github.com/gbdev/rgbds/releases/), for boot ROM compilation
 * [cppp](https://github.com/BR903/cppp), for cleaning up headers when compiling SameBoy as a library

On Windows, SameBoy also requires:
 * Visual Studio (For headers, etc.)
 * [GnuWin](http://gnuwin32.sourceforge.net/)
 * Running vcvars64 before running make. Make sure all required tools and libraries are in %PATH% and %lib%, respectively. (see [Build FAQ](https://github.com/LIJI32/SameBoy/blob/master/build-faq.md) for more details on Windows compilation)

To compile, simply run `make`. The targets are:
 * `cocoa` (Default for macOS)
 * `sdl` (Default for everything else)
 * `lib` (Creates libsameboy.o and libsameboy.a for statically linking SameBoy, as well as a headers directory with corresponding headers; currently not supported on Windows due to linker limitations)
 * `ios` (Plain iOS .app bundle), `ios-ipa` (iOS IPA archive for side-loading), `ios-deb` (iOS deb package for jailbroken devices)
 * `libretro`
 * `bootroms`
 * `tester` 

You may also specify `CONF=debug` (default), `CONF=release`, `CONF=native_release` or `CONF=fat_release`  to control optimization, symbols and multi-architectures. `native_release` is faster than `release`, but is optimized to the host's CPU and therefore is not portable. `fat_release` is exclusive to macOS and builds x86-64 and ARM64 fat binaries; this requires using a recent enough `clang` and macOS SDK using `xcode-select`, or setting them explicitly with `CC=` and `SYSROOT=`, respectively. All other configurations will build to your host architecture, except for the iOS targets. You may set `BOOTROMS_DIR=...` to a directory containing precompiled boot ROM files, otherwise the build system will compile and use SameBoy's own boot ROMs.

The SDL port will look for resource files with a path relative to executable and inside the directory specified by the `DATA_DIR` variable. If you are packaging SameBoy, you may wish to override this by setting the `DATA_DIR` variable during compilation to the target path of the directory containing all files (apart from the executable, that's not necessary) from the `build/bin/SDL` directory in the source tree. Make sure the variable ends with a `/` character. On FreeDesktop environments, `DATA_DIR` will default to `/usr/local/share/sameboy/`. `PREFIX` and `DESTDIR` follow their standard usage and default to an empty string an `/usr/local`, respectively

Linux, BSD, and other FreeDesktop users can run `sudo make install` to install SameBoy as both a GUI app and a command line tool.

SameBoy is compiled and tested on macOS, Ubuntu and 64-bit Windows 10.
