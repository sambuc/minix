# This file is automatically generated.  DO NOT EDIT!
# Generated from: 	NetBSD: mknative-gdb,v 1.5 2011/11/06 19:46:12 christos Exp 
# Generated from: NetBSD: mknative.common,v 1.9 2007/02/05 18:26:01 apb Exp 
#
G_INTERNAL_CFLAGS=    -I. -I${GNUHOSTDIST}/gdb -I${GNUHOSTDIST}/gdb/common -I${GNUHOSTDIST}/gdb/config  -DLOCALEDIR="\"/usr/share/locale\"" -DHAVE_CONFIG_H -I${GNUHOSTDIST}/gdb/../include/opcode -I${GNUHOSTDIST}/gdb/../opcodes/.. -I${GNUHOSTDIST}/gdb/../readline/..  -I../bfd -I${GNUHOSTDIST}/gdb/../bfd -I${GNUHOSTDIST}/gdb/../include -I../libdecnumber -I${GNUHOSTDIST}/gdb/../libdecnumber  -I./../intl -I${GNUHOSTDIST}/gdb/gnulib -Ignulib  -DMI_OUT=1 -DTUI=1  -Wall -Wdeclaration-after-statement -Wpointer-arith -Wformat-nonliteral -Wno-pointer-sign -Wno-unused -Wunused-value -Wunused-function -Wno-switch -Wno-char-subscripts 
G_LIBGDB_OBS=mips-tdep.o mipsnbsd-tdep.o corelow.o solib.o solib-svr4.o nbsd-tdep.o ser-base.o ser-unix.o ser-pipe.o ser-tcp.o fork-child.o inf-ptrace.o nbsd-nat.o mipsnbsd-nat.o  nbsd-thread.o  remote.o dcache.o tracepoint.o ax-general.o ax-gdb.o remote-fileio.o  cli-dump.o  cli-decode.o cli-script.o cli-cmds.o cli-setshow.o  cli-logging.o  cli-interp.o cli-utils.o mi-out.o mi-console.o  mi-cmds.o mi-cmd-env.o mi-cmd-var.o mi-cmd-break.o mi-cmd-stack.o  mi-cmd-file.o mi-cmd-disas.o mi-symbol-cmds.o mi-cmd-target.o  mi-interp.o  mi-main.o mi-parse.o mi-getopt.o tui-command.o  tui-data.o  tui-disasm.o  tui-file.o tui.o  tui-hooks.o  tui-interp.o  tui-io.o  tui-layout.o  tui-out.o  tui-main.o  tui-regs.o  tui-source.o  tui-stack.o  tui-win.o  tui-windata.o  tui-wingeneral.o  tui-winsource.o  tui.o python.o py-value.o py-prettyprint.o py-auto-load.o elfread.o posix-hdep.o c-exp.o  cp-name-parser.o  objc-exp.o  ada-exp.o  jv-exp.o  f-exp.o m2-exp.o p-exp.o  version.o  annotate.o  addrmap.o  auxv.o  bfd-target.o  blockframe.o breakpoint.o findvar.o regcache.o  charset.o disasm.o dummy-frame.o dfp.o  source.o value.o eval.o valops.o valarith.o valprint.o printcmd.o  block.o symtab.o psymtab.o symfile.o symmisc.o linespec.o dictionary.o  infcall.o  infcmd.o infrun.o  expprint.o environ.o stack.o thread.o  exceptions.o  filesystem.o  inf-child.o  interps.o  main.o  macrotab.o macrocmd.o macroexp.o macroscope.o  mi-common.o  event-loop.o event-top.o inf-loop.o completer.o  gdbarch.o arch-utils.o gdbtypes.o osabi.o copying.o  memattr.o mem-break.o target.o parse.o language.o buildsym.o  findcmd.o  std-regs.o  signals.o  exec.o reverse.o  bcache.o objfiles.o observer.o minsyms.o maint.o demangle.o  dbxread.o coffread.o coff-pe-read.o  dwarf2read.o mipsread.o stabsread.o corefile.o  dwarf2expr.o dwarf2loc.o dwarf2-frame.o  ada-lang.o c-lang.o d-lang.o f-lang.o objc-lang.o  ada-tasks.o  ui-out.o cli-out.o  varobj.o vec.o wrapper.o  jv-lang.o jv-valprint.o jv-typeprint.o  m2-lang.o opencl-lang.o p-lang.o p-typeprint.o p-valprint.o  sentinel-frame.o  complaints.o typeprint.o  ada-typeprint.o c-typeprint.o f-typeprint.o m2-typeprint.o  ada-valprint.o c-valprint.o cp-valprint.o d-valprint.o f-valprint.o  m2-valprint.o  serial.o mdebugread.o top.o utils.o  ui-file.o  user-regs.o  frame.o frame-unwind.o doublest.o  frame-base.o  inline-frame.o  gnu-v2-abi.o gnu-v3-abi.o cp-abi.o cp-support.o  cp-namespace.o  reggroups.o regset.o  trad-frame.o  tramp-frame.o  solib.o solib-target.o  prologue-value.o memory-map.o memrange.o xml-support.o xml-syscall.o  target-descriptions.o target-memory.o xml-tdesc.o xml-builtin.o  inferior.o osdata.o gdb_usleep.o record.o gcore.o  jit.o progspace.o inflow.o    init.o
G_SIM_OBS=
