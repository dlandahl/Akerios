
printf "\n == Assembling bootsector == \n"
nasm -f bin bootsector.asm -o bootsector.bin
nasm -f elf kernel_entry.asm -o kernel_entry.o

printf "\n == Compiling kernel == \n"
./i686-elf-tools-linux/bin/i686-elf-gcc -ffreestanding -c kernel.c -o kernel.o -masm=intel -mno-80387

printf "\n == Linking kernel == \n"
ld -o kernel.bin -Ttext 0x1000 kernel_entry.o kernel.o --oformat binary -m elf_i386 -e 0

printf "\n == Concatenating bootsector and kernel == \n"
cat bootsector.bin kernel.bin > AsbestOS

printf "\n == Cleaning up == \n"
rm *.o
rm *.bin

printf "\n == Running == \n"
qemu-system-x86_64 -drive format=raw,file=AsbestOS
