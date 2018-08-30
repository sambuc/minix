# Work in progress

This repository contains my currently unfinished work on MINIX.

At this time there is the following:
 * clang-arm: A build of the whole tree for ARM using clang instead of gcc
 * rcp-gen: Adding support for SUN RPC interface generator, API and required OS runtime support
 
Kept around for reference if I start working on this again:
 * gdb: an import of the GNU debuggger, most likely to be dropped in favor of lldb
 * lldb: an import of the LLVM debugger, need to be rebased on top of the current NetBSD work as soon as it is completed
 * lld: Trying to import one of the LLVM linker, with the end goal of completly removing our dependency on Binutils.
 * smp_might_even_compile: This is a couple of fixes I have when I was trying to get SMP back into working shape.
 * gpl-free: Trying to compile minix without any GPL dependencies
