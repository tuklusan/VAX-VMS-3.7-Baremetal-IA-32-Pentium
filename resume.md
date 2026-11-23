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


## M194 — GCC 11 strict build compatibility, 22 November 2026

Initialized the `when` outputs in the two scheduler timer request paths before calling the status-returning expiration helper. This preserves request behavior while allowing GCC 11.4 to prove the outputs initialized under `-Werror=maybe-uninitialized`.

Verification from a clean tree: `make clean && make -j2 all` PASS with default GCC 11.4; full `make test` PASS (`MAKE_EXIT:0`, suite reached M49); `make relocdrvcheck f11channelcheck f11ioccheck sysexecentrycheck sourcerangecheck` PASS; source-range validation PASS (1,957 ranges, 0 unmapped functions). One QEMU M193 regression under the GCC 11 build PASS, confirming the live IRQ14 read and incomplete-INIT stop; runner terminated. Raw COM1 transcript: `logs/serial/qemu-m193-run-local-20261123T041538684129Z.log`, 4,597 bytes, SHA-256 `9a9ecd9667d9cad8af00099d46edafc9ed23f09087478140b85a399c6a84c593`. Serial manifest verification PASS.

No VMS source coverage status changed. DQA0 DDT/CRB publication, public process `$QIO`, Files-11 ACP/AST integration, INIT, process activation, LOGINOUT/DCL, DECnet and SET HOST 0 remain incomplete.

M194 commit: `d1ee8397ad7cea37c6aa03e6885418d44d44a8a3`.

## M195 — Staged DQ physical read dispatch, 22 November 2026

Added port_dqddt_bootread, which validates the staged source-shaped DDT and its FDT, classifies the original READPBLK function, then enters the existing guarded privileged physical-read path. Host coverage verifies a 512-byte completion through the bound sector callback and rejects a mismatched requested LBN without a second disk access. port_dqddt_function remains unbound for general DDT actions. This is not DQA0 DDT/CRB publication, public SYS$QIO, async IOC, ACP, or process AST completion; resident INIT remains incomplete.

Verification: clean GCC 11.4 make clean && make -j2 all PASS; full make test PASS (process exit 0); make dqddtcheck relocdrvcheck f11channelcheck f11ioccheck sysexecentrycheck sourcerangecheck PASS; source range validation PASS (1,957 ranges, 0 unmapped functions; inventory counts unchanged). Real python3 tools/qemu_sysboot_m193.py PASS for the existing M193 IRQ14 boot regression; guest still stops at the known partial INIT boundary, with no login/DCL. Runner terminated. Raw COM1 transcript logs/serial/qemu-m193-run-local-20261123T042427302970Z.log, 4,600 bytes, SHA-256 b83ec2a164adef10221c110aba147dc906b7174b69c6ba0886e5d700589c2984; manifest verification PASS. Public diff audit found no DEC source, image, archive, or recognized secret material.

An additional harness run recorded byte-identical M193 serial evidence at logs/serial/qemu-m193-run-local-20261123T042841960367Z.log; serial manifest validation passes with six raw transcripts.

Original source coverage: DQDRIVER.MAR 634-639 now maps partial to the staged READPBLK dispatch. All remaining DQA0 DDT/CRB, RB730 semantics, public IOC/$QIO, ACP, and process completion remain unresolved. Totals remain 28,697 partial, 50,504 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped; 441/4,566 original files indexed.

M195 implementation commit: `33802ac` (`33802aca2b8d2c17456319544463354dd1b3a01a`).

## M196 — Resident DQ staged read through ATA IRQ14, 22 November 2026

Resident SYS.EXE now accepts the registered native ATA read callback at its DQ DDT-stage INIT phase, builds a separate resident physical READPBLK IRP for LBN0, validates the staged DQ DDT/FDT and invokes the guarded private read bridge. Kernel handoff verifies a new IRQ14 completion and the returned IRP/media/sector result. This is a real guest read through the staged gate; the DDT image has no callable action slot and DQA0 DDB DDT/UCB CRB are still unpublished. Public SYS$QIO, IOC process ownership, ACP and AST delivery remain absent.

Verification: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0); `make dqddtcheck relocdrvcheck f11channelcheck f11ioccheck sysexecentrycheck sourcerangecheck` PASS; source-range validation PASS (1,957 ranges, 0 unmapped functions; inventory counts unchanged). Real `python3 tools/qemu_sysboot_m196.py` PASS; expected source-order messages include the new SYS.EXE DQ READPBLK/IRQ14 stage and the guest's honest incomplete-INIT stop. No login or DCL observed; QEMU runner terminated. The initial harness attempt recorded a false harness failure because its expected serial string had not been updated; actual guest output reached the incomplete-INIT stop. After updating the expected string, M196 QEMU passed. Both transcripts are retained in the manifest. Final successful COM1 transcript `logs/serial/qemu-m196-run-local-20261123T043848030722Z.log`, 4,624 bytes, SHA-256 `d5bbb1bec480938886b7a2acfb938af07738ed07e424eb1797a8ec971eab9e35`; manifest validation reports 11 raw logs.

