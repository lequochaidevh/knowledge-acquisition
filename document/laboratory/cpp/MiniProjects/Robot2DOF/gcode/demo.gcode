
; --- Return home ---
G28            ; Return HOME_X, HOME_Y (always absolute)
G1 X30 Y00 F50  ; Move
G92 X0 Y10
G90
G1 X30 Y0 F30  ; Move
G1 X30 Y20 F30  ; Move
G28
G92 X0 Y0
G4 P0.5
; --- absolute ---
G90             ; Set absolute
G1 X30 Y50 F50  ; Move
G1 X60 Y10 F20  ; Move
G28
G92 X-100 Y0
G91             ; Set non-absolute
G1 X30 Y50 F50  ; Move duplicate
G1 X60 Y10 F20  ; Move
G28            ; Return HOME_X, HOME_Y (always absolute)
G92 X0 Y0
G90             ; Set absolute
G1 X10 Y10 F70
G3 X10 Y10 I10 J10 F70
G91             ; Set absolute
G1 X10 Y10 F70
G2 X0 Y0 I10 J10 F70
; --- End ---
; ======================