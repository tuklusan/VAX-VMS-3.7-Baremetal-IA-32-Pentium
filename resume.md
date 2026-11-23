# Native VAX/VMS 3.7 IA-32 M192 checkpoint

INCOMPLETE. M185-M192 implements owner-scoped private Files-11 channel handles with 32 host cases; QEMU assigns a native private handle, checks 600 real ATA bytes via two IRPs and releases it. Public process $ASSIGN/$QIO, DQA0 DDT/IRQ, ACP, login, DCL and DECnet remain unimplemented. Genuine SYSTEM login,
DCL, DECnet Phase IV and SET HOST 0 are not implemented.

Branch: tuklusan/OpticOS vms37-x86-qemu-m45-ci
Native tree: vms37-native-x86-m89 (all patches M90-M143, M146 (M144-M146 combined), M149 (M147-M149 combined), M152 (M150-M152 combined), M160 (M153-M160 combined), M168 (M161-M168 combined), M176 (M169-M176 combined), M184 (M177-M184 combined), M192 (M185-M192 combined) applied)
Rebuild: make -j2 all && make relocdrvcheck
Guest regression: python3 tools/qemu_sysboot_m192.py
All QEMU boot traces are partial INIT only, never a complete VMS boot.
Original DEC source is not distributed with this archive.

## Independent M192 baseline verification — 22 November 2026

Recovery integrity: the provided archive is 7,993,520 bytes with SHA-256 `cdfb3de26382b210625392947e848e3fc484e3643234af0f640761daeae80ed3`; the ZIP CRC scan passed (2,234 entries). The recovered working files match archive entries except for generated `build/` outputs, local QEMU evidence, and this updated resume/build documentation. The separate `../original-vms37` checkout is exactly `2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752`; `../optic-history` is clean at M192 commit `1f01e66bb99144ba4aa3af5ec40be4906e4b2bf5` and was used only as reference.

Build environment: Ubuntu 22.04.5 WSL; GNU Make, binutils, NASM, QEMU 6.2, Python 3.10, GCC 11.4 and GCC 12.3 installed. GCC 11.4 with `-Werror` rejects `src/vms_exe_schevt.c` with `-Wmaybe-uninitialized` for `when`; a clean build with GCC 12.3 succeeded. This compiler-specific failure is recorded, not hidden.

Verified from a clean build (`make clean`): `make -j2 CC=gcc-12 all` PASS. `make CC=gcc-12 relocdrvcheck f11channelcheck f11ioccheck sysexecentrycheck sourcerangecheck` PASS, including 32 M185-M192 channel regressions, 32 M177-M184 IOC bridge regressions, resident SYSLOA vector/INIT checks, and source-range validation. Full `make CC=gcc-12 test` PASS, including the existing host and layout regressions through M49 and generated source-range audit consistency (1,957 ranges, 0 unmapped functions). Counts remain descriptive, not functional completion.

Real QEMU M192: `python3 tools/qemu_sysboot_m192.py` PASS under QEMU TCG; independent COM1 output is archived as `logs/serial/qemu-m192-run-local-20261123T040040872345Z.log`. Source-order checks passed through the native owner-scoped private channel operation and the explicit incomplete-INIT stop. No login or DCL was observed. The runner was corrected to record M192 evidence under M192 rather than M160. The runner terminates its QEMU child before returning.

Public-repository audit: the recovery archive contains no original `.MAR`, `.B32`, `.BLI`, or `.REQ` sources; matching restricted extensions are absent from `src/`. `.gitignore` excludes source extensions, original/recovery trees, build products, disk images, secrets, and local configs. A text scan found no recognized credential/private-key patterns. `portdisk.img`, `vms37lab.ima`, and generated `build/` products are not to be committed. Source coverage is an index/audit record only; it does not establish functional port completeness.

Initial active-repository baseline commit: f800400c6c23eee00dadac6a5be1a342c240b961. New active repository: `https://github.com/tuklusan/VAX-VMS-3.7-Baremetal-IA-32-Pentium.git` (branch `main`). M192 source is the baseline; development priority remains source-matched DQA0 IOC DDT/CRB/IRQ integration, public `$QIO`, process-aware ACP/AST completion, and native INIT/process activation. All acceptance conditions remain incomplete.



## M193 — Native ATA IRQ14 completion, 22 November 2026

Implemented an actual primary-channel ATA IRQ14 route: IDT vector 46, PIC cascade/IRQ14 unmasking, device interrupt enable, ATA status acknowledgement, and an armed/completed single-sector read callback with a bounded wait. Existing polling bus tests remain supported; the host regression also covers arming before command issue and rejecting unpaired callbacks. The QEMU M193 run confirms physical reads through the IRQ path and remains halted at the known incomplete INIT boundary. This does not publish VMS DQA0 DDT/CRB, implement RB730 controller semantics, public SYS$QIO, ACP, or process-aware AST completion.

Verification: `make -j2 CC=gcc-12 all` PASS; full `make CC=gcc-12 test` PASS (shell reported `MAKE_EXIT:0`; all existing host/layout regressions reached M49); ATA callback and IRQ layout checks PASS; generated source audit has 1,957 ranges and 0 unmapped functions; `python3 tools/validate_source_ranges.py` PASS. One real QEMU runner was active at a time and it terminated before returning. `logs/serial/qemu-m193-run-local-20261123T041034164166Z.log` is the raw 4,600-byte COM1 transcript; SHA-256 `b83ec2a164adef10221c110aba147dc906b7174b69c6ba0886e5d700589c2984`. Serial manifest verification PASS. No login or DCL was observed.

Coverage audit change: DQDRIVER lines 772-840 now map as partial to `src/vms_ata_x86.c:port_ata_x86_irq`; the record limits this to ATA status/sector completion and leaves RB730, DQA0 DDT/CRB, IOC, ACP and process completion unresolved. Totals: 28,697 partial, 50,504 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped; 441 of 4,566 original files indexed. These are inventory counts only.

M193 commit: `7c252354af3518af3c781a59bd1ae1905d8a08d7`. Resume from source-matched DQA0 DDT/CRB publication and connect the interrupt completion to native IOC ownership and completion semantics before enabling public process I/O.

