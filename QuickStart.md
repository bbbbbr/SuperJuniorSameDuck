
# SuperJuniorSameDuck Emulator
A fork of [SameBoy](https://github.com/LIJI32/SameBoy) with various patches for MegaDuck research, development and emulation (priorities in that order).

Source repo: https://github.com/bbbbbr/SuperJuniorSameDuck
By: bbbbbr


### Example command line to start the emulator with:
- The two-pass printer connected: `--duck-printer-2pass`
- The MegaDuck Laptop model: Automatically connected by default
- The Spanish Laptop System ROM (md0 extension sets MBC): megaduck_laptop_systemrom_spanish.md0

Command line:
superjunior_sameduck --duck-printer-2pass megaduck_laptop_systemrom_spanish.md0


Once running, press the "F12" key to turn on the SDL keyboard emulation support.
- To turn off the keyboard emulation and return the keyboard to normal emulator use, press the "F12" key again.
- Important translated key mappings:
  - `CTRL + F1-F10`: Sharp piano keys, (``CTRL + `,1-10,-,=,Backspace``): Primary piano keys
  - `Tab`: Help key
  - `Scroll Lock`: Printscreen key


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
  - `.bin` / `.duck`: MegaDuck with NO MBC bank switching controller
    - Games: Arctic Zone, Bomb Disposer, Magic Maze, Pile Wonder, Street Rider, The Brick Wall, Trap and Turn, Vex
