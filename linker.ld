OUTPUT_FORMAT(elf64-x86-64)
ENTRY (main)

SECTIONS {
    _InfernoStart = .;
    .text : ALIGN(0x1000) { *(.text) }
    .data : ALIGN(0x1000) { *(.data) }
    .rodata : ALIGN(0x1000) { *(.rodata) }
    .bss : ALIGN(0x1000) {
        *(COMMON)
        *(.bss)
    }
    /* Add section for identity-mapped code */
    .identtext ALIGN(4K) : {
        *(.identtext)
    }
	_InfernoEnd = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}