! C runtime initialization for Prizm addins
! By Tari and calc84maniac, based on Kristaba's reverse-engineered crt0.
 
    .extern _main
    .global initialize
   
    .section ".pretext"
    .align 2
initialize:
! Preserve things on the stack
    mov.l r14, @-r15    ! Frame pointer
    sts.l pr, @-r15     ! Return address
    mov.l r4, @-r15     ! Parameter 1
   
! Copy .data section into RAM
    mov.l v_datald, r0      ! From
    mov.l v_sdata, r2       ! To
    mov.l v_edata, r3       ! Limit
dataLoop:
    cmp/hs r3, r2
    bt dataDone             ! Stop when r2 >= r3
    mov.l @r0+, r1
    mov.l r1, @r2
    bra dataLoop
    add #4, r2              ! Delay slot
dataDone:

! Copy .ilram section into ILRAM
    mov.l v_ilramld, r0      ! From
    mov.l v_silram, r2       ! To
    mov.l v_eilram, r3       ! Limit
ilramLoop:
    cmp/hs r3, r2
    bt ilramDone             ! Stop when r2 >= r3
    mov.l @r0+, r1
    mov.l r1, @r2
    bra ilramLoop
    add #4, r2              ! Delay slot
ilramDone:

! Copy .ocram section into the first additional on-chip RAM
    mov.l v_ocramld, r0      ! From
    mov.l v_socram, r2       ! To
    mov.l v_eocram, r3       ! Limit
ocramLoop:
    cmp/hs r3, r2
    bt ocramDone             ! Stop when r2 >= r3
    mov.l @r0+, r1
    mov.l r1, @r2
    bra ocramLoop
    add #4, r2              ! Delay slot
ocramDone:

! Copy .ocram2 section into the second additional on-chip RAM
    mov.l v_ocram2ld, r0      ! From
    mov.l v_socram2, r2       ! To
    mov.l v_eocram2, r3       ! Limit
ocram2Loop:
    cmp/hs r3, r2
    bt ocram2Done             ! Stop when r2 >= r3
    mov.l @r0+, r1
    mov.l r1, @r2
    bra ocram2Loop
    add #4, r2              ! Delay slot
ocram2Done:

! Copy .rsmem section into what I believe is an unused area of RS memory
    mov.l v_rsmemld, r0      ! From
    mov.l v_srsmem, r2       ! To
    mov.l v_ersmem, r3       ! Limit
rsmemLoop:
    cmp/hs r3, r2
    bt rsmemDone             ! Stop when r2 >= r3
    mov.l @r0+, r1
    mov.l r1, @r2
    bra rsmemLoop
    add #4, r2              ! Delay slot
rsmemDone:
 
! Zero out .bss
    mov.l v_ram_ebss, r2    ! To
    mov.l v_ram_bbss, r3    ! Limit
    mov #0, r1              ! Constant
bssLoop:
    cmp/hi r3, r2
    bf bssDone              ! Stop when r2 <= r3
    nop
    bra bssLoop
    mov.b r1, @-r2          ! Delay slot
bssDone:
   
! RAM is now initialized
 
    mov r5, r14             ! Save parameter 2
    mov #1, r6
    mov #0, r4
    bsr _GlibAddinAplExecutionCheck
    mov r6, r5
   
! main(r4, r5) with same state as input (returns to our caller)
   
    mov.l main, r7
    extu.w r14, r5
    mov.l @r15+, r5
    lds.l @r15+, pr
    jmp @r7
    mov.l @r15+, r14        ! Delay slot
   
_GlibAddinAplExecutionCheck:
    mov.l v_syscall, r2
    jmp @r2                 ! _GlibAddinAplExecutionCheck
    mov #0x29, r0           ! Delay slot
   
! Constants
    .align 4
main:
    .long _main
v_syscall:
    .long 0x80020070
 
v_datald:
    .long _datald
v_edata:
    .long _edata
v_sdata:
    .long _sdata
    
v_ilramld:
    .long _ilramld
v_eilram:
    .long _eilram
v_silram:
    .long _silram
    
v_ocramld:
    .long _ocramld
v_eocram:
    .long _eocram
v_socram:
    .long _socram
    
v_ocram2ld:
    .long _ocram2ld
v_eocram2:
    .long _eocram2
v_socram2:
    .long _socram2
    
v_rsmemld:
    .long _rsmemld
v_ersmem:
    .long _ersmem
v_srsmem:
    .long _srsmem
   
v_ram_bbss:
    .long _bbss
v_ram_ebss:
    .long _ebss
 