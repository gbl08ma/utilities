OUTPUT_FORMAT(binary)
OUTPUT_ARCH(sh3)
 
/* Entry point.  Not really important here, since doing binary output */
ENTRY(initialize)
 
MEMORY
{
        /* Loads code at 300000, skips g3a header */
        rom (rx) : o = 0x00300000, l = 1024k
        ram (rwx) : o = 0x08100004, l = 524283  /* 0x1FFFF */
        ocram (rwx) : o = 0xE5007000, l = 8192 /* first on-chip-ram-area */
        ocram2 (rwx) : o = 0xE5017000, l = 8192 /* second on-chip-ram-area */
        ilram (rwx) : o = 0xE5200000, l = 4k /* ILRAM */
        rsmem (rwx) : o = 0xFD803200, l = 3576 /* portion of RS memory that I believe is unused by the OS */
}
 
SECTIONS
{
        /* Code, in ROM */
        .text : {
                *(.pretext)     /* init stuff */
                *(.text)
                *(.text.*)
        } > rom
       
        /* Read-only data, in ROM */
        .rodata : {
                *(.rodata)
                *(.rodata.*)
        } > rom
       
        /* RW initialized data, VMA in RAM but LMA in ROM */
        .data : {
                _datald = LOADADDR(.data) ;
                _sdata = . ;
                *(.data)
                *(.data.*);
                _edata = . ;
        } >ram AT>rom
       
        /* Uninitialized data (fill with 0), in RAM */
        .bss : {
                _bbss = . ;
                *(.bss) *(COMMON);
                _ebss = . ;
        } >ram
        
        .ocram : {
                _ocramld = LOADADDR(.ocram) ;
                _socram = . ;
                *(.ocram) *(.ocram.*) *(COMMON); 
                _eocram = . ;
        } >ocram AT>rom
        
        .ocram2 : {
                _ocram2ld = LOADADDR(.ocram2) ;
                _socram2 = . ;
                *(.ocram2) *(.ocram2.*) *(COMMON); 
                _eocram2 = . ;
        } >ocram2 AT>rom
        
        .ilram : {
                _ilramld = LOADADDR(.ilram) ;
                _silram = . ;                
                *(.ilram) *(.ilram.*) *(COMMON); 
                _eilram = . ;
        } >ilram AT>rom
        
        .rsmem : {
                _rsmemld = LOADADDR(.rsmem) ;
                _srsmem = . ;                
                *(.rsmem) *(.rsmem.*) *(COMMON); 
                _ersmem = . ;
        } >rsmem AT>rom
}
