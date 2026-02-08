
# Directives

## .align

## .comm/.common

## .entry

## .equ

## .export

## .extern

## .fill, .endfill

Repeats a pattern until it reaches a mark.
The pattern might be terminated before it is finished.

## .global

## .if, .elif, .else, .endif

Conditional preprocessor directives.
Expressions using labels and constant identifiers are allowed, but the preprocessor must be capable of determining whether the expressions in the `.if`, `.elif` evalutes to zero.
This means that the difference between two labels is not supported.

## .import

## .macro, .endmacro

## .org

Sets the current instruction location.
If supported by the backend, it will also move the instruction pointer to a specific location.
Currently, only the Intel HEX, REL and Intel OMF backends support this, all other output formats ignore it and only change the internal value for the instruction location.
Other backends should use `.fill`, `.times` or `.skip` to fill up the output with bytes.

Note that `.org` should only be used with increasing offsets, backtracking might result in unexpected behavior.

## .rel

## .section

## .skip

Changes the current instruction location and instruction pointers.

For backends that support moving the instruction pointer, it changes the address of the next instruction, and the intervening bytes are undefined.

For the binary backend, it places 0 bytes in the instruction stream.

For other backends, it determines the precise action based on the type of the section.
For sections containing instructions, it inserts bytecode that represents a no-operation.
For sections containing other types of data, it inserts 0 bytes.
For sections that are zero filled, it increases the section size to accomodate the skip.

## .times, .endtimes

Repeats a pattern a specified amount of times.

