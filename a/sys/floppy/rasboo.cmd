!
! BOOTSTRAP ON UP, LEAVING SINGLE USER
!
SET DEF HEX
SET DEF LONG
SET REL:0
HALT
UNJAM
INIT
LOAD BOOT
D R10 3		! DEVICE CHOICE 3=RA
D R11 2		! 2= RB_SINGLE
START 2
