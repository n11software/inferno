OUTPUT_FORMAT(elf64-x86-64)
ENTRY (main)

SECTIONS {
    InfernoStart = .;
    .text : ALIGN(0x1000) { *(.text) }
    .data : ALIGN(0x1000) { *(.data) }
    .rodata : ALIGN(0x1000) { *(.rodata) }
    .bss : ALIGN(0x1000) {
        *(COMMON)
        *(.bss)
    }
    .note.GNU-stack : ALIGN(0x1000) { *(.note   ) }
    /DISCARD/ : { *(.note.GNU-stack) }
	InfernoEnd = .;
}