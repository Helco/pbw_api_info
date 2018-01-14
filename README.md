# pbw_api_info

This is tool to find out which API functions a pebble app uses. It was created to help the development of [RebbleOS](https://github.com/pebble-dev).

## Usage

```
usage: pbw_api_info [options] inputfile [outputfile]

options:
 -h --help            -> Shows this help screen and exits the program
 --sdkroot            -> Sets the path of the *core* sdk
 --libpath-<platform> -> Sets the path of a single platform import library
   <platform> may be: aplite, basalt, diorite, chalk, emery
 -v --verbose         -> Prints detailed progress information to stderr
```

Currently *inputfile* has to be a .pbw file, all binaries within are scanned. If no output file is given, the result is printed to stdout. The result is always JSON formatted, here a small example:

```
{
  "platforms": {
    "basalt": {
      "usedAPIs": [
        "app_event_loop"
      ]
    },
    "aplite": {
      "usedAPIs": [
        "app_event_loop"
      ]
    }
  }
}
```

## Building

On windows you should probably use the Visual Studio Solution (Migration to use CMake on Windows as well comes soon(tm)).

On *nix you have to install cmake, then run these commands in the root directory:

```
cmake .
make
```

If everything was successful, you should find the binary in the *bin* directory. Please keep in mind that I could not test this on MacOS.

## How it all works

The pebble calls APIs by using a symbol table. Every pebble binary has a function which resolves a function address off this symbol table and calls it. And there are small functions for every import function that the app calls in the binary. These look more or less like this:

```
app_event_loop    PUSH  {R0-R3}
                  LDR   R1, dword_194
                  B.W   jump_to_pbl_function

dword_194         DCD   0x7C
```

These can be found in the app binary as well as in the import library, with the exception that the call to `jump_to_pbl_function` has to be relocated.

This tool first searches through an import library for these functions and determines where relocation might change the code. It then scans through the pebble app code and searches for this pattern. If it exists the app uses said function. Because the functions in the import library are conveniently placed (each as its own section with a name like `.text.app_event_loop`) this scanning can be done without disassembling either the pebble app or the import library. 

## License

This project is licensed under the terms of the [GNU General Public License v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0). This does not apply to the projects found in the *thirdparty* directory. These projects have their own COPYING or LICENSE file in their respective directories.

Furthermore elfio is modified by me in order to not use std::istream and instead use a small interface for easier reading (see elfio_loader.hpp)