Coverage: DQDRIVER.MAR 634-639 remains partial and now reaches the staged DDT/FDT-gated physical read in QEMU. DQA0 DDT action slots, DDB/UCB publication, CRB/IRQ ownership, RB730 command/error semantics, public SYS$QIO, Files-11 ACP and process AST completion are unresolved. Counts remain 28,697 partial, 50,504 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped; 441/4,566 original files indexed.

M196 implementation commit: `fb6e7ee` (`fb6e7eecc4e90bc2804ba6762d5b71789e34c93b`).


## M197 — Resident DQA CRB with live IRQ14 completion, 22 November 2026

Added a source-shaped 108-byte CRB record with a native IRQ14 callback extension. `arch_trap` dispatches vector 46 through the CRB and acknowledges the PIC. Resident SYS.EXE stages DQ DDT/FDT and initializes the CRB; kernel binds that CRB into the IDT route. After the existing private DQ DPT_STORE and DDT relocation checks, resident SYS.EXE attaches the CRB pointer at the DQA UCB and issues the guarded physical READPBLK. Kernel verifies the UCB CRB pointer, unchanged zero DDB DDT/UCB DDT fields, ATA completion count and CRB interrupt count. This is actual guest CRB-based interrupt completion, not DQA DDT action dispatch or public SYS$QIO.

Verification: `make -j2 all dqcrbcheck` PASS; `make test` PASS (exit 0, including new CRB host regression); `make -j2 all dqddtcheck relocdrvcheck f11channelcheck f11ioccheck sysexecentrycheck sourcerangecheck` PASS; source coverage validation PASS (1,957 ranges, zero unmapped functions; 441/4,566 originals indexed, status totals unchanged). QEMU M197 initially exposed an expected-message ordering error in the new harness; after correction it passed. That first pass used an intermediate CRB record size and was superseded after SYSDEF.MDL layout inspection established the 72-byte standard CRB plus two 36-byte VECs (108 bytes total). Final rebuilt `python3 tools/qemu_sysboot_m197.py` PASS confirms the corrected 108-byte record, CRB attachment, physical READPBLK completion through IRQ14, then the still-incomplete INIT stop. No login, DCL or DECnet initialization was observed. Retained M197 attempts: failed harness ordering `logs/serial/qemu-m197-run-local-20261123T044826626242Z.log` (4,713 bytes, SHA-256 `16124d5b5c46cb5c573ca18d156adf0d14e4df39296ac68b54b404f4e457430a`); intermediate-layout pass `logs/serial/qemu-m197-run-local-20261123T044846295706Z.log` (4,710 bytes, SHA-256 `dccce7d9535fd25d40f63116b2eb958d1bc644c4f554fbefbcbf3cd767d698ed`); final-layout pass `logs/serial/qemu-m197-run-local-20261123T045325614312Z.log` (4,713 bytes, SHA-256 `16124d5b5c46cb5c573ca18d156adf0d14e4df39296ac68b54b404f4e457430a`). Manifest verifies 14 raw logs. QEMU runner terminated; no QEMU process remains.

Coverage: DQDRIVER.MAR integration note now records native UCB CRB publication and CRB-dispatched IRQ14 completion as partial coverage. DQA DDB DDT/action slots, RB730 semantics, public IOC/$QIO, Files-11 ACP and process AST completion remain unresolved. Native INIT, scheduler/process image activation, genuine login/DCL, DECnet Phase IV and SET HOST 0 remain incomplete.

M197 implementation commit: `64d4d57e6e8e8e3985a34d1eb5a2b8ea44deacfa`.
## M198 — Resident DQ IOC FIFO over the attached CRB, 22 November 2026

The resident private READPBLK request now enters the existing bounded DQ IOC FIFO state machine (prepare, enqueue, start, retry/transfer, completion/post) before its physical ATA callback. That callback completes through the attached DQA CRB and IDT IRQ14 path; data is committed to the resident sector only after successful IOC completion. This remains a single private supervisor boot read: DQA DDB DDT action slots, public process `$QIO`, Files-11 ACP and process AST delivery are not implemented.

Verification after the change: `make -j2 all dqprivateioccheck dqcrbcheck sysexecentrycheck` PASS; `make test` PASS (exit 0, including 32 IOC host regressions and the CRB host regression); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; 441/4,566 originals indexed, status totals unchanged); real `python3 tools/qemu_sysboot_m198.py` PASS. COM1 shows one resident DQ IOC FIFO READPBLK completed through the attached CRB IRQ14, then the existing incomplete INIT stop. No login, DCL or DECnet initialization was observed. QEMU runner terminated; no QEMU process remained. Raw transcript `logs/serial/qemu-m198-run-local-20261123T045801658353Z.log`, 4,697 bytes, SHA-256 `8a0693a1dd9b91c0ba132bcf81c587fb3ae15d9d097e60b3acd38913583ad05c`; serial manifest verifies 15 raw logs.

DQDRIVER.MAR coverage now maps the resident single-read queue/transfer path as partial. Native full driver dispatch, physical RB730 control/error handling, public SYS$QIO, Files-11 ACP, AST completion, scheduler-driven INIT and process activation remain unfinished. Genuine SYSTEM login, DCL, DECnet Phase IV and SET HOST 0 remain unverified and incomplete.

M198 implementation commit: `4c7933768bfb3948f386f840bd8eaa22e9e4836a`.