# chipolata

A Chip-8 Emulator with standard op codes mainly to understand how a basic emulator works.

No sound yet because I did not want to deal with SDL3 Audio just yet. Regardless, there are some demo files you can play around with in res/ folder.

## Build

```bash
git clone https://github.com/planeflight/chipolata --recursive
cd chipolata/
```

Run the following to build and run:

```bash
cmake -S . -B build/
cmake --build build/
./build/chipolata <file>
```
