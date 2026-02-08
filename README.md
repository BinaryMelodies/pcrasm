
# A portable cross-platform assembler

This is an assembler that intends to be easy to extend to new targets and new object file formats.

**The current status of the assembler is unfinished and it is in the process of a redesign. Nonetheless it was deemed feature rich enough to consider releasing it publicly.** It supports the following targets:

* `680x`: The Motorola 6800 and 6809 (based on [6809tools](https://github.com/BinaryMelodies/6809tools), the assembler relicensed under the [MIT](LICENSE) license)

* `dummy`: A backend with no instruction set, mostly intended for testing

* `i4`: The Intel 4004 and 4040

* `x65`: The 6502, 65C02, 65CE02 and 65C816

* `x80`: The Intel 8080/Zilog Z80 family, also including Datapoint 2200 (versions 1 and 2), Intel 8008, Intel 8085, ASCII R800 and Sharp SM83 (Gameboy CPU) (the Zilog Z180, Z280, Z380, eZ80 are not currently supported)

* `x86`: The Intel x86 family, including early NEC CPUs. Intel 8080/Zilog Z80 and Intel 8089 instructions can be intermixed with x86 instructions

It supports the following output formats:

* `bin`: Flat binary, only the raw instruction stream (and data declarations) appears inside

* `hex16`, `hex32`: [Intel HEX](https://en.wikipedia.org/wiki/Intel_HEX) for Intel 8080, 8086 and 386

* `omf80`, `omf86`: The [Intel Object Module Format](https://en.wikipedia.org/wiki/Object_Module_Format_(Intel)) for the Intel 8080 and Intel 8086, the latter with the 32-bit extensions that were included in most DOS assemblers and linkers

* `rel`: [Microsoft REL](https://www.seasip.info/Cpm/rel.html) for Intel 8080

* `coff`, `win32`, `win64`: The [UNIX COFF format](https://en.wikipedia.org/wiki/COFF), optionally with the Microsoft modifications

* `elf32`, `elf64`: The [UNIX ELF format](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)

Note that these formats are object formats, and they are not expected to be executable.
A linker such as [RetroLinker](https://github.com/BinaryMelodies/RetroLinker) can be used to create executables from the generated object files.

# Design goals

The main goal of this project was to provide an assembly language that is consistent among different targets, similarly to the [GNU assembler](https://www.gnu.org/software/binutils/).
Unlike the GNU assembler however, this project strives to replicate the syntax for the instructions as documented by the original manufacturers.
Specifically, the x86 instruction set uses Intel mnemonics (though to simplify the implementation, the actual syntax is closer to the [Netwide Assembler](https://nasm.us/).
The assembler incorporates features of multiple assemblers, including a variety of syntactic encodings (for example, for radix integers).
It was also somewhat modelled on the Microsoft M80 and MASM assemblers, in particular the set of supported object formats (although it does not currently support Microsoft-style directives).
It also features a macro facility and stores values as arbitrary precision integers using [The GNU Multiple Precision Arithmetic Library](https://gmplib.org/).

To achieve this, the assembler is divided into a platform independent part and platform specific parts.
The platform independent part handles preprocessing the file, evaluating expressions, maintaining and manipulating the instruction flow and generating the final binary.
This code has almost no references to the targets and usually needs only minor modifications when implementing a new target.
The platform dependent parts are included under subdirectories for each target.
These are typically independent from each other, but it is possible to include parts of other ones (for example, the x86 parser can include Intel 8080 and Intel 8089 instructions in the input source code).

The lexical analyzer and parser generator are similarly divided into two components: a general one and a platform specific one.
To combine the two, the python script `ypp.py` (which stands for "YACC preprocessor") is invoked which recognizes the special `%include` directive.
Then the two files are combined in a way that preserves the layout of the file (the declarations, grammar rules and epilogue get concatenated one by one) to create a combined source file.
This also means that generally speaking, the files `src/parser.y` and `src/target/parser.y` are **not actually complete YACC files** and neat to be preprocessed.

The code generator is optimizing and tries to generate a binary that is as compact as possible.
This is done by rerunning the code generator starting from the most compact encoding, and iterates as long as any of the references need truncation.
The specific targets need to assure that this iteration halts eventually, for example by refusing to choose a more compact form that has been rejected in an earlier iteration.

The assembler supports output formats with different feature sets.
As such, some directives only work for specific targets or work differently for different targets, for example `.org` and `.skip`.
See the file [doc.md](doc.md) for more details on how to use the directives.

# Compilation

The source code relies on a [GCC](https://gcc.gnu.org/)-[compatible](https://clang.llvm.org/) C compiler, the [Flex](https://github.com/westes/flex) lexical analyzer, the [GNU Bison](https://www.gnu.org/software/bison/) parser generator, [Python 3.x](https://www.python.org/), [The GNU Multiple Precision Arithmetic Library](https://gmplib.org/), [GNU Make](https://www.gnu.org/software/make/) and a UNIX/Linux-compatible environment.
Compiling is done by issuing `make` from the root directory of the repository.
The directory `bin` is created with executable binaries, one for each target.

# What needs to be done

* The code base is currently being redesigned. There is probably a decent amount of dead code, especially in the `x86` port.
* There should be tests included in the repository that can be used to check that the assembly files get properly converted into binary format.
* The code generator needs to display cleaner error messages.
* Most targets currently have hard coded patterns. These should be automatically generated from a text database instead.
* Other projects (such as [x80-emulator](https://github.com/BinaryMelodies/x80-emulator)) already provide extensive instruction sets and assemblers that should be integrated.
* Some directives are not yet implemented or could use more options.
* Other targets (in particular the 68k family) and file formats are planned.

