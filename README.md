
# Super Junior SameDuck Emulator
A fork of [SameBoy](https://github.com/LIJI32/SameBoy) with various patches for MegaDuck research, development and emulation (priorities in that order).

![Bilder Databank ROM Cart running](/info/megaduck_databank.png)

### Features
The first emulator that supported the MegaDuck Super Junior/Quique Laptop hardware. This support was implemented based on [disassembly of the laptop system ROM](https://github.com/bbbbbr/megaduck-quique-disasm) and other research.
- Keyboard
- RTC
- Printers (Single and Double Pass)
- SRAM Cart
- Run Cart from ROM Slot
 - one of few emulators with (seemingly) all audio registers correctly supported.

#### Update!
- The [MiSTer Game Boy/MegaDuck core](https://github.com/MiSTer-devel/Gameboy_MiSTer/) core now also supports the MegaDuck laptop model and has correct audio emulation.
It is also one of very few emulators with (seemingly) all audio registers correctly supported.

### Accuracy
- Safe keyboard polling intervals: The emulator does not yet currently enforce the required "safe" interval between sequential keyboard polling requests. It always succeeds. In hardware polling too quickly may result in lockup of the peripheral IO controller (which connects the keyboard, rtc, etc).
- Serial clock speeds: The emulator does not yet mimic the (significantly) faster transfer speed when the peripheral IO controller is driving the serial clock (compared to the much slower speed when the MegaDuck sm83 cpu CPU is driving the serial clock).
- RTC reset quirk: The Spanish laptop hardware System ROM monitors WRAM (across power cycles and cart slot program launches) to see whether the sequence `0xAA, 0xE4, 0x55` is preserved starting at WRAM address `0xDBFC`. The emulator does not implement preserving WRAM across power cycles and so the System ROM will always try reset the RTC when it starts up. The RTC reset behavior is not present in the German model System ROM.


### Detecting MBC by Filename Extension
The emulator will attempt to identify which MBC (cart memory controller) to use based on the filename extension.
The MBC type can also be set explicitly using the `--force-mbc` option (overrides file extension).
  - `.md0`: MegaDuck MD 0
    - Games/Programs: Laptop System ROM, Bilder Lexikon, DataBank
      - These are all laptop programs require the laptop hardware (automatically connected by default in the emulator, will not work with `--duck-handheld`)
  - `.md1`: MegaDuck MD 1
    - Games: Puppet Knight, Suleiman’s Treasure
  - `.md2`: MegaDuck MD 2
    - Games: 2nd Space, Ant Soldiers, Armour Force, Beast Fighter, Black Forest Tale, Captain Knick Knack, Commin Five in One, Duck Adventures, Four in One, Magic Tower, Railway, Snake Roy, Worm Visitor, Zipball
  - `.md2s`: MegaDuck MD 2 + Cart SRAM
    - Games: Pokemon Red (rom patch), QR-Paint
  - `.bin` / `.duck`: MegaDuck with NO MBC bank switching controller
    - Games: Arctic Zone, Bomb Disposer, Magic Maze, Pile Wonder, Street Rider, The Brick Wall, Trap and Turn, Vex
  - `.mbc5`: Standard Game Boy MBC5 with SRAM and Battery

### Emulated Keyboard for Laptop Model
Once running, press the "F12" key to turn on the SDL keyboard emulation support.
- To turn off the keyboard emulation and return the keyboard to normal emulator use, press the "F12" key again.
- Important translated key mappings:
  - `CTRL + F1-F10`: Sharp piano keys, (``CTRL + `,1-10,-,=,Backspace``): Primary piano keys
  - `Tab`: Help key
  - `Scroll Lock`: Printscreen key

### Added debugger commands:
- `vram`: Opens a VRAM tile viewer window

### Added command line options:
- `--duck-handheld`: Handheld support only. **Turn OFF MegaDuck Super Junior/Quique Laptop emulation** and SDL keyboard support (`F12` to toggle keyboard)
- `--duck-printer-1pass` or `--duck-printer-2pass`: Turn on MegaDuck printer emulation for the laptop model
  - With keyboard support enabled, use `scroll lock` or `print screen` keys to emulate print screen key.
- `--workboy`: Turn on Workboy emulation and SDL keyboard support (`F12` to toggle keyboard)
- `--duck-sram-cart`: The add-on SRAM cart is plugged into the second cart slot on the laptop model (or a custom cart with on-board SRAM)
- `--force-mbc <hex mbc number>`: Explicitly specify which MBC to use, do not use header or other detection. Allows using MegaDuck ROMs built for Game Boy MBCs (such as MBC5) as well as avoiding heuristics for other ROMs. 
  - `0xE0`:  MegaDuck MD 0 (Laptop Cart and System ROM MBC)
    - 32k ROM bank size, reg addr `0x1000`, range 0-15
    - 4 x 8k plug-in cart SRAM banks, shares mbc bank switch register with ROM banks (uses Upper Nibble)
  - `0xE1`:  MegaDuck MD 1 (32K banks, rom bank switch reg addr `0xB000`, range 0-1)
  - `0xE2`:  MegaDuck MD 2 (16k banks, rom bank switch reg addr `0x0001`, range 1-3 or 1-7)
  - `0x00`:  MegaDuck with NO MBC bank switching controller
  - Others: For MegaDuck games that use MBC controllers, their MBC number may be specified. It is an uncommon scenario.


# Screenshots
![Spanish MegaDuck Laptop Super QuiQue System ROM running](/info/megaduck_systemrom_spanish.png)
![Bilder Lexikon ROM Cart running](/info/megaduck_bilderlexikon.png)
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
