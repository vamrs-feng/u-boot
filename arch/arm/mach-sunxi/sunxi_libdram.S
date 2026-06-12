/* SPDX-License-Identifier: GPL-2.0+ */

.section .rodata.sunxi_libdram, "a"
.balign 64
.global sunxi_libdram_blob_start
sunxi_libdram_blob_start:
.incbin SUNXI_LIBDRAM_BIN
.balign 32
.global sunxi_libdram_blob_end
sunxi_libdram_blob_end:
