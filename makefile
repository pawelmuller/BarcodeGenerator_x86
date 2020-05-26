CC=gcc
ASMBIN=nasm

all : asm cc link clean
asm : 
	$(ASMBIN) -o barcode.o -f elf -g -l barcode.lst barcode.asm
cc :
	$(CC) -m32 -fpack-struct -c -g -O0 barcode_main.c
link :
	$(CC) -m32 -g -o barcode barcode_main.o barcode.o
clean :
	rm *.o
	rm barcode.lst
debug:
	gdb barcode