Userland Vita Loader 1.1.0
===============================================================================

## What's new?

### Version 1.1.0

* Added Unity PSM cleanup code (thanks Netrix)

### Version 1.0.1

* Added support for multiple loads (stacked homebrew loading)

### Version 1.0.0

* Relocatable ELF (ET_SCE_RELEXEC) support
* ARM relocation resolving
* SceLibKernel NID cache database (for NID poison bypass)
* Library reloading (another NID antidote technique)
* UVL custom exports (code alloc, icache flush, logging via USB)

### Version 0.1.0

* Initial release

## What is this?

In short, this is a loader that allows running homebrew games on the Vita using 
save-file exploits or similar methods where there is no access to the system 
loader functions (which are found in the kernel). UVL does this by hooking on 
to functions and API calls imported by the running game and passing them to 
the homebrew being loader. This is **not** a way to run backups or pirated 
games as that is not only wrong to do, but also because UVL does not and can 
not decrypt content nor can it do dynamic linking or other sophisticated things 
that the system loader does.

## How do I run it?

UVL is designed to work with any userland exploit with little configuration. 
However, you should check with the developer of the exploit who ported UVL 
to see how to use it. UVL cannot do anything by itself, it is simply a 
payload that is executed by an exploit to run homebrews unmodified.

## How can I call UVL API functions from my homebrew?

Make sure `$VITASDK` points to where the toolchain is installed. Then run 
`make install_lib` to install the UVL stub library to the right place. Then you 
can include `psp2/uvl.h` into your project and build with `-lUVLoader_stub`.
Finally to create the resulting VELF, you need to pass the UVL JSON database 
as the parameter to `vita-elf-create` (`$(VITASDK)/share/uvloader.json`).
See the documentation for details on the exported functions and when to use 
them. The library exposes functions for logging and dynamic code generation 
that is otherwise missing from the SDK.

## How do I port UVL?

If you have an exploit for the Vita (not the PSP emulator as UVL does not work 
on that), then you should be able to port the exploit by finding a couple of 
memory addresses for some API calls and passing them to the config file. 
More information will be available when the time comes...

## How do I compile UVL?

First of all, be aware that it is impossible to use UVL without an exploit, 
but once you have that and need a payload, all you need to do is modify 
the Makefile to point to your ARM toolchain and run "make". The toolchain 
that is tested with is <https://launchpad.net/gcc-arm-embedded/+download>.

## Who's responsible for this?

This project is based heavily off of 
[Half Byte Loader](http://valentine-hbl.googlecode.com/) for the PSP. 
Some code is ripped from the 
[Bionic](https://github.com/android/platform_bionic/) libc project.
The project is started by [Yifan Lu](http://yifan.lu/) with thanks to 
the following people for their contribution. (Apologies for those 
forgotten.)

### Thanks To

* Davee for many ideas and help
* Proxima for module reloading NID antidote method
* naehrwert for some code snippets and programming help
* roxfan for finding structures
* Netrix for Unity cleanup code
* anyone in #vitadev who answered my stupid questions
