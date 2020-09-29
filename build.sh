
printf "\n == Assembling bootsector == \n"
nasm -f bin boot/bootsector.asm -o bootsector.bin
nasm -f elf boot/kernel_entry.asm -o 0kernel_entry.o

printf "\n == Compiling kernel == \n"
./i686-elf-tools-linux/bin/i686-elf-gcc -ffreestanding -masm=intel -mgeneral-regs-only \
    -c kernel/*.c drivers/*.c -I include -march=i386

printf "\n == Linking kernel == \n"
ld -o kernel.bin -Ttext 0x1000 --oformat binary -m elf_i386 -e 0 \
    *.o

printf "\n == Concatenating bootsector and kernel == \n"
cat bootsector.bin kernel.bin > AkeriOS

binary_size=$(wc -c < AkeriOS)
padding=$(((1<<16)-binary_size))

dd if=/dev/zero bs=1 status=none count=$padding >> AkeriOS
cat filesystem >> AkeriOS

printf "\n == Cleaning up == \n"
rm *.o
rm *.bin

printf "\n == Running == \n"
qemu-system-x86_64 -drive format=raw,file=AkeriOS -vga std
