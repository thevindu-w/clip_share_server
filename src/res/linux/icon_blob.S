.global icon_png
.global icon_png_len
.section .rodata

icon_png:
    .incbin "res/linux/icon.png"
icon_end:

.balign 4
icon_png_len:
    .int icon_end - icon_png
