
all: asms

asms:
	make bin/asm-dummy
	make bin/asm-x86
	make bin/asm-x86.old
	make bin/asm-x80
	make bin/asm-x65
	make bin/asm-680x
	make bin/asm-68k
	make bin/asm-i4

clean:
	rm -rf bin obj
	make -C src/dummy clean
	make -C src/x86 clean
	make -C src/x80 clean
	make -C src/x65 clean
	make -C src/680x clean
	make -C src/68k clean
	make -C src/i4 clean

distclean: clean
	rm -rf *~ src/*~
	make -C src/dummy distclean
	make -C src/x86 distclean
	make -C src/x80 distclean
	make -C src/x65 distclean
	make -C src/680x distclean
	make -C src/68k distclean
	make -C src/i4 distclean

.PHONY: all clean distclean asms tests bin/asm-dummy bin/asm-x86 bin/asm-x86.old bin/asm-x80 bin/asm-x65 bin/asm-680x bin/asm-68k bin/asm-i4

bin/asm-dummy:
	make -C src/dummy

bin/asm-x86 bin/asm-x86.old &:
	make -C src/x86

bin/asm-x80:
	make -C src/x80

bin/asm-x65:
	make -C src/x65

bin/asm-680x:
	make -C src/680x

bin/asm-68k:
	make -C src/68k

bin/asm-i4:
	make -C src/i4

