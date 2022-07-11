.global blob_page
.global blob_size_page
.section .rodata

blob_page:
    .incbin "html/page.html"

1:
blob_size_page:
    .int 1b - blob_page