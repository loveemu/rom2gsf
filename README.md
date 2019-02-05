ROM2GSF: GBA ROM to GSF Converter
==================================
[![Travis Build Status](https://travis-ci.org/loveemu/rom2gsf.svg?branch=master)](https://travis-ci.org/loveemu/rom2gsf) [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/wfhyffhwtk5hqkh6/branch/master?svg=true)](https://ci.appveyor.com/project/loveemu/rom2gsf/branch/master)

Program to turn a ROM (or any binary files) into a GSF file. This is used mostly with manual rips.

Downloads
---------

- [Latest release](https://github.com/loveemu/rom2gsf/releases/latest)

Usage
-----

Syntax: `rom2gsf (options) <GBA Files>`

### Options

`--help`
  : Show help

`-m`
  : Multiboot ROM (set entrypoint to 0x2000000)

`-o [output.gsf]`
  : Specify output filename

`--load [offset]`
  : Load offset of GBA executable

`--lib [libname.gsflib]`
  : Specify gsflib library name

`--psfby [name]` (aka. `--gsfby`)
  : Set creator name of GSF

### Example

```bash
rom2gsf AGB-SMPL-USA.gba
```

```bash
rom2gsf -o AGB-SMPL-USA.gsflib AGB-SMPL-USA.gba
```

```bash
rom2gsf -o AGB-SMPL-USA-0001.minigsf --load 0x80cafe8 --lib AGB-SMPL-USA.gsflib --psfby loveemu param.bin
```
