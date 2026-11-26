# Native VAX/VMS 3.7 IA-32 recovery, updated through M390

INCOMPLETE. M185-M192 implements owner-scoped private Files-11 channel handles with 32 host cases; QEMU assigns a native private handle, checks 600 real ATA bytes via two IRPs and releases it. M205-M207 publish a partial native DQA0 DDT and CRB pointer graph, route physical READPBLK and WRITEPBLK through the published DDT STARTIO via IOC$SEARCHDEV, verify real ATA IRQ14 readback/restore, reject inconsistent published DDB/UCB/CRB backlinks, reread the restored sector through the published DDT, and route guarded reads and writes through bounded FIFOs, with ordered post-publication multi-request read completion and writes. M216/M217 read and verify 64 then 700 SYS.EXE bytes from the Files-11 FH2 chain through IOC$SEARCHDEV-selected published DQA0 DDT STARTIO and real IRQ14, crossing a sector boundary. M218-M225 add host-tested SYS$QIO argument/preflight/IOSB/quota/device-access stages; M234-M236 add a bounded IRP lookaside allocator, resident DQA0 IRP allocation, and an isolated SYS$QIO IRP-reservation stage. They are not wired to a live user service or process-aware DQA dispatcher. INI_PSTATE now points at source-layout SYSTEM/NULL PCB records, but their PHD/native contexts and process activation remain incomplete. Public process $ASSIGN/$QIO, full driver completion, ACP, login, DCL and DECnet remain unimplemented. Genuine SYSTEM login,
DCL, DECnet Phase IV and SET HOST 0 are not implemented.

Active repository: tuklusan/VAX-VMS-3.7-Baremetal-IA-32-Pentium main
Recovery reference branch: tuklusan/OpticOS vms37-x86-qemu-m45-ci (read-only)
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
## M199 — Staged DQ READPBLK action dispatch, 23 November 2026

The staged source-shaped DQ FDT now has one tagged resident native READPBLK action address. The FDT function classifier permits only that function; the private DDT dispatcher validates the stage and invokes that action for the existing privileged boot IRP. The source relocation marker preserves the absolute resident address in the offline DDT relocation path. All remaining DQ actions remain unbound. This does not publish DQA0 DDB/DDT through IOC, implement DQ_STARTIO/RB730 semantics, or expose public process SYS$QIO; the operation remains the one supervisor INIT READPBLK. Files-11 ACP, process AST completion, scheduler activation, login, DCL and DECnet remain unfinished.

Verification: `make -j2 all dqddtcheck relocdrvcheck sysexecentrycheck` PASS; full `make test` PASS; `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; 441/4,566 original files indexed; 28,697 partial, 50,504 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `python3 tools/checkpoint_serial_logs.py verify` PASS (21 transcripts); `git diff --check` PASS. Real `make qemu-sysboot-m199` PASS: COM1 reaches the private DQ IOC FIFO READPBLK through attached CRB IRQ14 and then honestly stops at the incomplete INIT boundary; no login, DCL or DECnet start observed. QEMU child terminated. Latest repeated-run transcript `logs/serial/qemu-m199-run-local-20261123T051223571928Z.log`, 4,697 bytes, SHA-256 `8a0693a1dd9b91c0ba132bcf81c587fb3ae15d9d097e60b3acd38913583ad05c`. Earlier M199 attempts revealed and recorded the relocation integration mismatch and harness milestone ordering; failed transcripts remain labeled `harness-fail` in the manifest. Final public diff audit found no original DEC source, disk/image binaries, or recognized credential/private-key patterns.

M199 implementation commit: `6d9258d9e75e9269a0be76789b946eedf457b1a0`. Continue with true IOC publication of DQA0 DDB/DDT and source DQ_STARTIO dispatch, then public process `$QIO` and ACP/AST completion; do not mark this staged boot action as full DQA support.

## M200 — Native ATA PIO sector write transport, 23 November 2026

Added a bounded one-sector ATA LBA28 PIO write primitive with a separate write-bus contract. The IA-32 implementation polls the initial DRQ phase, arms IRQ14 before sending the data block, and waits for device command completion; a bounded polling fallback supports low-level callers. The M200 host mock did not exercise device timing; the following M201 QEMU regression does. Host mocks verify register selection/order, all 256 output words, high LBA nibble, hook pairing, and invalid LBA rejection. This is hardware transport only: it has not yet been exercised against QEMU ATA writes, connected to a DQ IRP/DDT action, exposed through DQA0 IOC, or made available to public SYS$QIO. Original DQ write/error semantics remain incomplete.

Verification: `make -j2 all` PASS (including freestanding IA-32 `vms_ata_x86.c`); full `make test` PASS, including `M10 ATA host PASS`; DQ DDT and relocation host checks PASS; `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions); `git diff --check` PASS. Real QEMU M199 boot regression PASS on the rebuilt image and reaches the same DQ READPBLK/CRB IRQ14 milestone before the honest incomplete-INIT stop. This boot did not issue a write command. Raw COM1 transcript `logs/serial/qemu-m199-run-local-20261123T052051359529Z.log`, 4,697 bytes, SHA-256 `8a0693a1dd9b91c0ba132bcf81c587fb3ae15d9d097e60b3acd38913583ad05c`; serial manifest verification PASS. No original DEC source, image binary, or recognized credential material entered the commit.

M200 implementation commit: `484ff22bc8fc779d4d4ab8e0c14c4f3ccb61a37f`. Next integrate native write through a validated DQ request and test it only on a disposable QEMU snapshot, then continue DQA DDB/DDT publication and public SYS$QIO/ACP completion.


## M201 — ATA write IRQ completion on disposable QEMU media, 23 November 2026

The real IA-32 ATA writer now follows the observed QEMU/ATA PIO sequence: bounded status polling for initial DRQ, then IRQ14 armed before the 256-word payload and a real completion interrupt before success. The M201-only kernel regression is gated by the QEMU fw_cfg signature plus a complete marker pattern in LBN 8063. Its runner copies `portdisk.img` to an ignored disposable image, marks that scratch sector, and runs QEMU with snapshot mode. The guest saves the marked sector, writes a distinct pattern, reads and compares it, restores the exact saved sector, and reads/compares the restoration before reporting success. Normal hardware without QEMU fw_cfg and ordinary QEMU boots without the marker do not issue writes. This validates hardware transport only; DQ WRITEPBLK/STARTIO, DQA0 DDB/DDT IOC publication, public SYS$QIO, ACP, and process-aware completion are still unimplemented.

Verification: `make -j2 all` PASS; full `make test` PASS (including ATA host register/data sequence and all existing regressions); `make qemu-sysboot-m201` PASS with actual QEMU IRQ14 write completion, readback, and restore on the marked disposable disk; `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; 441/4,566 original files indexed; inventory counts unchanged); `python3 tools/checkpoint_serial_logs.py verify` PASS (26 raw transcripts); `git diff --check` PASS. The final M201 transcript is `logs/serial/qemu-m201-run-local-20261123T052944049192Z.log`, 4,847 bytes, SHA-256 `1b3c371c82c202ef89685f288c9ad746317222b5bf8f8e497381ab6ee9e35974`. Three earlier M201 harness attempts are preserved and labeled `harness-fail`; the final runner terminated QEMU and removed the disposable disk copy. M201 does not advance the guest beyond incomplete INIT; no SYSTEM login, DCL or DECnet was observed.

M201 implementation commit: `d8fc96605e018bbf2f54ec625794ec80a0fcd579`. Resume by connecting tested ATA write transport to a validated DQ IRP and source DDT action only after implementing the necessary VMS dispatch and completion semantics, then proceed to public SYS$QIO and Files-11 ACP.

## M202 — Staged DQ WRITEPBLK action over ATA IRQ14, 23 November 2026

The staged source-shaped DQ FDT now binds native private READPBLK and WRITEPBLK action slots. Resident SYS.EXE exposes the WRITEPBLK dispatcher through its INIT result, and the action validates the staged DDT/FDT, decodes the physical LBN, calls the ATA LBA28 sector writer, and updates UCB accounting and IRP IOSB fields after hardware completion. The QEMU regression invokes this staged action for a scratch sector only when QEMU fw_cfg and the complete marker sector are present. It saves the sector, writes through DQ WRITEPBLK/ATA IRQ14, verifies the data, restores the original sector through the same action, and verifies restoration. The runner uses a disposable disk clone and snapshot mode. This is a private supervisor request, not IOC-published DQA0 DDB/DDT, public process SYS$QIO, Files-11 ACP, or process AST completion. INIT still halts at the existing incomplete boundary.

Verification: `make -j2 all dqddtcheck relocdrvcheck sysexecentrycheck` PASS; `make test` PASS (full suite, including host DQ DDT WRITEPBLK dispatch); `make qemu-sysboot-m202` PASS on the final renamed/rebuilt kernel, confirming guest DQ WRITEPBLK action, ATA IRQ14 completion, readback, restore and honest incomplete-INIT stop. An earlier M202 harness run reached the same successful guest write but failed the runner's stale expected message; retained as `harness-fail`. `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; 441/4,566 original files indexed; totals unchanged); `python3 tools/checkpoint_serial_logs.py verify` PASS (29 raw transcripts); `git diff --check` and credential-pattern audit PASS. Final raw COM1 log: `logs/serial/qemu-m202-run-local-20261123T054529603544Z.log`, 4,848 bytes, SHA-256 `d8867d252401fe35c7cfbcb029e07fa5db667789857fe847e8171e61fa1e354a`. QEMU runner terminated; disposable disk copy removed. No login, DCL or DECnet was observed.

Coverage: DQDRIVER.MAR lines 634-639 now describe both staged native READPBLK/WRITEPBLK action slots as partial coverage. No original source lines changed to complete. DQA0 DDB/DDT publication, native STARTIO integration to IOC-owned requests, full RB730 command/error behavior, public SYS$QIO integration, ACP file operations, process AST completion, scheduler-driven INIT/process activation, SYSTEM login, DCL, DECnet Phase IV and SET HOST 0 remain incomplete. Do not treat private staged dispatch as public I/O or a complete DQA driver.

M202 implementation commit: `e37183a3fcaac47e2b1f1a7eba6d9602b6352c3f`.

M202 re-verification after the M203 write-queue experiment: the experimental extension generalized the private IOC FIFO to own WRITEPBLK payloads. Host queue and full host suites passed, but the rebuilt guest failed before completing the established private DQ boot-read path. The unverified code was discarded; no M203 code was committed. The clean M202 source rebuilt and `make qemu-sysboot-m202` passed again. The latest raw transcript is `logs/serial/qemu-m202-run-local-20261123T060323524568Z.log`, 4,848 bytes, SHA-256 `d8867d252401fe35c7cfbcb029e07fa5db667789857fe847e8171e61fa1e354a`; the serial manifest verifies 46 real transcripts, including failed experiment runs. M202 source, coverage and hardware result remain as above. Continue with the next change isolated from the validated private read path, and require a fresh real QEMU boot before committing.

## M203 — DQ WRITEPBLK through a private IOC FIFO, 23 November 2026

Added an isolated source-shaped DQ write FIFO with eight owned 512-byte payload slots, FIFO ordering, queued status, ATA backend dispatch, and source-style status/byte-count IOSB completion. The staged DQ WRITEPBLK action now submits through this FIFO and updates the VMS UCB/IRP only after successful hardware completion. The already validated READPBLK queue layout and path were left intact after a broader shared-queue experiment regressed guest boot; that failed experiment and its transcripts are retained separately. The QEMU M203 regression confirms the real DQ DDT-gated WRITEPBLK action reaches the write FIFO, completes a real ATA IRQ14 write, reads back the test pattern, restores the original marker sector, and verifies restoration on a disposable disk clone in snapshot mode.

Verification: `make -j2 all dqwriteioccheck dqddtcheck relocdrvcheck sysexecentrycheck` PASS; full `make test` PASS (including FIFO ordering and payload-ownership host coverage); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; 441/4,566 original files indexed; inventory totals unchanged); `python3 tools/checkpoint_serial_logs.py verify` PASS (48 transcripts); `git diff --check` and credential-pattern audit PASS. `make qemu-sysboot-m203` PASS. Final COM1 log: `logs/serial/qemu-m203-run-local-20261123T060947132042Z.log`, 4,853 bytes, SHA-256 `c6b52161873a63f79f93565fe70b5ff4c0bd22886d4616dd0007bbb8af42e9a7`. The runner terminated QEMU and removed its disposable disk image. No login, DCL, or DECnet was observed.

Coverage: DQDRIVER.MAR lines 634-639 remain partial, now including the private queued WRITEPBLK action. This still does not publish the DQA0 DDB/DDT through IOC or expose public process `$QIO`; full RB730 behavior, Files-11 ACP, process AST completion, scheduler-driven INIT/process activation, SYSTEM login, DCL, DECnet Phase IV and SET HOST 0 remain incomplete. Source inventory totals remain descriptive; no original source file coverage became complete.

M203 implementation commit: `8623858c48d8b5270984f1dd0843643bb749ee9a`.
## M203 re-verification — 23 November 2026

Rebuilt the native image and targeted DQ/RELOCDRV/SYS.EXE host check binaries; `make test` PASS (exit 0); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; indexed inventory/counts unchanged); `python3 tools/checkpoint_serial_logs.py verify` PASS (50 raw transcripts); `git diff --check` PASS. A fresh `make qemu-sysboot-m203` PASS: disposable QEMU media WRITEPBLK through staged DDT, DQ private IOC FIFO, ATA IRQ14, readback and restore all verified. QEMU terminated normally, disposable disk removed, and no QEMU process remained. The guest still halts at the declared incomplete INIT boundary; no login, DCL or DECnet initialization observed. Latest repeated-run transcript `logs/serial/qemu-m203-run-local-20261123T061559056659Z.log`, 4,853 bytes, SHA-256 `c6b52161873a63f79f93565fe70b5ff4c0bd22886d4616dd0007bbb8af42e9a7`.

No functional source or coverage status changed in this verification. The tested implementation remains at commit `8623858c48d8b5270984f1dd0843643bb749ee9a`; previous resume record commit `4c412d70a4f64d9bee0b9ca50e6ebd085ec54e5e`. No success criteria beyond the M203 private physical write regression have been met.


## M204 — Native DQ_STARTIO dispatch through staged DDT, 23 November 2026

The resident DQ DDT START slot now identifies a native DQ_STARTIO adapter. It validates the staged DDT/FDT and private IRP, classifies the function, dispatches READPBLK or WRITEPBLK through the corresponding FDT action slot, and fails closed for legal but unsupported actions and illegal function codes. SYS.EXE boot READPBLK and the disposable QEMU WRITEPBLK/readback/restore regression now exercise this STARTIO entry via its function pointer returned by resident SYS.EXE. DQA0 DDB/UCB DDT fields remain unpublished and DQA remains unavailable through IOC; the adapter is restricted to the existing privileged private physical-I/O request contract. It is not public process SYS$QIO, buffered/ACP I/O, full DQ/RB730 behavior, or process completion.

Coverage: DQDRIVER.MAR lines 634-639 now map partial to src/vms_dqddt_bootread.c:port_dqddt_startio. Other source driver commands, live DDB/UCB DDT publication, SYS$QIO, Files-11 ACP, process AST completion and scheduler/process activation remain incomplete. The guest still stops at incomplete INIT; no SYSTEM login, DCL or DECnet startup was observed.

Verification: make -j2 all dqddtcheck PASS; make relocdrvcheck sysexecentrycheck PASS; full make test PASS (exit 0); python3 tools/validate_source_ranges.py PASS (1,957 ranges, zero unmapped; 441/4,566 original files indexed; inventory counts unchanged); python3 tools/checkpoint_serial_logs.py verify PASS (53 raw transcripts); git diff --check PASS. make qemu-sysboot-m204 PASS on disposable media: real ATA IRQ14 WRITEPBLK, readback, restore and reread completed through DQ_STARTIO. Runner terminated QEMU and removed scratch media; no QEMU process remained. Final COM1 log logs/serial/qemu-m204-run-local-20261123T063030854847Z.log, 4,853 bytes, SHA-256 d4424f82e90fe120d841daf38fb574f38910e643c4f8fcd85bb2179da759a296. Candidate commit audit found no DEC source text, restricted source files, disk images, recognized credentials or private keys. Original source checkout remained pinned at 2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752; no build dependencies were added.

M204 implementation commit: f86096995207d9b955f590f9494fbe1b526a6cc0.

## M205 — Native DQA0 DDT/FDT activation and IOC graph publication, 23 November 2026

Resident SYS.EXE now materializes the source DQ DDT/FDT at its actual IA-32 address, relocates the header and action slots to native pointers, and uses fail-closed native stubs for unsupported actions. It publishes the active DDT address into the DQA0 DDB and UCB, retaining the already attached IRQ14 CRB in the UCB. The existing physical READPBLK/WRITEPBLK FDT actions execute through the active table. SYS.EXE then calls the existing physical `IOC$SEARCHDEV` path and validates the DQA0 DDB/UCB/unit and bound-driver result. This remains a partial device driver: no public SYS$QIO or process channels, buffered/virtual/ACP services, full RB730 controller/error behavior, or process AST delivery. The private test path still does not constitute complete IOC request scheduling.

Coverage: `DRIVER/SRC/DQDRIVER.MAR` lines 438-471 now map partial to `port_dqddt_activate`; lines 634-639 map partial to `port_dqddt_startio` and note the published DQA0 graph and remaining public I/O/ACP gaps. No original file is marked complete; inventory status totals remain unchanged. The native guest still halts at the original incomplete INIT boundary before login; no SYSTEM login, DCL or DECnet activity was observed.

Verification: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped functions; 441/4,566 original files indexed; counts unchanged); `python3 tools/checkpoint_serial_logs.py verify` PASS (57 raw logs); `git diff --check` PASS for source/doc changes. `make qemu-sysboot-m205` PASS twice on disposable QEMU media: resident DDT/FDT action dispatch performed actual ATA IRQ14 WRITEPBLK, readback and restore, then SYS.EXE published the native DQA0 DDT pointers and `IOC$SEARCHDEV` found DQA0. One intervening QEMU run failed during native DDT/CRB publication validation; the real failure log is retained. Latest COM1 log `logs/serial/qemu-m205-run-local-20261123T065319671972Z.log`, 5,042 bytes, SHA-256 `13c70ec84d6a903867651246ea53f726c5d2133571d2ca799a28ff3507ac6a64`. QEMU was terminated by the runner; no QEMU process remains. Original source checkout remains at `2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752`. Candidate audit found no restricted DEC source, credentials, private keys, disk images or new build dependencies.

M205 implementation commit: 974a9a31273bcc887f396231f75569cb03620646.

## M206 — Route IRQ14 boot READPBLK through native DQ STARTIO, 23 November 2026

The resident SYS.EXE DQ CRB-read phase now calls the START function pointer from the activated native DDT, which enters `port_dqddt_startio` and dispatches the real READPBLK through the native DDT action slot. That request completes through the attached CRB and actual ATA IRQ14 before the later DQA0 DDB/UCB DDT publication phase. It keeps the existing privileged boot IRP contract; it does not implement public `$QIO`, process I/O, general RB730 interrupt/error processing, ACP, or process AST completion.

Verification: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m206` PASS (actual DDT STARTIO READPBLK through CRB IRQ14, followed by M205 DQA0 DDT publication and IOC search); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges; no missing mapped functions; inventory totals unchanged); `python3 tools/checkpoint_serial_logs.py verify` PASS (60 raw transcripts); `git diff --check` PASS. Latest raw COM1 log: `logs/serial/qemu-m206-run-local-20261123T065837305021Z.log`, 5,020 bytes, SHA-256 `a572600c1f03990b44e4432c812a82f7fd7a66c0f434ce5a8911f06ccab39e2d`. The guest still halts at incomplete INIT; no login, DCL or DECnet was observed. QEMU was terminated and no runner remains.

M206 implementation commit: 62191a4007ec435e6a8efc3310a5f3fce9377201.


## M207 — IOC-selected DQA0 DDT WRITEPBLK, 23 November 2026

Resident SYS.EXE now uses its actual `IOC$SEARCHDEV` callback to find DQA0 after DDT publication, validates the selected DDB/UCB/unit and published DDT START address, and invokes the STARTIO pointer from that DDT for the guarded physical WRITEPBLK test. The DDT adapter marks the request as active only after native DDT validation; preflight checks that DDB and UCB still point to that published DDT and the CRB remains present. The disposable scratch sector is written through ATA IRQ14, read back, restored and reread. Host tests check that an unselected/missing active DDT request is rejected and the selected request dispatches. This remains a SYS.EXE privileged physical-I/O regression; public SYS$QIO, process I/O, Files-11 ACP, AST completion, scheduler/process activation and general DQ/RB730 behavior are not implemented.

Coverage remains partial: DQDRIVER.MAR lines 634-639 are indexed to `port_dqddt_startio`; no source file status was promoted. `python3 tools/validate_source_ranges.py` PASS: 1,957 ranges, zero unmapped functions, 441/4,566 original repository files indexed; statuses partial 28,697, unresolved 50,504, missing 7,799, mapped 17,065, excluded 26,217, hardware-excluded 917. These inventory numbers are not functional completion claims.

Verification at implementation commit `80912fedd5b6e10b6a90caacaf9ef7d239f8ba44`: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m207` PASS with real DQA0 IOC-selected DDT STARTIO WRITEPBLK over ATA IRQ14, verified readback and restore, then the explicit incomplete-INIT stop. `python3 tools/checkpoint_serial_logs.py verify` PASS (66 raw transcripts); `git diff --check` PASS. Final COM1 transcript `logs/serial/qemu-m207-run-local-20261123T071545580409Z.log`, 5,029 bytes, SHA-256 `c0ab5120ca6423aaafbe894e35101503f8a229a4ea0367ca58143ab8449a1f32`. Two failed runs are retained: `...T071040833647Z.log` stopped on stale SYS object/header dependency; `...T071148975653Z.log` failed the DQA write path with root cause undetermined. Three further raw transcripts, including the final identical-hash pass, record passing runs. Runner terminates QEMU; no QEMU process remains. No login, DCL, or DECnet was observed.

Candidate audit found no original DEC source files, binary images, recognized credentials/private keys or new dependencies staged. Original source checkout remains pinned at `2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752`. M207 implementation commit: `80912fedd5b6e10b6a90caacaf9ef7d239f8ba44`.


## M208 — Validate published DQA0 request backlinks, 23 November 2026

Hardened the privileged DQ request preflight for post-publication operation. It now requires the UCB's CRB backlink to equal the validated DQA0 CRB address, in addition to requiring the published DDT base to match the active STARTIO context and both DDB/UCB DDT pointers. Host tests independently corrupt the UCB CRB, UCB DDT and DDB DDT pointers and verify rejection before dispatch. The guarded prepublication INIT request path remains available. This is request graph validation, not general IOC scheduling or public process QIO.

Verification at implementation commit `d04ca2698fa761da70d299531553e9a57c84be93`: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m207` PASS with actual IOC-selected DQA0 DDT WRITEPBLK, ATA IRQ14, readback and restoration; source-range validation PASS (1,957 ranges, zero unmapped functions; inventory statuses unchanged); serial manifest PASS (67 raw logs); diff whitespace check PASS. QEMU transcript `logs/serial/qemu-m207-run-local-20261123T072146092441Z.log`, 5,029 bytes, SHA-256 `c0ab5120ca6423aaafbe894e35101503f8a229a4ea0367ca58143ab8449a1f32`. No QEMU process remains. Original source checkout remains pinned; no DEC sources, binary images, secrets, or dependencies were added.

M208 implementation commit: `d04ca2698fa761da70d299531553e9a57c84be93`. The guest still stops at incomplete INIT. Public SYS$QIO, Files-11 ACP, process-aware AST completion, scheduler/process image activation, SYSTEM login, DCL and DECnet Phase IV remain unfinished.


## M209 — Post-publication DQA0 READPBLK, 23 November 2026

Extended the DQA0 DDT dispatch path to issue both READPBLK and WRITEPBLK through the START pointer selected from the published DDB after `IOC$SEARCHDEV`. On the disposable QEMU disk, the test saves the marked scratch sector, writes a pattern through the active DDT, checks the physical readback, restores the original contents, confirms physical restoration, then reads the sector again through published DDT READPBLK and compares the returned bytes. The DDT request constructor now supplies both read and write backends. Coverage for DQDRIVER.MAR lines 634-639 records both physical functions through the published DDT. This remains privileged private INIT I/O and does not implement public process SYS$QIO, buffered I/O, ACP, AST completion, or general driver error/controller semantics.

Verification at implementation commit `6170c013bece6ae9e9f99e807bfb90ee3dec5a6f`: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m209` PASS twice. Both real serial traces show IOC-selected DQA0 DDT WRITEPBLK and READPBLK over ATA IRQ14, verified restoration and the explicit incomplete-INIT stop. `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped; 441/4,566 original files indexed; status counts unchanged); serial manifest PASS (69 logs); `git diff --check` PASS. Logs `logs/serial/qemu-m209-run-local-20261123T072529189421Z.log` and `...T072637343273Z.log`, each 5,042 bytes, SHA-256 `3a60452228235d3d9f8dd7f2ae8d110dd51e05bf55fdb36b17de985b4c8a0427`. QEMU terminated; no runner remains. Candidate audit found no DEC source, binary images, secrets or new dependencies.

M209 implementation commit: `6170c013bece6ae9e9f99e807bfb90ee3dec5a6f`. VMS initialization, login, DCL and DECnet remain incomplete.


## M210 — DQA0 write through the FIFO, 23 November 2026

The QEMU scratch-sector test now submits each WRITEPBLK payload through the existing bounded `port_dq_ioc_write` FIFO. The FIFO owns a copy, retains ordering/tag state, invokes a backend that resolves DQA0 through `IOC$SEARCHDEV`, calls STARTIO from the published DDB DDT, and returns a checked completion status/byte count before readback and restore. The kernel links the FIFO implementation directly. This is one synchronous privileged private FIFO request per service call; it is not public process SYS$QIO, a general resident IOC scheduler, process AST delivery, or ACP I/O.

Verification at implementation commit `88920c95d8618f8bc10fbbb6b443eae062cb8fd2`: `make -j2 all dqwriteioccheck` PASS; host FIFO checks PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m210` PASS. The real COM1 transcript confirms FIFO-dispatched DQA0 DDT WRITEPBLK and READPBLK via ATA IRQ14, physical readback/restoration, and the explicit incomplete-INIT stop. Source coverage validation PASS (1,957 ranges, zero unmapped functions; status counts unchanged); serial manifest verification PASS (70 raw logs); `git diff --check` PASS. Log `logs/serial/qemu-m210-run-local-20261123T072931380268Z.log`, 5,043 bytes, SHA-256 `f8bbf12bedd43cc193a3b571b8e10d2b3212b63963fd92858896a514eb69dbce`. QEMU terminated; no runner remains. Candidate audit found no restricted source, binary image, recognized secret, or new dependency.

M210 implementation commit: `88920c95d8618f8bc10fbbb6b443eae062cb8fd2`. VMS INIT still stops before ACP/process creation; public QIO, login, DCL, and DECnet remain incomplete.


## M211 — DQA0 READPBLK through IOC completion FIFO, 23 November 2026

The post-publication scratch-sector READPBLK now traverses the existing `port_dq_ioc` FIFO and its retry, transfer, IOC post, and completion path. Its hardware callback resolves the DQA0 unit with `IOC$SEARCHDEV`, invokes the START entry published in the selected DDB's native DDT, and returns the actual ATA IRQ14 sector. The queue copies the completed sector to its destination and the guest verifies it against the saved/restored media contents. Kernel linkage now includes the existing bounded private DQ IOC implementation. The write path still uses the existing private write FIFO. Both are synchronous bootstrap regressions with one request, not public SYS$QIO, process scheduling, asynchronous request dispatch, or process AST completion.

Verification at implementation commit `e554665125f4ec1508ac71f9a1dfe47d312c07ae`: `make -j2 all dqprivateioccheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m211` PASS (IOC-selected DQA0 DDT write FIFO and read FIFO, actual ATA IRQ14, byte-verified readback/restoration, then incomplete-INIT stop). Source validation PASS (1,957 ranges, zero unmapped; 441/4,566 original files indexed; status totals unchanged); serial manifest PASS (71 raw transcripts); `git diff --check` PASS. Real COM1 transcript: `logs/serial/qemu-m211-run-local-20261123T073231758308Z.log`, 5,043 bytes, SHA-256 `b45a1c2747a493bc9195e9858dcb94150c627c88673332b2089b1070a599eb32`. QEMU terminated; no runner remains. No DEC sources, disk images, secrets or new dependencies were staged.

M211 implementation commit: `e554665125f4ec1508ac71f9a1dfe47d312c07ae`. SYS$QIO, Files-11 ACP, process-aware AST, scheduler/process-image activation, SYSTEM login, DCL and DECnet remain incomplete.


## M212 — Multiple DQA0 reads through IOC FIFO, 23 November 2026

The post-publication DQA0 read path now prepares and enqueues two requests before draining the existing `port_dq_ioc` FIFO: one reads the restored scratch sector, and the other reads LBN 0. The guest verifies FIFO processing, both successful completions/posts and byte-for-byte results against the restored scratch data and a separately acquired LBN 0 reference. Both queued requests enter the IOC-selected DQA0 DDB's published DDT START function and complete through physical ATA IRQ14. The implementation is still a bounded, synchronous bootstrap regression; it is not public process SYS$QIO, concurrent execution, or process AST completion.

Verification at implementation commit `33e600cbed187ba7f20a3dfe00c1abb293c7eb50`: `make -j2 all dqprivateioccheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m212` PASS with two actual queued post-publication DQA0 reads and verification of the prior queued write/restore. Source-range validation PASS (1,957 ranges, zero unmapped; counts unchanged); serial manifest PASS (72 transcripts); `git diff --check` PASS. Real COM1 log `logs/serial/qemu-m212-run-local-20261123T073452869042Z.log`, 5,043 bytes, SHA-256 `93217f0d9eaf017a2631ac8a9d6c1976075820589ed83dd8ffae963be09b4dea`. QEMU terminated; no runner remains. No original DEC source or binary image was staged.

M212 implementation commit: `33e600cbed187ba7f20a3dfe00c1abb293c7eb50`. The guest still halts at incomplete INIT. Public SYS$QIO, Files-11 ACP, process AST/scheduler integration, process image activation, SYSTEM login, DCL and DECnet Phase IV remain unfinished.


## M213 — Ordered multi-write DQA0 FIFO dispatch, 23 November 2026

The private DQA0 write FIFO now prepares two same-LBN WRITEPBLK IRPs with different payloads, enqueues both before service, verifies tag/status/byte count for each completion in enqueue order, and dispatches both payloads through IOC-selected DQA0 DDT STARTIO. The final payload is checked by physical ATA readback; the original scratch contents are restored by another two-write FIFO sequence, verified physically, then checked through the two-request READPBLK IOC FIFO. Guest validation checks the two read LBNs and their order, completion/post counts, media bytes, and empty retired packets. These are bounded synchronous supervisor INIT tests, not public SYS$QIO, asynchronous scheduling, process ASTs, or general DQ error recovery.

Verification at implementation commit `6842c70821130cc1fd0be039b1bcb80ba9854fd1`: `make -j2 all dqwriteioccheck dqprivateioccheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m213` PASS twice, with both logs showing the two ordered writes, two posted reads, real IRQ14 and verified media restoration before incomplete-INIT stop. `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped; inventory counts unchanged); serial manifest PASS (74 raw logs); `git diff --check` PASS. Final raw COM1 trace `logs/serial/qemu-m213-run-local-20261123T140926164590Z.log`, SHA-256 `973917730c6f3d75167d14aedefde8418d3ae28284ae7c3c4545423faddd6f53`; earlier passing trace `logs/serial/qemu-m213-run-local-20261123T140857732647Z.log`, SHA-256 `5405179af72992cdfcae37122ef5ac0c2ed4159402093f98f696cecda3eeed94`. QEMU terminated; no runner remains. Public candidate audit found no DEC source, restricted images or secrets.

M213 implementation commit: `6842c70821130cc1fd0be039b1bcb80ba9854fd1`. VMS initialization, ACP, public QIO, process activation, login, DCL and DECnet remain incomplete.


## M214 — Verify DQA0 FIFO backend dispatch order, 23 November 2026

Added guest-side records of each actual write backend invocation: LBN and first payload word are captured immediately before IOC-selected DQA0 DDT STARTIO dispatch. The QEMU test now compares these observations with the two queued payloads and verifies their order, in addition to checking returned tags, status and byte counts. This directly confirms the real DDT driver action received the queued payloads in order. It remains the private synchronous boot FIFO, not public QIO or process I/O.

Verification at implementation commit `7dae89739dd36e4b47f799f791135358a321c67b`: `make -j2 all dqwriteioccheck dqprivateioccheck` PASS; full `make test` PASS (exit 0); `make qemu-sysboot-m214` PASS; `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped; inventory counts unchanged); serial manifest PASS (75 raw logs); `git diff --check` PASS. Raw serial trace `logs/serial/qemu-m214-run-local-20261123T141142577386Z.log`, SHA-256 `5405179af72992cdfcae37122ef5ac0c2ed4159402093f98f696cecda3eeed94`. The guest shows two ordered WRITEPBLK payloads and two READPBLK completions through the published DQA0 DDT, with actual IRQ14, verified restoration, and the incomplete-INIT stop. QEMU terminated; no runner remains. No DEC source or binary image was staged.

M214 implementation commit: `7dae89739dd36e4b47f799f791135358a321c67b`. Public SYS$QIO, ACP, process AST/scheduling, native INIT completion, login, DCL and DECnet remain incomplete.


## M215 — DQA0 queued write failure posting, 23 November 2026

Extended the host regression for the bounded DQ WRITEPBLK FIFO to inject one backend failure followed by a valid request. It verifies the failed IRP receives an I/O error and zero byte count, that the FIFO continues to the next request, and that completion/failure counters and successful media contents match the two outcomes. This covers private FIFO failure posting only; no hardware error was fabricated and no guest behavior was changed.

Verification at test commit `af9806bbf71e442c6bac921c5736d786623245fc`: `make dqwriteioccheck` PASS; full `make test` PASS (exit 0, suite reached M49); `git diff --check` PASS. The immediately preceding M214 QEMU run remains the current guest evidence; this host-only change did not alter the boot image. No QEMU process was started for M215. Source coverage counts unchanged. Public SYS$QIO and process completion remain missing.

M215 test commit: `af9806bbf71e442c6bac921c5736d786623245fc`.


## M216 — Files-11 chain read through published DQA0 DDT, 23 November 2026

After resident SYS.EXE publishes DQA0 in the DDB/UCB and IOC$SEARCHDEV resolves the bound unit, INIT rereads 64 bytes at an unaligned SYS.EXE file offset by following the existing FH2 retrieval map. Physical sector requests use the published DDT STARTIO entry, a physical READPBLK IRP and real ATA IRQ14 completion. The returned bytes are compared with the independently cached SYS.EXE text. This verifies Files-11 chain callback integration with the published native DDT. It remains a supervisor INIT operation, not public SYS$QIO, a Files-11 ACP, process AST completion, or process-aware I/O.

Verification at implementation commit `4d35917c10c2a700e487e2aaf4fc52a02319af0d`: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped; 441/4,566 original files indexed; 28,697 partial, 50,504 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `python3 tools/checkpoint_serial_logs.py verify` PASS (76 raw logs). Real `make qemu-sysboot-m216` PASS; raw COM1 transcript `logs/serial/qemu-m216-run-local-20261123T142250878233Z.log`, 5,153 bytes, SHA-256 `6b565037e853b69bfe8c3eae60de36cf13b24828b26dc03fa8c25d37399bc268`. The guest reached the explicit incomplete-INIT stop; no SYSTEM login, DCL or DECnet initialization occurred. The QEMU runner terminated.

Coverage records this limited FILEREAD.MAR integration and does not count it as ACP coverage. Public SYS$QIO, Files-11 ACP, process-aware AST/scheduling, native INIT completion, process image activation, DECnet Phase IV runtime initialization, SYSTEM login/DCL and genuine SET HOST 0 remain unfinished.

M216 implementation commit: `4d35917c10c2a700e487e2aaf4fc52a02319af0d`.


## M217 — Cross-sector Files-11 read through published DQA0 DDT, 23 November 2026

Expanded the supervisor INIT chain reread from 64 bytes to 700 bytes at an unaligned SYS.EXE offset. This crosses a sector boundary; exactly two physical READPBLK IRPs must pass through IOC$SEARCHDEV-selected DQA0 DDT STARTIO and ATA IRQ14, and all returned bytes must match the independent cached FH2 chain read. This validates cross-sector mapped reads through the published DDT, but remains outside public SYS$QIO and ACP semantics.

Verification: `make -j2 all dqddtcheck sysexecentrycheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,957 ranges, zero unmapped; inventory counts unchanged); serial manifest validation PASS (78 raw logs). Two real `make qemu-sysboot-m217` runs PASS. Final raw COM1 log `logs/serial/qemu-m217-run-local-20261123T143002564868Z.log`, 5,170 bytes, SHA-256 `c56c3238f189713bdb2dd893cd2f85bc3aff5bd25cfc1d06e2000019709d79a9`; prior identical run `logs/serial/qemu-m217-run-local-20261123T142815661992Z.log`. Both reach the explicit incomplete-INIT stop; no login, DCL, or DECnet initialization was observed. QEMU runners terminated.

Coverage keeps FILEREAD.MAR partial and explicitly excludes ACP/public QIO from this guest path. Public SYS$QIO, Files-11 ACP, process-aware AST/scheduling, native INIT completion, process activation, DECnet runtime, SYSTEM login/DCL and genuine SET HOST 0 remain unfinished.

M217 implementation commit: `ab95489ad29dec563044e5e363cf80d84001152d`.


## M218 — Native SYS$QIO argument-list decoder, 23 November 2026

Added a bounded decoder for the original SYS$QIO AP layout: argument count plus the twelve fixed longword slots for EFN, channel, function, IOSB, AST address/parameter and P1-P6. It preserves the channel value and zero-extends the word-sized function as EXE$QIO does. Invalid alignment, short buffers and insufficient argument counts fail atomically. This helper assumes its input bytes have already been safely copied or validated by the caller; it does not implement user SYS$QIO, event-flag handling, channel authorization, device FDT dispatch, IRP allocation, IOSB completion or process AST delivery.

Verification: `make sysqioargscheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,959 ranges, zero unmapped; 441/4,566 original files indexed; 28,709 partial, 50,492 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run was needed: this host-tested helper is not linked into the guest image. No DEC source or binary image was staged.

SYSQIOREQ.MAR lines 112-123 now map as partial coverage to the decoder only; the complete service remains unimplemented. Public SYS$QIO, Files-11 ACP integration, process-aware AST/scheduling and image activation, genuine SYSTEM login/DCL, DECnet Phase IV and SET HOST 0 remain unfinished.

M218 implementation commit: `d2f8fab4ee10d8c25a28a492aca3cda40c0fec9b`.


## M219 — SYS$QIO channel/FDT preflight, 23 November 2026

Extended the SYSQIOREQ helper to apply the source channel-access threshold (`CCB$B_AMOD` is `MAXACMODE+1`), classify the low six I/O function bits through the DQ FDT while retaining the full function/modifier word, and allow only DEACCESS/ACP_CONTROL through an offline device. The source coverage now marks SYSQIOREQ lines 253-283 partial. This is a preflight primitive after a caller resolves the channel; it does not perform channel lookup, event flag clearing, IOSB probing, quota charging, protection checks, IRP allocation, FDT action invocation, or completion. It is not a public SYS$QIO service.

Verification: `make sysqioargscheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,961 ranges, zero unmapped; 441/4,566 originals indexed; 28,740 partial, 50,461 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: the host-tested preflight is not linked into the guest image.

M219 implementation commit: `2d7a360b014886af8273c1cf853be097a38b71a7`. Public SYS$QIO and all remaining process/device integration remain incomplete.


## M220 — SYS$QIO event-flag prelude, 23 November 2026

Implemented the SYSQIOREQ.MAR EFN boundary: the low byte of EFN is used; local EFNs 0-63 atomically clear the corresponding bit in the two-longword process-local flag cluster, while global EFNs call the executive clear callback and preserve its VMS condition status. Failed global clears are propagated. This helper is not yet wired into the public service entry or a live process PCB.

Verification: `make sysqioargscheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,963 ranges, zero unmapped; 441/4,566 originals indexed; 28,749 partial, 50,452 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: the host-tested EFN helper is not linked into the guest image. No DEC source or binary image was staged.

SYSQIOREQ.MAR lines 239-247 are recorded as partial coverage; live event flag clearing through a public SYS$QIO call, user service entry, channel lookup, IOSB/quota handling, driver dispatch and AST completion remain unimplemented.

M220 implementation commit: `e6e03d8b4db2f70418a0f6a27f3e09039f55fb06`.


## M221 — SYS$QIO IOSB probe and clear, 23 November 2026

Implemented the source IOSB step after channel/FDT/offline checks: a null IOSB is accepted; a nonnull address must pass a writable probe for the full eight-byte status block before an all-or-none clear callback runs. Probe failure leaves the caller block untouched. This contract still needs wiring to the live IA-32 process page tables and SYS$QIO path.

Verification: `make sysqioargscheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,965 ranges, zero unmapped; 441/4,566 original files indexed; 28,756 partial, 50,445 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: the helper is not linked into the guest image. No DEC source or binary image was staged.

SYSQIOREQ.MAR lines 286-292 are partial coverage only. Public SYS$QIO, process page-table callback wiring, quotas, full device-access checks, IRP allocation, dispatch, IOSB completion and AST delivery remain unfinished.

M221 implementation commit: `aab2a16cfd3a8b3d7baaab5e829e55d877c1a3d2`.


## M222 — Source-ordered SYS$QIO pre-dispatch, 23 November 2026

Added a source-ordered `EXE$QIO` begin routine: decode the AP list; clear the local/global EFN; resolve the caller's channel; check CCB access mode, DQ FDT legality and offline exceptions; then probe and clear the optional eight-byte IOSB. Failures after EFN handling post the error status to the requested event flag, and the prepared request output remains unchanged on failure. A follow-up correction moved `CCB$B_AMOD` out of device state and into the resolved channel, matching its source per-channel meaning. This routine prepares a request but does not invoke a driver FDT action, allocate an IRP, or enter public SYS$QIO from a live process.

Verification: `make sysqioargscheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,966 ranges, zero unmapped; 441/4,566 original files indexed; 28,751 partial, 50,450 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: the source-ordered service front end is host-tested and not linked into the guest.

SYSQIOREQ.MAR coverage now maps the EFN entry ordering, channel-mode/FDT preflight and IOSB initialization as partial. Real CCB lookup, process page-table callbacks, event/IOSB wiring, quotas, IRP allocation, DDT action dispatch, asynchronous completion and AST delivery remain unimplemented.

M222 implementation commit: `86a6be8f5c70f2f041c0bedc1725073f6310cca8`.


## M223 — SYS$QIO direct/buffered I/O quota charge, 23 November 2026

Extended the source-ordered begin helper through the SYSQIOREQ.MAR 300-312 quota step. It charges the process's direct or buffered I/O counter according to the DQ IOTYPE mask; a negative post-decrement is accepted only through a single-unit quota callback, while denial leaves the counter unchanged and posts `SS$_EXQUOTA`. IOSB initialization precedes this quota check as in the source. The callback does not implement resource waiting, and this helper still does not allocate an IRP, alter IPL, dispatch a DDT action, or run as a live SYS$QIO entry.

Verification at implementation commit `4595e62db48bf91194d001b2f2f5e31eac853b7c`: `make sysqioargscheck` PASS, including denied quota rollback, permitted negative counter charge, EFN post, and already-cleared IOSB; `make -j2 all sysqioargscheck` PASS (guest target was already current); full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,968 ranges, zero unmapped; 441/4,566 original files indexed; 28,764 partial, 50,437 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: this host-tested pre-dispatch/quota helper is not linked into the guest image.

SYSQIOREQ.MAR lines 300-312 are now partial coverage. Source IPL/deletion protection, quota wait semantics, exact VMS error path and status mapping, IRP allocation/build, device privilege/protection checks, FDT action invocation, DQA process I/O and AST completion remain unimplemented. All end-to-end acceptance conditions remain incomplete.

M223 implementation commit: `4595e62db48bf91194d001b2f2f5e31eac853b7c`.


## M224 — SYS$QIO failure status values, 23 November 2026

Separated internal helper result enums from service status values for the covered SYSQIOREQ failures. Invalid AP shape returns BADPARAM; invalid channel posts/returns IVCHAN; denied channel access maps to NOPRIV; IOSB write failure maps to ACCVIO; quota denial maps to EXQUOTA. The event-post callback receives those condition values. Exact ILLIOFUNC and DEVOFFLINE status mappings are still unresolved, and the begin helper remains disconnected from a real service entry and process/device tables.

Verification at implementation commit `130bb9476ba527c1b086133316fdbf4f8dd616fe`: `make sysqioargscheck` PASS, including IVCHAN/NOPRIV event posting and quota error status; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (1,968 ranges, zero unmapped; 441/4,566 original files indexed; 28,764 partial, 50,437 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: host-only helper is not linked into the guest image.

Remaining SYSQIOREQ work includes exact ILLIOFUNC/DEVOFFLINE statuses, EXE$SNGLEQUOTA wait semantics, IPL/deletion protection, IRP allocation and construction, device privilege/protection, FDT execution, actual DQA0 process I/O and completion/AST delivery. All end-to-end acceptance conditions remain incomplete.

M224 implementation commit: `130bb9476ba527c1b086133316fdbf4f8dd616fe`.


## M225 — SYS$QIO ILLIOFUNC and DEVOFFLINE status mapping, 23 November 2026

Completed the SYSQIOREQ FDT/offline error status mapping using the VMS 3.7 SYSTEM/FATAL assignments in VMSLIB/SRC/SSMSG.MSG V03-003: SS$_ILLIOFUNC=244 and SS$_DEVOFFLINE=132. Host regressions verify both values are returned and posted through the event callback, with no quota charge. This closes the covered early-preflight status set; it does not implement the remaining SYSQIOREQ service path or connect these values to live processes.

Verification: `make sysqioargscheck` PASS; `python3 tools/gen_source_coverage.py` PASS (1,968 ranges, zero unmapped); `python3 tools/validate_source_ranges.py` PASS (441/4,566 original files indexed; 28,764 partial, 50,437 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `make -j2 all` PASS; full `make test` PASS (exit 0, suite reached M49); `git diff --check` PASS. No QEMU run: this behavior remains in a host-only helper not linked into the guest.

SYSQIOREQ.MAR still lacks public entry integration, real CCB lookup, EXE$SNGLEQUOTA wait/commit semantics, IPL/deletion protection, IRP allocation and construction, device privilege/protection, FDT execution, DQA process I/O and completion/AST delivery. All end-to-end acceptance conditions remain incomplete.

M225 implementation commit: `1607b2b76b78e570c425e22350c6179b803eac5c`.

## M226 — Scheduler suspend dispatch, 23 November 2026

Added `port_exe_sched_suspend` for the original executive path where a current process blocks or is deleted: it saves the live continuation while the PCB is CURRENT, marks it OTHER without putting it on a ready queue, clears scheduler ownership, then selects the highest-priority already-ready process through the existing source scheduler. Host coverage verifies the suspended PCB is not requeued, a ready higher-priority process is loaded, and invalid no-current calls are rejected. This closes a scheduler primitive needed by real wait/wake integration; `arch.c` still uses its diagnostic two-task XOR scheduler, and this helper is not yet bound to live VMS PCBs, event wakeups, IRQ rescheduling, or process activation.

Verification: `make exeportcorecheck` PASS; full `make test` PASS (exit 0, suite through M49; native image rebuilt and installed as part of test dependencies); `python3 tools/validate_source_ranges.py` PASS (441/4,566 originals indexed; 28,764 partial, 50,437 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,065 mapped); `git diff --check` PASS. No QEMU run: the modified helper remains outside the live guest scheduler. No restricted DEC source or disk image was staged.

Source coverage counts unchanged. Live scheduler binding, process wake queues, actual public SYS$QIO and DQA process completion, Files-11 ACP, native INIT continuation, process image activation, SYSTEM login/DCL, DECnet and SET HOST 0 remain incomplete.

M226 implementation commit: `94d38f08320b81799b846c9d5c0acdbaebd5306b`.

## M227 — IA-32 diagnostic context switch uses executive scheduler, 23 November 2026

Bounded the source `port_exe_sched` priority/FIFO engine into the actual kernel diagnostic task context-switch path. CPL3 yield, PIT preemption, block/wait, exit, and ready-task selection now save/load the IA-32 trap frame through scheduler callbacks; selection activates the chosen PCB, updates its TSS kernel stack and private CR3, and applies VMS time-sharing priority float. Event and mutex wake paths synchronize newly READY diagnostic PCBs into the executive priority queues. This replaces XOR alternation for these two diagnostic tasks; task priorities are the test fixture value 16, the adapter still has a fixed two-task table, and it is not the resident VMS PCB database, production wait queues, normal executive reschedule interrupt, or process activation. Accordingly `SCHED.MAR` source coverage is now `partial`, not full. Totals: 28,813 partial, 50,437 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped; 441/4,566 original source files indexed.

Verification: `make -j2 all` PASS; `make exeportcorecheck` PASS; full `make test` PASS (exit 0, through M49); scheduler and preemption linked-layout checks PASS; `python3 tools/validate_source_ranges.py` PASS; `python3 tools/checkpoint_serial_logs.py verify` PASS (80 raw logs); `git diff --check` PASS. Two real `make qemu-sysboot-m217` runs PASS and both preserve the expected explicit incomplete-INIT stop; COM1 transcript `logs/serial/qemu-m217-run-local-20261123T152405875810Z.log`, 5,170 bytes, SHA-256 `c56c3238f189713bdb2dd893cd2f85bc3aff5bd25cfc1d06e2000019709d79a9`. The SYSBOOT runner terminated QEMU. The independent interactive `tools/qemu_smoke.py --timeout 300` did not reach its legacy `PORTLAB>` prompt: captured output reaches the known incomplete-INIT stop. Thus no interactive scheduler command or full login was observed; this is not recorded as a scheduler runtime pass.

M227 does not finish public SYS$QIO, DQA0 process completion, Files-11 ACP, native INIT, production scheduler/process activation, SYSTEM login, DCL or DECnet. Continue by binding the executive process database and process-aware completion without weakening the source-order SYSQIO and privilege semantics.

M227 implementation commit: `7101fe6a4d2965d08a71914e1b6648b5e6298c65`.

## M228 — Source-layout SYS$QIO IRP construction, 23 November 2026

Added a host-tested builder for SYSQIOREQ.MAR SUCCES's device-independent IRP fields. It validates the prepared request and caller-supplied reserved packet, writes PID/access mode/AST and AST parameter/window/UCB/function/EFN/priority/IOSB/channel/status, clears the translation/transfer fields, and stores ARB. Inspection of SHRLIB/SYSDEF.MDL's CDRP negative IRP fields plus positive fork fields established the standard IRP length as 136 bytes through CDRP$L_LENGTH; SYSBOOT rounds that to a 144-byte packet stride. Resident boot packet sizing and host queue fixtures now use the corrected size/stride. This helper still does not allocate a packet, account a live channel, enforce source IPL/deletion and all device checks, call the public service, dispatch the FDT, or complete a process AST. SYSQIOREQ source lines 386-429 are partial only.

Verification: `make clean && make -j2 all && make test` PASS (suite through M49); `make sysqioargscheck qiopostcheck nlddtcheck dqddtcheck dqbootreadcheck dqphysicalcheck` PASS; `python3 tools/validate_source_ranges.py` PASS (1,970 ranges, 0 missing mapped functions; 441/4,566 original files indexed; 28,857 partial, 50,393 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped); `python3 tools/checkpoint_serial_logs.py verify` PASS (83 raw logs). Real `make qemu-sysboot-m217` PASS: the guest read 700 real Files-11 SYS.EXE bytes across two sectors via IOC-selected DQA0 DDT and IRQ14, then stopped at the known incomplete-INIT boundary. No login/DCL/DECnet success is claimed; the runner terminated. Two earlier incremental-build QEMU attempts stopped at the DQ read boundary; a complete clean build passed. Failed raw logs are 1,441 bytes each with SHA-256 `d3a605a12a27a5822c0612091bcc272938355fd538971eb926d59f3af2526f4f`; successful `logs/serial/qemu-m217-run-local-20261123T154547671352Z.log` is 5,170 bytes, SHA-256 `c56c3238f189713bdb2dd893cd2f85bc3aff5bd25cfc1d06e2000019709d79a9`.

M228 implementation commit: `a8faddf181e73e7e3ffae15cf8956256c0d6ed12`.

## M229 — Incremental IRP ABI dependency tracking, 23 November 2026

The M228 incremental QEMU failures exposed missing explicit Make dependencies for `vms_qio_post.h`, which carries the resident IRP length. Added that header dependency to kernel, resident SYS.EXE, QIO, NLA DDT, IOC post, DQ boot-read, and DQ DDT object groups. Also fixed missing `vms_dq_ioc_write.c` linkage in the DQ boot-read and physical-read host targets. Touching the IRP header rebuilt 42 dependent objects, including kernel and resident SYS.EXE.

Verification: header-triggered `make -j2 all` PASS; subsequent full `make test` PASS (exit 0, through M49); the six targeted SYSQIO/QIO/DDT/DQ host checks PASS with the 136-byte ABI; source-range validation PASS with the M228 counts above; QEMU serial manifest PASS (83 raw logs). The clean-build QEMU pass is recorded under M228. No new QEMU process was started for this Make-only change. No original DEC source or binary image was staged.

M228 implementation commit: `a8faddf181e73e7e3ffae15cf8956256c0d6ed12`.
M229 implementation commit: `9009e3302dbd13e05c8251bc10ad564b30e5458b`.

## M230 — SYS$QIO WRITEVBLK device-state normalization, 23 November 2026

Ported SYSQIOREQ.MAR ACCESS lines 450-479 for the source boot-time WRITEVBLK cases. The host-tested helper decodes the original DEV characteristics bits, preserves function modifiers, converts non-FOD and mounted-foreign no-window writes to WRITELBLK, rejects an unmounted FOD write with SS$_NOPRIV, preserves mounted structured/windowed requests, and reports the source AST-quota branch predicate. The helper is not wired to live UCB/PCB state; ASTCNT enforcement, device privileges/protection, FDT dispatch, public service entry and completion remain unresolved. SYSQIOREQ coverage for 450-479 is partial.

Verification: `make sysqioargscheck` PASS with modifier preservation, FOD/MNT/FOR/window states, NOPRIV and AST predicate cases; full `make test` PASS (exit 0, through M49); `python3 tools/validate_source_ranges.py` PASS (1,972 ranges, zero missing mapped functions; 441/4,566 originals indexed; 28,887 partial, 50,363 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped); `git diff --check` PASS. No QEMU run: helper is host-only and not linked into the guest. No original DEC source or binary image was staged.

M230 implementation commit: `75cbb7e938f715643e2b0d2dda448a041875ef94`.

## M231 — SYS$QIO AST quota charge and check, 23 November 2026

Extended source SYSQIOREQ IRP construction to set the ACB quota bit and decrement the process AST count only when an AST routine is specified. Added the ACCESS quota check: a negative count is SS$_EXQUOTA only for AST requests; no-AST requests continue despite a negative AST count. Host tests cover successful build charge, AST quota failure, and the no-AST exception. This remains a supplied process-record model: it is not the live PCB, does not return the allocated IRP or channel IOC count through EXE$ABORTIO, and does not post the event/AST.

Verification: `make sysqioargscheck` PASS; full `make test` PASS (exit 0, through M49); source range generation/validation PASS (1,973 ranges, 0 missing mapped functions; 441/4,566 originals indexed; counts unchanged from M230 because these lines were already partial); `git diff --check` PASS. No QEMU run: host-only helper, not linked into the guest. No original DEC source or binary image was staged.

M231 implementation commit: `181350020fde46e2c10a879efb443bc7ffa8cfee`.

## M232 — SYS$QIO privilege and device-characteristic gates, 23 November 2026

Ported the SYSQIOREQ.MAR ACCESS decision subset for virtual, logical and physical I/O. The helper applies PHY_IO and LOG_IO privilege bypasses, sets the physical-I/O result, rejects unprivileged physical requests, enforces SPL/FOD/MNT/FOR/SHR device gates, and selects the source physical or logical volume checker with VPROT/owner UIC/ARB inputs. A supplied callback decides the actual volume access; no real VMS privilege mask, UCB/PCB, EXE$CHKPHYACCES/CHKLOGACCES implementation, or abort/IOSB/event wiring is connected. Diagnostic-buffer handling remains unresolved. Coverage for SYSQIOREQ lines 515-535, 545-547 and 570-581 is partial.

Verification: `make sysqioargscheck` PASS across privilege bypasses, physical/logical checker selection, denied ACL, spooled, unmounted/structured FOD, virtual I/O, and physical flag; full `make test` PASS (exit 0, through M49); coverage generation/validation PASS (1,979 ranges, zero missing mapped functions; 441/4,566 originals indexed; 28,923 partial, 50,327 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped); `git diff --check` PASS. No QEMU run: host-only code is not linked to the guest. No original DEC source or binary image was staged.

M232 implementation commit: `0c864cbd7df22e2abf6347a623208396bae2dc3a`.

## M233 — SYS$QIO ABORTIO IOSB and AST-quota return, 23 November 2026

Ported SYSQIOREQ.MAR EXE$ABORTIO lines 736-740 into a bounded native completion routine. It validates an allocated standard IRP and IOC post queue, clears IRP$L_IOSB, clears ACB$V_QUOTA and restores the AST count exactly once when that reservation bit was set, appends the IRP to the IOC post FIFO, and requests IPL$_IOPOST. Host cases cover the quota-bit and no-quota paths, signed 16-bit AST-count wrap, IOSB clearing, queue insertion, and software-interrupt request. It still accepts an AST-count pointer rather than resolving the live PCB, and is not wired to public SYS$QIO, source abort/error status posting, IRP return, or process AST delivery.

Verification at implementation commit `cca296792e53e826fd302c6765a7081c12901f88`: `make -j2 all` PASS; `make qiopostcheck` PASS; full `make test` PASS (exit 0, suite reached M49); coverage generation and `python3 tools/validate_source_ranges.py` PASS (1,979 ranges, zero mapped-function misses; 441/4,566 original files indexed; 28,928 partial, 50,322 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped); `git diff --check` PASS. No QEMU run: this host-tested abort path is not connected to a live SYS$QIO or DQA process path. No original DEC source or binary image was staged.

SYSQIOREQ.MAR lines 736-740 are now partial. Public service entry, live PCB/CCB lookup, source event and IOSB postprocessing, IRP allocation/return, driver/ACP completion, process AST delivery and channel accounting remain incomplete. The existing real QEMU boot still stops at incomplete INIT; DECnet, SYSTEM login, DCL, SET HOST 0, second login and logout remain unverified.

M233 implementation commit: `cca296792e53e826fd302c6765a7081c12901f88`.

## M234 — SYS$QIO IRP lookaside allocate and return, 23 November 2026

Ported the IRP-sized `EXE$ALONONPAGED` lookaside dequeue and `EXE$DEANONPAGED` return behavior from MEMORYALC.MAR. The allocator rounds requests to 16-byte packet granularity, removes the oldest validated IRP from the resident FIFO, sets its source size/type fields, and restores the caller's IPL after synchronized queue access. Return inserts the packet at the source FIFO tail. Duplicate returns and malformed/corrupt links are rejected. The finite implementation reports an empty lookaside; VMS variable-pool allocation, extension, resource waits, and AST-delivery semantics are not implemented. These routines are linked into SYS.EXE but are not yet called by public SYS$QIO.

Verification at implementation commit `423b173d0c68266aa42c70edf8ce14158b704c1e`: `make -j2 all irpalloccheck qiopostcheck` PASS; full `make test` PASS (exit 0, suite reached M49); coverage generation and `python3 tools/validate_source_ranges.py` PASS (1,986 ranges, zero mapped-function misses; 442/4,566 original files indexed; 29,032 partial, 51,137 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped); `python3 tools/checkpoint_serial_logs.py verify` PASS (85 raw logs). Real `make qemu-sysboot-m217` PASS: resident SYS.EXE verified the 700-byte Files-11 read across two published DQA0 DDT/IRQ14 sectors and the QEMU scratch write/read/restore; boot then stopped at the existing incomplete-INIT boundary. Raw COM1 log `logs/serial/qemu-m217-run-local-20261123T161833629743Z.log`, 5,170 bytes, SHA-256 `c56c3238f189713bdb2dd893cd2f85bc3aff5bd25cfc1d06e2000019709d79a9`. Runner exited and terminated QEMU. Raw CRLF serial logs are preserved as evidence; source/documentation `git diff --check` passed (raw serial logs naturally contain CR bytes).

MEMORYALC.MAR IRP lookaside portions (lines 215-240, 350-397, 516-545) are now partial. Dynamic nonpaged allocation and waits, live CCB/PCB/channel accounting, public SYS$QIO dispatch, DQA process I/O, Files-11 ACP, process AST completion, native INIT continuation, login/DCL and DECnet remain unfinished. No restricted DEC source or disk image was staged. All end-to-end acceptance conditions remain incomplete.

M234 implementation commit: `423b173d0c68266aa42c70edf8ce14158b704c1e`.

## M235 — Resident DQA0 boot reads allocate real SYS.EXE IRPs, 23 November 2026

Replaced the static scratch IRP in the resident SYS.EXE DQA0 physical-read path with allocation from INIT's real resident IRP lookaside queue. Both the prepublication DQA0 CRB read and the published DDT Files-11 chain-sector callback now allocate a source-sized IRP, build the physical READPBLK fields, call DQ DDT STARTIO/IRQ14, and return the same IRP to the FIFO tail. The guest checks that each operation restores the lookaside free count. On IA-32, the queue critical section preserves EFLAGS.IF and uses CLI/STI; host checks use a nonprivileged synchronization fixture. The QEMU runner now requires the guest's explicit allocation/completion/return line. This is a resident boot test of the allocator/DDT connection; public process SYS$QIO, CCB accounting, user IOSB/event/AST delivery, variable-pool fallback and resource waits are not implemented.

Verification at implementation commit `0355f6a30455c4831ea1aa9fa7569eae23565a69`: `make -j2 all` PASS; full `make test` PASS (exit 0, through M49); `make sysexecentrycheck irpalloccheck qiopostcheck` PASS; coverage generation and source-range validation PASS (1,986 ranges, zero mapped-function misses; 442/4,566 original files indexed; 29,032 partial, 51,137 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded, 17,016 mapped); serial manifest verification PASS (88 raw logs). Real `make qemu-sysboot-m217` PASS. The serial capture includes `INIT: SYS.EXE allocated, completed and returned DQA0 IRP through resident source lookaside.`, verifies the 700-byte Files-11 chain across two IOC-selected DQA0 DDT sectors via IRQ14, and records the existing incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261123T162635266763Z.log`, 5,264 bytes, SHA-256 `09541767cf4417c0a4d385da088608f87e30b726cd2e97a39f5d399eecf7685e`. QEMU runner exited and terminated its guest. No restricted DEC source or disk image was staged.

MEMORYALC.MAR lookaside allocation/return now has actual resident boot-path use, but remains partial: public SYS$QIO, dynamic nonpaged pool fallback/wait, DQA process requests and Files-11 ACP, process-aware AST/scheduler completion, INIT process activation, SYSTEM login/DCL, DECnet and SET HOST 0 remain unfinished. All end-to-end acceptance conditions remain incomplete.

M235 implementation commit: `0355f6a30455c4831ea1aa9fa7569eae23565a69`.

## M236 — SYS$QIO IRP reservation stage, 23 November 2026

Added a source-ordered SYS$QIO reservation helper that removes a real IRP-sized packet from the INIT lookaside, checks AST quota after allocation, writes the standard 136-byte IRP using the actual packet VA, and returns packets to the same pool on AST rejection or malformed IRP construction. Pool exhaustion maps to SS$_INSFMEM. Host regression covers successful reservation and ASTCNT charge, AST-quota failure with packet return, and malformed construction with packet return. This is a tested QIO transaction stage; it is not called by a VMS system service, does not account the channel IOC, and does not dispatch DQA0 FDT or deliver process completion.

Verification: `make sysqioargscheck irpalloccheck` PASS; full `make test sourcerangecheck sysexecentrycheck qiopostcheck nlddtcheck` PASS (exit 0); `make -j2 all` PASS. Source-range validation reports 1,990 ranges, zero mapped-function misses, 442/4,566 original files indexed, 29,046 partial lines, 51,123 unresolved, 7,799 missing, 26,217 excluded, 917 hardware-excluded and 17,016 mapped. No QEMU test was run for M236 because the isolated reservation helper is not linked into the resident SYS.EXE/QEMU path. No restricted DEC source or disk image was added.

SYSQIOREQ.MAR lines 336-345 and 537-540 now map partially to the tested reservation helper. The service entry, source IPL/deletion exclusion, failure rollback and event completion, CCB IOC accounting, buffered window, device FDT invocation, public SYS$QIO dispatch and process AST completion remain unresolved. No end-to-end acceptance condition is complete.

M236 implementation commit: `dded7a52dfead635df3244d945fef333b123c4ab`.

## M237 — Corrected INIT process-vector coverage statement, 23 November 2026

Source audit found the diagnostic `INI_PSTATE` vector points at `port_exe_sched_process` records, whose offsets do not match SYSDEF.MDL's VMS PCB. Corrected the resident-entry comments and recovery summary to make clear these pointers cannot activate VMS processes. This removes the inaccurate claim that they are native PCB descriptors; it adds no process behavior. The required next process work is source-layout PCB plus PHD construction and activation, after the missing page-table/SPT prerequisites.

Verification: `make sysexecentrycheck` PASS; `make -j2 all` PASS. No QEMU run was needed because runtime behavior did not change; the previous real QEMU capture still stops at incomplete INIT. No restricted DEC source or disk image was added.

M237 implementation commit: `e84fbf482149c93c47e3c47cef3b88ad16341125`.

Post-M236 runtime regression: `make qemu-sysboot-m217` PASS. The guest again completed the 700-byte Files-11 chain read through published DQA0 DDT/IRQ14 and stopped at `INIT stopped: remaining original initialization dependencies incomplete.` Capture `logs/serial/qemu-m217-run-local-20261123T163911638559Z.log`, 5,264 bytes, SHA-256 `09541767cf4417c0a4d385da088608f87e30b726cd2e97a39f5d399eecf7685e`; serial manifest verification PASS (89 raw logs). This is boot regression evidence; the host-only M236 reservation helper is still not on the resident execution path. The QEMU runner terminated before return.

## M238 — SYSDEF-layout SYSTEM PCB template, 23 November 2026

Added a tested 156-byte SYSDEF.MDL PCB initializer for the source `PDAT.MAR GENPCB` record and static `MMG$AL_SYSPCB` template, using source structure offsets checked against the pinned SYSDEF and original SYS.STB symbols. The initializer handles source queue heads, DYN PCB type, priority encoding, state/status, privileges, PID/PHD, inline ARB, UIC, ASCIC name and lock queue. `INIT_SYSPCB` finalization has a tested helper to attach PHD and translated native PHYPCB addresses. Resident SYS.EXE now stages and validates the static source-layout SYSTEM PCB and emits an explicit template/PHD-pending boot line. The separate diagnostic process vector still points at scheduler-model records; no VMS PCB vector switch, PHD allocation, process activation or scheduler integration occurred.

Verification: `make -j2 all sysnativecheck` PASS; full `make test` PASS (exit 0, suite reached M49); `make sysexecentrycheck v37pcbcheck sourcerangecheck` PASS; source-range validation PASS (1,997 ranges, zero mapped-function misses; 443/4,566 original files indexed; 29,129 partial, 51,347 unresolved, 7,799 missing, 26,218 excluded, 917 hardware-excluded, 17,016 mapped); `git diff --check` PASS excluding preserved CRLF serial captures. Real `make qemu-sysboot-m217` PASS: guest validated the source-layout template, completed the existing 700-byte Files-11 chain read through published DQA0 DDT/IRQ14, then stopped at `INIT stopped: remaining original initialization dependencies incomplete.` No login, DCL, DECnet or SET HOST 0 was observed. Runner exited and terminated QEMU. Successful COM1 transcript `logs/serial/qemu-m217-run-local-20261123T165825955888Z.log`, 5,283 bytes, SHA-256 `236553623aa4d141aa426b14e12a72ec4792933f42baed27a2704f364080d121`; a second byte-identical successful transcript is `logs/serial/qemu-m217-run-local-20261123T170046152923Z.log`. Serial manifest verification PASS (93 raw logs). Two earlier actual boot attempts exposed a guest validation bug and a stale runner expectation; both transcripts remain recorded alongside the successful runs.

PDAT.MAR source ranges now map the GENPCB record and static system PCB template as partial; PHD construction, null/swapper records and activation remain unresolved. INIT.MAR `INI_SYSPCB` is partial through a host-tested finalization helper, which is not yet executed against a real SYS PHD. No original DEC source, disk image, recognized secret, or restricted source file was staged.

M238 implementation commit: `f70f15502ed02a9e9f03f6945eace60c723cb4da`.

Next: implement the native PHD/page-table prerequisites and connect a genuine VMS PCB to the process vector; keep the scheduler-model vector explicitly diagnostic until ABI-compatible activation is verified. Public SYS$QIO, process-aware DQA0 completion, Files-11 ACP, AST scheduling, native INIT, login/DCL and DECnet remain incomplete. All end-to-end acceptance conditions remain incomplete.

## M239 — Source-layout process-header initialization, 23 November 2026

Added a native initializer for the fixed 376-byte `SYSDEFMP.MDL` PHD format and the `PDAT.MAR GENPHD` fields. Layout offsets follow the original PHD structure through its embedded hardware-context slots, process page-table descriptors, privilege masks and the dynamic WSL boundary. Callers supply aligned native stack and page-table addresses; the helper does not synthesize VAX address-bias values. Tests cover the initialized fields, zeroed untouched bytes and transactional rejection of malformed alignment/capacity. This helper is host-tested only and is not linked into SYS.EXE or connected to PHD allocation, IA-32 page mapping, PCB vectors or process activation.

Verification: `make v37phdcheck sourcerangecheck` PASS; `make -j2 all` PASS (no guest relink required); full `make test` PASS (exit 0, suite reached M49); source-range validation PASS (1,997 ranges, zero mapped-function misses; 443/4,566 original files indexed; 29,179 partial, 51,297 unresolved, 7,799 missing, 26,218 excluded, 917 hardware-excluded, 17,016 mapped); `git diff --check` PASS. No QEMU run: new helper is host-only. No DEC source, binary image, or recognized secret was staged.

PDAT.MAR lines 171-220 now map partially to `port_v37_phd_init_genphd`; the PHD byte layout and GENPHD field stores are covered, but the IA-32 page-table representation, process header allocation, null/swapper records and activation remain missing.

M239 implementation commit: `fa949a95f830c5507bf132231447183147219147`.

## M240 — INIT process vector points to VMS PCB records, 23 November 2026

Changed the resident `INI_PSTATE` address vector to point at the actual 156-byte SYSDEF-layout SYSTEM and NULL PCB templates instead of scheduler-model records. The diagnostic scheduler descriptors remain separate and are not activated. The guest checks both record layouts, DYN tags, initial sequence/PID words, absent PHD pointers and exact vector entries before continuing. The INIT vector now has VMS PCB ABI pointers, but neither process has an allocated PHD/native context; no VMS process is activated.

Verification: `make -j2 all sysexecentrycheck v37pcbcheck sourcerangecheck` PASS; real `make qemu-sysboot-m217` PASS through the changed guest checks and the existing 700-byte Files-11 DQA0 DDT/IRQ14 test; guest stopped at the incomplete-INIT boundary. No login, DCL, DECnet or SET HOST 0 observed. QEMU exited and terminated. `python3 tools/checkpoint_serial_logs.py verify` PASS (94 raw logs); source-range validation PASS (1,997 ranges, zero mapped-function misses; 443/4,566 originals indexed; 29,179 partial, 51,297 unresolved, 7,799 missing, 26,218 excluded, 917 hardware-excluded, 17,016 mapped); `git diff --check` PASS excluding CRLF serial logs. Capture `logs/serial/qemu-m217-run-local-20261123T171217945755Z.log`, 5,283 bytes, SHA-256 `2f81f677ef43ac153987d060cc21f3bce8e5d7e4b0be5c71337256d54fafa325`.

Coverage counts are unchanged; `INIT.MAR` PSTATE mapping now records that the address vector points to VMS PCB-layout records. PDAT.MAR PHD/context creation, VMS PCB scheduler binding, page-table mapping, public SYS$QIO, Files-11 ACP, process AST completion, login/DCL and DECnet remain incomplete.

M240 implementation commit: `4a9ab425064c59c426a0619b55a0c6bedc84a882`.

## M241 — Native resident COM/COMO queues for NULL and SWAPPER, 23 November 2026

Implemented the resident/nonresident executable queue portion of original `RSE.MAR SCH$CHSE`: 32 priority queues per class, source priority increments and inverted priorities, source PCB links/state, COMQS summary bits, and callback points for swapper wake/reschedule. INIT now sets SYSTEM PIX 32, installs the source NULL/SWAPPER/SYSTEM PCB pointers into the process vector and queues resident NULL and SWAPPER. Guest verifies their sequence PIDs, vector pointers and resident queue summaries. These PCBs still have no PHD/native hardware context and are not runnable; this is not scheduler activation.

Verification: `make -j2 all sysexecentrycheck v37pcbcheck v37schedcheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (2,006 ranges, zero unmapped native functions; 444/4,566 original files indexed; 29,267 partial, 51,944 unresolved, 7,799 missing, 26,218 excluded, 917 hardware-excluded, 17,016 mapped); source audit and `git diff --check` PASS. Real `make qemu-sysboot-m217` PASS: COM1 verified the resident NULL/SWAPPER queues and existing DQA0 DDT/IRQ14 700-byte read, then stopped at the incomplete INIT boundary. No login, DCL, DECnet or SET HOST 0 was observed. QEMU exited. Transcript `logs/serial/qemu-m217-run-local-20261123T172440194753Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`; serial manifest verification PASS (95 raw logs).

PDAT.MAR now maps NULL/SWAPPER GENPCB instances and the process-vector storage/installation as partial. RSE.MAR maps B_PINC/COMQS and SCH$CHSE queue behavior as partial; source IPL enforcement, swapper execution and remaining RSE routines are unresolved. Public process SYS$QIO, Files-11 ACP, process AST completion, PHD/page-table activation, scheduling, login/DCL and DECnet remain incomplete.

M241 implementation commit: `35fecbeeda7bd60409675a7103a2550988055d32`.

## M242 — Resident SCH$SCHED queue selection, 23 November 2026

Added source-order resident COM queue selection to the native VMS scheduler state: choose the first set COMQS priority, remove the FIFO head, validate its source PCB, set CUR/current-PCB/current-priority, and apply source priority floating. Selection refuses to consume a PCB with no PHD or native PHYPCB context. This implements the selection bookkeeping before source LDPCTX; it does not restore an IA-32 register/stack context or run a process. SCHED.MAR coverage now maps these exact source ranges to this routine and leaves context save/restore, idle and empty-queue handling partial or unresolved.

Verification: `make -j2 all sysexecentrycheck v37schedcheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (2,006 ranges, zero mapping errors; 444/4,566 original files indexed; 29,259 partial, 51,944 unresolved, 7,799 missing, 26,226 excluded, 917 hardware-excluded, 17,016 mapped). Host scheduler checks cover contextless queue preservation, priority/FIFO selection, CUR state, floating priority and empty COM queues. Real `make qemu-sysboot-m217` PASS through the existing INIT NULL/SWAPPER queue and DQA0 IRQ14 read checks; the new selection path is not invoked in the guest because the processes still have no native contexts. The guest stops at incomplete INIT; no login, DCL, DECnet or SET HOST 0 observed. QEMU exited. Transcript `logs/serial/qemu-m217-run-local-20261123T223814519904Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`; manifest verification PASS (96 raw logs).

M242 implementation commit: `528adf358646539558896251de04643864715792`.

## M243 — Native SCH$RESCHED COM queue requeue, 23 November 2026

Added the source SCH$RESCHED queue update after an external architecture layer saves the running context: validate the current resident VMS PCB, set COM, append it to its priority queue, and publish the COMQS bit/count. Host regressions exercise FIFO round-robin requeue with SCH$SCHED selection. SCHED.MAR coverage now maps the exact current-PCB queue insertion lines to this routine. The IA-32 interrupt entry, IPL transition, saved-context construction, and eventual LDPCTX/IRET equivalent are still absent; this code does not switch the running process.

Verification: `make -j2 all sysexecentrycheck` PASS; `make v37schedcheck` PASS; full `make test` PASS (exit 0, suite reached M49); `python3 tools/validate_source_ranges.py` PASS (2,009 ranges, zero mapping errors; 444/4,566 original files indexed; 29,246 partial, 51,944 unresolved, 7,799 missing, 26,239 excluded, 917 hardware-excluded, 17,016 mapped). Real `make qemu-sysboot-m217` PASS through existing INIT PCB queue checks and DQA0 DDT/IRQ14 disk read, then stopped at incomplete INIT; no login/DCL/DECnet/SET HOST observed. QEMU exited. Capture `logs/serial/qemu-m217-run-local-20261123T224317238061Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`; serial manifest verification PASS (97 raw logs).

M243 implementation commit: `bb04230f4cf246700a568582886c7d4b9a093024`.

## M244 — Validated native PCB context binding, 23 November 2026

Added a source-PCB helper to bind aligned native PHD and IA-32 processor-context addresses to a valid DYN PCB. SYSTEM PCB finalization uses the helper; scheduler host fixtures now bind context through this ABI helper instead of writing PCB bytes directly. Tests cover successful binding and transactional rejection of an unaligned PHD. Resident NULL/SWAPPER in the live guest still have no PHD or IA-32 context, so SCH$SCHED continues to refuse their dispatch and no context switch is claimed.

Verification: `make v37pcbcheck v37schedcheck`, `make -j2 all sysexecentrycheck`, full `make test`, source-range validation (2,009 ranges, zero mapping errors) and serial manifest verification (98 transcripts) PASS. Real `make qemu-sysboot-m217` PASS through existing resident queue checks and DQA0 IRQ14 Files-11 read, then stopped at incomplete INIT; no login/DCL/DECnet/SET HOST observed. Transcript `logs/serial/qemu-m217-run-local-20261123T224722659349Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`.

M244 implementation commit: `1319780d0d79512865133793fb2db78648134b0d`.

Independent M244 checkpoint rerun, 23 November 2026: `make -j2 all sysexecentrycheck v37pcbcheck v37schedcheck`, full `make test`, source-range validation, and serial-manifest verification PASS at the existing M244 source commit `f0070116068a4b87c9a0290f593e4c7d9ae31779`. Real `make qemu-sysboot-m217` PASS through the existing 700-byte Files-11 read and DQA0 queued I/O checks, then stopped at the incomplete INIT boundary. No SYSTEM login, DCL, DECnet, or SET HOST 0 was observed. QEMU exited; no QEMU runner remains. Raw transcript `logs/serial/qemu-m217-run-local-20261123T225320429666Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`; transcript manifest verification PASS (99 logs). This rerun did not add functional coverage: PHD/native contexts are not created for live NULL/SWAPPER, so process activation and the remaining acceptance conditions are still incomplete.

## M245 — Source-faithful native NULL loop, 23 November 2026

Corrected `port_exe_nullproc` to emit the source `NULLPROC.MAR` unconditional self-branch. The earlier IA-32 `sti; hlt` sequence was interruptible idle behavior absent from the original `BRB 10$` loop. Object disassembly confirms the routine is exactly `eb fe` (`jmp` to itself). The source coverage record now describes that precise mapping. The live VMS process scheduler still does not activate this routine; this change does not claim NULL process runtime dispatch.

Verification: `make -j2 all` PASS; full `make test` PASS (exit 0, suite reached M49); source-range validation PASS (2,009 ranges, zero mapping errors; 444/4,566 original files indexed; 29,246 partial, 51,944 unresolved, 7,799 missing, 26,239 excluded, 917 hardware-excluded, 17,016 mapped); `objdump -dr build/vms_exe_nullproc.o` shows the self-loop; `git diff --check` PASS. Real `make qemu-sysboot-m217` PASS through existing DQA0 Files-11 I/O, then stopped at incomplete INIT. The null process was not dispatched; no login/DCL/DECnet/SET HOST 0 observed. QEMU exited. Transcript `logs/serial/qemu-m217-run-local-20261123T225905085756Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`; serial-manifest verification PASS (100 logs).

M245 implementation commit (published `main`): `348f4b73d30f96982aeb439bd35b9ed390e9e9a6`.

## M246 — Initial native IA-32 process frames, 23 November 2026

Added a kernel-linked constructor for initial IA-32 interrupt-return frames on a process-owned kernel stack. It lays out the segment-save prefix and PUSHAL/IRET register frame in the exact order consumed by `arch_isr_common`, supports native CPL0 and CPL3 starts, checks CR3/selector/stack validity, and rejects IOPL, NT, and VM EFLAGS bits. At M246, VMS executive and supervisor access modes (1 and 2) were rejected pending descriptor semantics; M247 maps all four VMS access modes directly to the matching IA-32 CPL and checks the exact GDT selectors. Host cases verify the frames and invalid privilege/flags/stack/CR3 cases. SCHED.MAR's initial context construction lines are partial coverage. The runtime trap return still restores only its current frame: this constructor is not called by SCH$SCHED and does not activate NULL, SWAPPER, or SYSTEM. No process dispatch or INIT progress is claimed.

Verification: `make -j2 all` PASS and kernel symbol table contains `port_v37_ia32_context_init`; full `make test` PASS (exit 0, suite reached M49); `make v37ia32ctxcheck` PASS; source-range validation PASS (2,009 ranges, zero mapping errors; 444/4,566 original files indexed; 29,248 partial, 51,944 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded, 17,016 mapped); serial-manifest verification PASS (101 logs); `git diff --check` PASS excluding raw CRLF COM1 logs. Real `make qemu-sysboot-m217` PASS through existing DQA0/Files-11 tests, then stopped at incomplete INIT. The new frame constructor was not invoked in QEMU. No login, DCL, DECnet, or SET HOST 0 was observed. Transcript `logs/serial/qemu-m217-run-local-20261123T231143756841Z.log`, 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`.

M246 implementation commit (published `main`): `60f209d06b83e9748e9bfe729dd70ad7c0c2a382`.

## M247 — VMS access modes mapped to IA-32 rings, 23 November 2026

Extended the initial IA-32 IRET frame constructor to represent VMS kernel, executive, supervisor and user access modes as CPL0, CPL1, CPL2 and CPL3. It requires the matching code/data descriptors in the native GDT (`08/10`, `19/21`, `2a/32`, `3b/43`), preventing a selector with a plausible RPL but the wrong descriptor privilege. Nonzero modes require the caller to supply already mapped initial stack and page tables. This makes the frame format capable of preserving each VMS access mode; it does not create those address spaces or activate a PCB.

Verification: `make v37ia32ctxcheck` PASS for all four modes and a wrong-CPL1-descriptor rejection; `make -j2 all` PASS; full `make test sourcerangecheck sysexecentrycheck v37pcbcheck v37schedcheck` PASS (exit 0; suite reached the resident SYS.EXE link/fail-closed INIT checks); `python3 tools/validate_source_ranges.py` PASS (2,009 ranges, zero mapping errors; 444/4,566 original files indexed; 29,248 partial, 51,944 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded, 17,016 mapped); real `make qemu-sysboot-m217` PASS through the 700-byte Files-11 SYS.EXE read through IOC-selected DQA0 DDT and IRQ14, then the expected incomplete-INIT halt. No process was activated; no login, DCL, DECnet or SET HOST 0 was observed. The QEMU runner exited. Captures `logs/serial/qemu-m217-run-local-20261123T231957006092Z.log` and `logs/serial/qemu-m217-run-local-20261123T232014586790Z.log` are each 5,281 bytes, SHA-256 `498d256405c3824e1d81ad5d2fe63f8a0b6184dccce4551907ecf83594034bca`; manifest records both real runs.

Coverage totals are descriptive inventory only. Scheduler dispatch/context restore, VMS PHD/page tables, process activation, public SYS$QIO, process AST completion, ACP, native INIT, login/DCL and DECnet remain incomplete.

M247 implementation commit (local): `f998e536951e9c785c1a8c7c944b9e3d7f658a85`; published `main`: `ae600ee7aa3e13d9b854480a9f5774352b6d0974`.

## M248 — QEMU CPL1/CPL2 frame and interrupt-return exercise, 23 November 2026

Connected the M246/M247 frame constructor to an IA-32 restore path and exercised actual CPL1 and CPL2 entries during native boot. Each frame enters a dedicated ring probe, executes `INT 0x80` through the installed IDT/TSS privilege stack, validates the observed CPL and code selector in `arch_trap`, then returns to CPL0 while restoring the original kernel stack, callee-saved registers and EFLAGS. The M217 QEMU harness now requires the serial confirmation before accepting the subsequent SYSBOOT/INIT sequence. This verifies IA-32 ring transitions and frame restoration; it is an architecture regression probe, not VMS scheduler dispatch or process activation.

Verification: `make -j2 all` PASS; full `make test sourcerangecheck sysexecentrycheck v37ia32ctxcheck` PASS (exit 0); `python3 tools/gen_source_coverage.py` PASS (2,009 ranges, zero missing mapped functions); `python3 tools/validate_source_ranges.py` PASS (444/4,566 original files indexed; 29,248 partial, 51,944 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded, 17,016 mapped); serial manifest verification PASS (107 raw logs). Real `make qemu-sysboot-m217` PASS with the new CPL1/CPL2 assertion, then verified the existing 700-byte Files-11 SYS.EXE chain read through IOC-selected DQA0 DDT/IRQ14 and stopped at the expected incomplete-INIT boundary. Two failed QEMU attempts were retained: the first exposed lost callee-saved registers in the new restore trampoline; the second exposed that it did not restore the caller's IF state. Both were corrected before the passing runs. Failure captures `logs/serial/qemu-m217-run-local-20261123T232841162069Z.log` (68 bytes, SHA-256 `29eb87b6df946d3a5482a16adc6f2db56bdb6889d5c756943d610e476a592b0f`) and `logs/serial/qemu-m217-run-local-20261123T233021678659Z.log` (139 bytes, SHA-256 `0eec2b91092d3010934f6a0d6259d10f594dbce8b5c50a1632d15c88e33e983d`). Passing captures `logs/serial/qemu-m217-run-local-20261123T233037282235Z.log` and `logs/serial/qemu-m217-run-local-20261123T233219424955Z.log` (each 5,361 bytes, SHA-256 `8aa380a9328287c76d6ef565eeb132f1b47103bbfd3c973efb0226e3f58fa353`). QEMU runners exited.

No original coverage status totals changed; SCHED.MAR initial-context lines remain partial because runtime VMS scheduler dispatch, current-context save, VMS PHD/page-table setup and live PCB activation are still unimplemented. Public SYS$QIO, process-aware DQA0 completion/AST delivery, ACP, native INIT completion, login/DCL and DECnet/SET HOST 0 remain incomplete.

M248 implementation commit (local): `37c40e2b7675c10bf50ff1074bc4621ba229d4b5`; published `main`: `33efcf0eafe36c2c81c4c3acfcfe20a9cc305674`.

## M249 — Trap-selected IA-32 frame dispatch, 23 November 2026

Extended `arch_isr_common` with a selected-frame return path for `arch_trap`. During the boot regression, a CPL1 software interrupt selects the already initialized CPL2 frame; common interrupt return restores that frame and enters CPL2, whose interrupt returns to the saved kernel caller. The QEMU harness requires the CPL1-to-CPL2 dispatch marker. This exercises the native hardware-frame selection step needed by future SCH$SCHED integration. The probe does not save a live VMS PCB context, switch a process CR3/TSS stack, or invoke resident SCH$SCHED, so no VMS scheduling or process activation is claimed.

Verification: `make -j2 all` PASS; full `make test sysexecentrycheck v37ia32ctxcheck` PASS (exit 0); source coverage generation and validation PASS (2,009 ranges, zero mapping errors; 444/4,566 original files indexed; 29,248 partial, 51,944 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded, 17,016 mapped); serial manifest verification PASS (108 raw logs). Real `make qemu-sysboot-m217` PASS with the selected-frame assertion, the existing DQA0/IRQ14 700-byte Files-11 read, and the expected incomplete-INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261123T233620657447Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`; QEMU exited.

Public SYS$QIO, process-aware DQA0 completion/AST delivery, ACP, native INIT completion, login/DCL, DECnet and SET HOST 0 remain incomplete.

M249 implementation commit (local): `186275478935110cf2875c05a86771cfa42b5ee9`; published `main`: `4ec4a4ac789d48a9f3b8fad7f40dd52cacac2004`.

## M250 — SYSQIO begin-to-IRP reservation transaction, 23 November 2026

Implemented `port_v37_sysqio_prepare_irp` to join the SYSQIO argument/preflight/IOSB/I/O quota stage with standard-IRP construction and AST quota reservation. A reservation failure restores the charged direct or buffered process I/O count and posts the returned service status to the requested event flag. Success publishes the prepared request and packet only after reservation completes. This remains a preparation boundary: CCB IOC accounting, device access/driver FDT dispatch, DQA0 submission, completion IOSB, process-aware AST delivery, and public live SYS$QIO dispatch remain incomplete.

Coverage records SYSQIOREQ.MAR lines 313–335 as partial. Source coverage validation: 2,009 ranges, zero missing mapped functions; SYSQIOREQ coverage remains partial/unresolved as recorded in `source-coverage.json`.

Verification: `make sysqioargscheck` passed with success and IRP-construction-failure rollback cases. `python3 tools/validate_source_ranges.py` passed (444/4,566 original source files indexed). `make -j2 all` passed (no rebuild needed). `make test sysexecentrycheck v37ia32ctxcheck` passed, including DQ private IOC and SYSQIO host tests. No QEMU run was required for this host-only transaction change; the last real IA-32 QEMU verification remains M249, which reaches the documented incomplete INIT stop.

Remaining: connect this transaction to live process AP/PCB/channel state and implement channel IOC, source error rollback, DQA0 FDT/IRQ completion, public SYS$QIO ABI dispatch, Files-11 ACP, AST completion, scheduler and image activation. Acceptance boot/login/DECnet/SET HOST 0 sequence is not achieved.

M250 implementation commit (local): `119d0a69795aa45d2284a0f3fe8aca11898fc0a1`. The code and coverage were published to GitHub `main` through contents commits ending at `e0c9b101b82d3a79d6dc01bba79b5bf62a910683`; resume publication commit: `511f8d5110465ba6e5c8a26bdcf0e954972683e1`. Native Git push remains unavailable because this environment has no HTTPS Git credentials.

## M251 — SYSQIO CCB pending wait boundary, 23 November 2026

Added the source CCB$L_WIND pending-bit guard to SYSQIO begin. When access/deaccess is pending, preparation now refuses to clear IOSB, charge I/O quota, allocate an IRP, or post a false service error until a scheduler wait callback returns with the CCB pending bit cleared. The no-callback result is a distinct internal wait condition propagated by the IRP preparation wrapper. Host tests cover both fail-closed/no-side-effect behavior and resumed preparation after the wait callback. This models the required `RSN$_ASTWAIT` synchronization boundary, but no live SCH$WAIT callback is wired to a process scheduler, so public SYS$QIO remains unavailable.

Coverage generator now records SYSQIOREQ.MAR lines 149–156 as partial and lines 313–335 as partial. Verification: `make sysqioargscheck` passed; full `make test sysexecentrycheck v37ia32ctxcheck` passed; `make -j2 all` passed (no rebuild required); source coverage validation passed with 2,011 ranges, 444/4,566 original files indexed, zero missing mapped functions, 29,279 partial and 51,913 unresolved lines. This host-only change was not given a new QEMU run; last real IA-32 QEMU boot remains the M249 run and still halts at incomplete INIT.

Remaining: connect the wait callback to a real VMS scheduler, bind CCB/channel lookup to the process, perform source-ordered FDT/device access and DQA0 STARTIO, and complete IRP/IOSB/event/AST posting. Files-11 ACP, INIT completion, image activation, SYSTEM login, DECnet Phase IV and nested SET HOST 0 acceptance remain unachieved.

M251 implementation commit (local): `865e0b3e864259d6e578f9900c7ab0bc56097856`.
GitHub `main` publication commits for the M251 source paths: `d3460742b59168904ac0fd282962a9885a9952ee`, `290124540c75fb3800708186f35a6b774463331f`, `b912a85b17748abb667ee0700714f2e41f4d459e`, `7c0becb6de9bacd24c4763c4bef01b30be52f9d7`, `3ead951cb8ae67dc6f729ac186b4da8a351089a2`; coverage generator repair `2f13550811cb98a32bfe6dae14768598d5af565d`; exact generated coverage blob commit `1f9b59625d615fe9d352d45eb160daa85d519723`; resume publication `e781e4f64cbaaacca0a7fd86f4bbfd88c834a392`.

## M252 — SYSQIO bound to the live VMS PCB ABI, 23 November 2026

Added a validated binding from SYSQIO's process state to the original SYSDEF.MDL PCB layout. The native code now reads and updates the PCB's DIOCNT, BIOCNT, ASTCNT, EFCS and EFCU fields at their source offsets, preserving the existing fixture path for isolated host tests. The host regression uses a byte-layout PCB fixture to verify the actual quota, AST and event-flag words change. SYSQIO modules are now linked into resident SYS.EXE; an ELF32 symbol check confirms begin, IRP preparation and PCB bind entry points are present in the image. This is resident code availability only: no public SYS dispatcher invokes it, and no live channel/CCB lookup, driver completion, process AST delivery, or scheduler wait is connected.

Verification: make -j2 all PASS; make -j2 test sysnativecheck sysqioresidentcheck sysexecentrycheck sysqioargscheck v37ia32ctxcheck PASS (full host suite exit 0); source-range validation PASS (2,013 ranges; 444/4,566 original source files indexed; zero mapping errors; 17,016 mapped, 29,364 partial, 51,828 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded); git diff --check PASS. Real make qemu-sysboot-m217 PASS for the existing boot and DQA0 Files-11 tests, then halted at the expected incomplete INIT boundary. Transcript logs/serial/qemu-m217-run-local-20261124T001208749404Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c. No SYSTEM login, DCL, DECnet Phase IV or SET HOST 0 occurred.

Remaining: wire SYSQIO to live process/channel state, DQA0 completion and AST posting; implement Files-11 ACP and native INIT continuation; connect scheduler and process image activation; then verify the required nested SYSTEM/DCL SET HOST 0 interaction. Overall acceptance remains incomplete.

M252 implementation commit (local): 91e3873bccd21b76a3575a2be8dca8d96e01aa6d.

M252 GitHub main contents publication commits: implementation files 5f408d00ee20dedf7ea32cd4f21670ca91289aef, ad35e64e9ff5eb5ec5bf24bde42237ccf023b522, dc33ca799ef85a33b06a14d617765a42a489a9df, a1b9aea3fd30da62a0341f0de32236556b8e2cdf, 3288180b113e5a55bf166dcbe1dc4e4d179ab1df, 149bdb01f0a45fe0ec1da78dcc3da6ed87f51b5e, cbb90c093e0f1af2d3d964852f71f49b3c14b171, 3f70d75d47eaf859582edd054ab881df661b5b45, 705c4cfaf85eaa363d1395ed98cbcdb257bf5836, e7367dc80a00b90e44d625d34ea394bf9d89dbc7, 38ca8792e2ed9a19856d186c3e24d7e737652d5e, cc8bcf3670c869fe4658f9bbb123af684823ad14, 6229f1d129624b0c8b79ce1c22caf63ed093ad94, coverage e3f3276b8a0a5cf1e590a7e4099962279a0cc54a; resume publication c6e63cef642651593611576aa164bc5c7fd28b68.

## M253 — Source-format VMS CCB channel resolution, 23 November 2026

Added the four-longword source CCB layout and SYSQIOREQ VCHAN resolver. It accepts the original byte-offset channel token, rejects zero/malformed/out-of-range channels, computes CCB as CTL - channel_offset, applies the source AMOD comparison, and returns the source UCB, WIND/pending bit, status and IOC fields. Host cases cover the backward index and the channel/access edge cases. The resolver is linked into resident SYS.EXE and symbol-checked. Its input CCB base still must be supplied by a live per-process channel table; SYS and the VMS user service dispatcher are not connected, so no public process $QIO works yet.

Verification: make -j2 all sysqioresidentcheck v37ccbcheck sysexecentrycheck PASS; full make -j2 test PASS (exit 0); python3 tools/gen_source_coverage.py and python3 tools/validate_source_ranges.py PASS (2,013 ranges, 0 mapping errors; 444/4,566 originals indexed; 17,016 mapped, 29,369 partial, 51,823 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded); serial manifest verification PASS (110 raw logs); git diff --check PASS excluding raw CR COM1 transcripts. Real make qemu-sysboot-m217 PASS through the existing DQA0/IRQ14 700-byte Files-11 read, then reached the expected incomplete-INIT stop. No SYSTEM login, DCL, DECnet or SET HOST 0 observed. Transcript logs/serial/qemu-m217-run-local-20261124T002306241980Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c.

Remaining: allocate and bind source CCBs through genuine SYS/channel state, invoke SYSQIO from the native VMS service ABI, connect DQA0's published DDT to process IRPs and AST/IOSB completion, port ACP/INIT continuation, and activate real VMS process images. Required login/DECnet/SET HOST 0 acceptance remains unachieved.

M253 implementation commit (local): 5a25c233b1aec0df2a293d70ae4278ec8187603c.

M253 GitHub main contents publication commits: 75109616da9e57e495b3ea3ae8f1bac467e4a044, fb7a298e28edfc0879611211121e590370920596, 9046fbae8fc04e3c4c4e87af74f4eb8c6deb5060, a84202f7846945be1e8b0e2776deb7addeaaf293, 817b209f6d632db330d2f152567c2fac6886359d, 70dcdf30a9a181b7ab4bd9ccc8743ff0cd34e94f, bdffcc5fc017bdde1c307014ceeb1ecc65b2f4fc, 939154e4c9cfbf3f79d84c1759c695f1f3db3e1e, coverage b2b19ef9ff189b175ad6e35a5e5edcee27fd122a; resume publication 2da903d83d0542a0769f39eaa0495f77ba8ba44d.
M253 resume publication commit: 2da903d83d0542a0769f39eaa0495f77ba8ba44d.


## M254 — SYSQIO begins through source CCB tables, 23 November 2026

Extended `port_v37_sysqio_begin` with a native source-layout channel path. It resolves the 16-byte channel token backward from the supplied CTL$GL_CCBBASE, enforces CCB AMOD, obtains the UCB's native DDB/FDT view through a UCB resolver, and re-reads the CCB after a scheduler wait callback. The existing direct channel callback remains for host fixtures. Host tests exercise successful begin through a byte-layout CCB, denied AMOD, invalid channel, access-pending refusal, and successful re-entry after the wait clears CCB$L_WIND. This integrates CCB lookup into the SYSQIO transaction code, but no SYS$ASSIGN allocator supplies the live per-process CCB table and no public VMS service vector calls the transaction.

Verification: `make -j2 all test sysqioresidentcheck sysexecentrycheck` PASS (exit 0); `make sysqioargscheck` PASS; source range generation and validation PASS (2,013 ranges, 0 mapping errors; 444/4,566 original source files indexed; 17,016 mapped, 29,369 partial, 51,823 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded); serial manifest verification PASS (111 raw logs); `git diff --check` PASS excluding raw CR COM1 transcripts. Real `make qemu-sysboot-m217` PASS through the existing DQA0/IRQ14 Files-11 read, then stopped at the incomplete INIT boundary. No public QIO was invoked; no SYSTEM login, DCL, DECnet or SET HOST 0 observed. Transcript `logs/serial/qemu-m217-run-local-20261124T003003365924Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`.

Remaining: implement genuine SYS$ASSIGN and the process CCB table, wire VMS system-service dispatch, complete process-owned DQA0 IOC/AST/IOSB behavior, implement ACP and finish native INIT/process activation, then run the required nested SYSTEM/DCL SET HOST 0 acceptance.

M254 implementation commit (local): `55079635ee0e79fe6205a05f2fddf8757d19ac63`.


M254 GitHub `main` exact-content publication commits: Makefile `d1d2dd7a63d9fcfa8f57ef6f4516af568f9c7ffb`; manifest `f759cbf59529825972b8eade7362da427d5e980f`; SYSQIO source `6b172fa30d230390a9dc8997acf7d1ae461b5e91` and `00142d0e44bd6c426c1e021ba5c897fc9f3f0a9e`; host test `a7a4139ed9d30e3ccbff04794da2191455aba0c2`; coverage generator `6aac6c796d7d5d653c19f9081d5a29d640aa7862`; raw QEMU log `5d2fdd3caf5f2e635afab62c67bd73e98d8b67c8`; exact coverage `63cbbbca7064bcd719c5756a4ffd008e1eb7ee33`; resume publication follows.
M254 resume publication commit: `005622f9329fb04b78ff217364cfc0d82fcd0808`.

## M255 — IOC free CCB allocation scan, 23 November 2026

Implemented the source IOC backward channel-table scan in the resident-linked CCB module. It walks 16-byte CCB tokens from channel 1, reads CCB, returns the first free channel/CCB, and updates CTL only when the new token extends the high-water mark. Host tests cover allocation, high-water update, exhausted tables, bounded backing storage, and a zero-channel table. Coverage now maps IOSUBPAGD.MAR lines 216–248 partially to this function; adjacent source routines and full SYS allocation remain uncovered. This does not yet create or bind a live process CCB table.

Verification: make -j2 all test sysqioresidentcheck sysexecentrycheck PASS (exit 0); source coverage generation/validation PASS (2,015 ranges, 0 errors; 444/4,566 original files indexed; 17,016 mapped, 29,402 partial, 51,790 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded); QEMU manifest check PASS. git diff --check PASS. Real make qemu-sysboot-m217 PASS through the existing 700-byte Files-11 DQA0 read via IRQ14, then halted at the known incomplete INIT boundary. Capture logs/serial/qemu-m217-run-local-20261124T003929803577Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c. QEMU exited normally. No public QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M255 implementation commit (local): 5b089e82dea3625af525e38c4468e1aa49c33da7.

Remaining: connect this allocator through genuine SYS$ASSIGN and live process CCB tables; connect public SYS dispatch, DQA0 DDT/CRB and process-owned completion/AST; implement the Files-11 ACP and native INIT continuation; activate resident VMS process images and complete DECnet Phase IV plus nested SYSTEM/DCL SET HOST 0 acceptance.

M255 GitHub main publication commits: CCB implementation ddb8912ffdd2b5e99f0b5d8bc4b3acc96e2bf834; header 2d102a422454129b6ce6764f7462457755a43175; CCB tests a624c35f1356fc95007bb17b0437bdd94281cf12; resident symbol check ad96235bc230ea1ab527411815e51a6818e456bb; coverage generator 511c1147d2666ddf939395ced986e169534e2f97; coverage aa97dfa310d389d6221c52dc26bc542a92a3e548; serial manifest 7b206dfe7bcde75e007d74c7c40dd147d9f8d1d5; QEMU transcript 57a4b07456fd599e856ae332ecb7b5c108e2ebf6. Resume publication before this final record: 44ce1ef87c0708b2215cbfc82801816216989bfc. GitHub blob IDs for all published files were checked against local Git blob IDs. Native Git push credentials remain unavailable; contents API publication succeeded.

## M256 — SYSQIO device privilege enforcement after IRP reservation, 23 November 2026

Connected the source PHY_IO/LOG_IO and volume-protection policy to the SYSQIO prepare transaction after standard IRP and AST reservation. The transaction now resolves its device security metadata, applies physical/logical access policy, sets IRP$M_PHYSIO only after authorization, and on denial returns the packet, restores process direct/buffered I/O and AST counts, and posts the mapped service status. PCB binding now carries the source ARB address into the process state. Host tests verify a physical request is denied without privilege with all reservations restored, then admitted with PHY_IO and marked physical in its IRP. This remains a transaction helper: no live SYS$QIO vector calls it, and runtime PCB privilege-mask/UCB protection binding, DQA0 process IRP dispatch, completion/AST posting, ACP and process activation remain incomplete.

Verification: make -j2 all test sysqioresidentcheck sysexecentrycheck PASS (exit 0); make sysqioargscheck PASS; source-range generation/validation PASS (2,015 ranges, 0 errors; 444/4,566 original files indexed; 17,016 mapped, 29,402 partial, 51,790 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded); QEMU manifest check PASS; git diff --check PASS. Real make qemu-sysboot-m217 PASS through the existing 700-byte Files-11 DQA0 read through IRQ14, then halted at the documented incomplete INIT boundary. Transcript logs/serial/qemu-m217-run-local-20261124T005456895201Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c. No public SYS$QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M256 implementation commit (local): c84d28f5d2c470173b0ea3b495d27d0af696129f.

M256 GitHub main publication commits: header cab3691ab1d4c3f93129b1031af8922617a710d8; transaction a80adfe87ef41b74803d6f7d5c2aa55089b47c34; PCB binding 4c09f560976f54aad1aff5bff0829a8cf9f6f888; host tests af9ce41b7aa6000c7c06880f8674e2d676e4a216; coverage generator be5307248d561ff305537c957a9b6331f0467782; coverage 10fed49efd994ed0c4380cafec4b1568990605c3; serial manifest 764702981096d6a6b579a75ecb7bfd6e81488500; QEMU transcript 21fa8f122d372f996a0c7b39cc0733ee1e8e1dd7. Resume publication before this record: d26d4ffb3e3d606cbddbc32441a5adc27cf816cb. All published content blob IDs match local Git objects.

## M257 — Bind QIO privilege decisions to the source PCB mask, 23 November 2026

The initial M257 change read the PCB's own eight-byte PRIV field to populate PHY_IO and LOG_IO flags. Source inspection of SHRLIB/SYSMAR.MAR IFPRIV showed that the service actually tests the process PHD privilege mask through PCB$L_PHD. M257's privilege source was therefore incorrect and is superseded by M258 below.

Verification: make sysqioargscheck PASS; make -j2 all test sysqioresidentcheck sysexecentrycheck PASS (exit 0); source coverage generation and validation PASS (2,015 ranges, 0 errors; 444/4,566 original files indexed); git diff --check PASS. Real make qemu-sysboot-m217 PASS for the existing 700-byte Files-11 DQA0 read via IRQ14, then stopped at the documented incomplete INIT boundary. Transcript logs/serial/qemu-m217-run-local-20261124T010209071845Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c. No public SYS$QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M257 implementation commit (local): 42ec9891bbdb60b5bb235e3c6e96875dbe0fcd96.

M257 GitHub main publication commits: PCB layout 6fbcd6ec596a3ebe998dc24ebdfcafed6ab75ca7; PCB privilege binding 2e6724d3a600c42c3693fd3935243f5cfa26a8aa; host tests 606d0a69e9b49461f4614719e252a956c4fa8d90; coverage generator corrected to complete content 668002147d2b489aaca5b1a2a800cf21e64f8132; source coverage corrected to complete content f32a97300a90674534f4e28338139c9de2cc751d; serial manifest 767f02df809d73fcd3058baeb2ca21cd6f469635; QEMU transcript 290538a421c4facc1aa4492c1e2173897c541122. Earlier truncated generator/coverage publication commits 680729fe4b4d438d7b844a4c8e5fb32fe55c00b0 and 60e6f948f0ec388ded7bf0bcea3b99994ad93506 were superseded immediately by the complete-file updates above. Resume content publication before this final record: 2b1184901d552e54a00c40d7f9eed35f20a27bff. Final local resume commit and publication follow.

## M258 — Correct SYSQIO privilege binding to source PHD PRIVMSK, 23 November 2026

Corrected M257 after tracing the original SHRLIB/SYSMAR.MAR IFPRIV macro: it dereferences PCB$L_PHD and checks the PHD mask at offset zero. The process binder now requires the matching PHD VA and bounded PHD record, derives PHY_IO/LOG_IO from PHD$Q_PRIVMSK, and retains the PCB only for the source quotas, PID, ARB and event flags. Tests verify the PHD mask grants the flags and clearing it revokes both. Public SYS$QIO dispatch and live PCB/PHD binding remain unimplemented.

Verification: make sysqioargscheck PASS; make -j2 all test sysqioresidentcheck sysexecentrycheck PASS (exit 0); source coverage generation/validation PASS (2,015 ranges, 0 errors; 444/4,566 original files indexed); git diff --check PASS. Real make qemu-sysboot-m217 PASS for the 700-byte Files-11 DQA0 read via IRQ14, then stopped at incomplete INIT. Transcript logs/serial/qemu-m217-run-local-20261124T011108781544Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c. No login, public SYS$QIO, DCL, DECnet or SET HOST 0 occurred.

M258 implementation commit (local): 9daf386cd7fe8bfb18b63ebfaf08592435f97e93. Publication details follow.

M258 GitHub main publication commits: SYSQIO process ABI 745a9c363a142a98af83bd19f9b5ba728b54d5f7; PHD-based privilege binding 8094cad758353d22f5590053f003ac786dd35693; PCB layout cleanup 1771d1f62ecf5b0e538583084069043488377d25; PHD privilege definitions fe8d861e6e13992c05aa7514784d7310d8cf25a7; host tests 26e83c245b817dbf6b01118f745c259eb7fe23a6; coverage generator 365654a15ed8eaa2c1e8ef0152aace4c984d531e; coverage 4742fa0c0441ff4f4587cd611586044a888695df; serial manifest 2a9728996f083b27a55af4406f56ff826fa6be77; QEMU transcript 48eaa1656786a93a4bbea72fb6ab10d0e97470db. All file blob IDs were verified against local Git objects. The M257 resume publication at e3ab8c817ded3319f7f9efa09458ca5d48bf1e0d was superseded by this corrected record.

## M259 — Validate the bound native PHD address, 23 November 2026

The SYSQIO PCB/PHD binder now rejects a PHD VA that differs from PCB$L_PHD without modifying the caller's process record. In native 32-bit builds it also verifies the supplied PHD pointer equals that VA. Host tests cover mismatch rejection and atomicity.

Verification: make sysqioargscheck PASS; make -j2 all test sysqioresidentcheck sysexecentrycheck PASS (exit 0); real make qemu-sysboot-m217 PASS through the existing 700-byte Files-11 read via DQA0 DDT/IRQ14 and stopped at incomplete INIT; git diff --check PASS. Transcript logs/serial/qemu-m217-run-local-20261124T011535282173Z.log, 5,350 bytes, SHA-256 8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c.

M259 implementation commit (local): 49c4e5e81ca9080b977a3b3c1244212ed47a4c78. Publication details follow.

M259 GitHub main publication commits: PHD address validation 0e0233e1b5a2f8ace1ec7a4c107e250e4abf60cf; host test 26209fec2778681d513f7465217d8052629de83a; serial manifest dd13473b9f51103ca213fbae1397b9a5362979d8; QEMU transcript 11eec1d34b02f0c2cd2d6b0d8ec2082135bb41f8. Each published blob was checked against the corresponding local Git blob. M259 resume publication: 44d162e7fd246601945c748241d97d9b81b89a8f.

## M260 — Correct DQA0 functional coverage records, 23 November 2026

Updated stale DQDRIVER, boot UCB, and system logical-name coverage descriptions to reflect the current published native DQA0 DDT/CRB and its physical READPBLK/WRITEPBLK routing through IOC$SEARCHDEV and bounded IRQ14 FIFOs. ACP, other DQ FDT actions, public process I/O, RB730 behavior, and process completion remain incomplete. No runtime behavior changed.

Verification: source coverage generation/validation PASS (2,015 ranges, 0 errors; 444/4,566 original files indexed; 17,016 mapped, 29,402 partial, 51,790 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded); git diff --check PASS. Runtime evidence remains the M259 QEMU transcript above; M260 did not run QEMU.

M260 local coverage commits: 6697d7b240ad75db71443ef2e24731b48fb9a219; bf363c4a40117921559003fbfc86d1b13c2aa71e. GitHub main publication commits: coverage generator 1fe875816ef448175f26db5c33170451d05fd427; source coverage e6c126ce09c8785c0f2442ed21ae6f9bf7856093. Published file blobs match local Git objects. M260 resume local commit: 33a4add5c12a6e3aa6651f0c6995230f56284697.

## M261 — Repeat full regression from M260, 23 November 2026

No source behavior changed. Rebuilt/checked the current tree with `make -j2 all test sysqioresidentcheck sysexecentrycheck sysqioargscheck`; PASS (exit 0). This includes the full host/layout suite, current SYSQIO argument and resident symbol checks, CCB checks, DQA private IOC tests, and the generated source audit (2,015 ranges; zero missing mapped functions). `make qemu-sysboot-m217` PASS under the sole QEMU runner: the guest performed the verified 700-byte Files-11 SYS.EXE chain read through published DQA0 DDT/IRQ14 and stopped at the documented incomplete-INIT boundary. No public SYS$QIO, login, DCL, DECnet or SET HOST 0 occurred. Runner exited before returning. Raw COM1 log `logs/serial/qemu-m217-run-local-20261124T012416575136Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`; manifest validation PASS. The local timestamp is UTC on 24 November despite the project checkpoint date.

M261 is verification-only; SYS$ASSIGN/channel-table allocation, public service dispatch, process-owned DQA completion/AST, Files-11 ACP, INIT continuation and process image activation remain incomplete. Acceptance remains unachieved. Local commit: `cf2d0b69d2ff3bd146f5d5cc5b51930bf44ebbf1`. GitHub `main` exact-content publication commits: QEMU transcript `04af1e5ab997bdb3ead9f7d317c1cc2a60c6c2aa`; serial manifest `328a3274a5cbab249f95816a7f51928d4417452c`. Both published blobs match the local Git objects. Resume publication follows.

## M262 — Native SYSASSIGN local CCB binding, 23 November 2026

Added a resident-linked local-device portion of `SYSASSIGN.MAR`: source IOC$FFCHAN scans a bounded process CCB table; under the supplied I/O database lock callbacks, the local path checks current/related ownership, share/spool flags and the allocation access callback, then binds CCB$L_UCB/AMOD/STS and increments UCB$W_REFC with source word wrap behavior. Allocation ownership is set only for a successful nonshareable, nonspooled assignment. Host cases verify successful CCB/UCB mutation, denied foreign ownership, shared-device policy, allocation denial, reference-count wrap, and mailbox rejection. The routine accepts an already resolved local UCB and effective access mode; it is a kernel helper, not a live SYS$ASSIGN service.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0); the resident symbol check finds the new helper in SYS.EXE. Coverage generation and validation PASS: 2,022 ranges; 445/4,566 original files indexed; 17,016 mapped, 29,476 partial, 52,131 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded. `git diff --check` PASS. Real `make qemu-sysboot-m217` PASS through the existing 700-byte Files-11 SYS.EXE read via DQA0 DDT/IRQ14, then stopped at incomplete INIT. The assignment helper was not invoked in QEMU. No public $ASSIGN/$QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred. Transcript `logs/serial/qemu-m217-run-local-20261124T013619850705Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`.

Remaining: live per-process CCB table and SYS$ASSIGN service vector, SYS$QIO dispatch, process-owned DQA IOC/AST completion, Files-11 ACP, native INIT continuation, process image activation, login/DCL and DECnet acceptance.

M262 local implementation commit: `05208d38300fea89e3817dc8bb16d5981eb81a55`. GitHub `main` exact-content publication commits: CCB helper `1427a99d29fcaba590a0c0a17079f63c489a9cf8`; CCB ABI `264514a126011544ffe996ba8af3b3bc4101ef71`; resident symbol check `31287578cacbcfebb22922653cd752a5e2578ce5`; host regression `ef794778e6f6007299e1865196c37c3a885e0625`; coverage generator `04d14f62e518d56b5be483a78dd56724fc15ca1f`; generated source coverage `3721f1739b4eafbb15345e5f80c567ffbc403db5`; serial manifest `2f39ea9206967d557b93953bbb26aa4aff75458e`; COM1 transcripts `3949a9c7c206323c8a29ee9a788b3498404a26aa` and `611524aa16e4487fb7ca5ca60d17a5d84c4df6ac`. Published blobs match local Git objects. Interim M262 resume publication `82b9ee1268d9e597f4a84b1441dc490d6977faa6` is superseded by this record.

## M263 — Correct source IRP channel encoding, 23 November 2026

Compared `SYSQIOREQ.MAR` VCHAN/SUCCES with `IOCIOPOST.MAR` DIRPOST. SYSQIOREQ negates the positive CCB byte-offset channel index before storing it in IRP$W_CHAN; IOPOST sign-extends the negative offset to recover the CCB address. Corrected the native standard IRP builder to store that negative offset and reject zero or non-16-byte-aligned CCB offsets. The existing host fixture had incorrectly used channel value 2; it now uses the valid byte offset 32 and verifies the stored -32 token plus rejection of zero/misaligned offsets. This corrects IRP ABI construction only; no live SYS$QIO dispatch, channel IOC charge or process completion is connected.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0); SYSQIO IRP host regression PASS. Coverage generation/validation PASS: 2,022 ranges; 445/4,566 original files indexed; 17,016 mapped, 29,476 partial, 52,131 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded. `git diff --check` PASS. One QEMU runner: `make qemu-sysboot-m217` PASS through the existing 700-byte Files-11 SYS.EXE chain read via published DQA0 DDT/IRQ14, then the guest stopped at incomplete INIT. No public SYS$QIO, login, DCL, DECnet or SET HOST 0 occurred. Transcript `logs/serial/qemu-m217-run-local-20261124T014744603221Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`.

M263 implementation commit: `3b4b5fc08bcf2bb807a0feffbd5031a35f1e2848`. Connected the independently published GitHub `main` history to the local implementation with merge commit `0da1e681b3abe7a3ad2d945c41a48a914e91d124`, preserving both parent histories. During merge, the fetched remote tree's `src/arch.c` and `src/kernel.c` blobs failed a text-integrity check (36 KB high-entropy data in each); resolved these paths to the locally built and tested source versions. M263 SYSQIO changes remain in the merged tree. The latest merged-tree regression repeated all tests and the QEMU run: same M217 partial-INIT result; transcript `logs/serial/qemu-m217-run-local-20261124T015139034827Z.log`, 5,347 bytes, SHA-256 `e146f273d840af1a1d91f302ac93c175543705c957648e9ae42634b8dbfafa9d`. `core.autocrlf=true` is set in the local WSL Git configuration so the Windows CRLF worktree is checked consistently. GitHub `main` was pushed and independently verified at `57fe4beb49f9dc1f375ac0d4e85e02ad426f2eac`; final resume publication follows.

## M264 — Validate SYSQIO IRP channel against request token, 24 November 2026

Extended the M263 source ABI fix: `port_v37_sysqio_build_irp` now checks that its positive CCB byte-offset input matches the channel argument after SYSQIOREQ.VCHAN's source mask. This prevents constructing a valid-looking negative IRP$W_CHAN for a different channel than the request selected. Host cases use a valid 0x20 offset despite ignored high channel bits and reject a mismatched IRP spec without modifying the packet.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0); generated coverage validation PASS (2,022 ranges, 445/4,566 original files indexed; 17,016 mapped, 29,476 partial, 52,131 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded). `make qemu-sysboot-m217` PASS; actual guest again performed the 700-byte Files-11 read through DQA0 DDT/IRQ14 and stopped at incomplete INIT. This SYSQIO builder is still not invoked by resident/public dispatch. Transcript `logs/serial/qemu-m217-run-local-20261124T133540704672Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

M264 implementation commit and GitHub `main` verification: `74ab13305beabff1dbd8182b0d1f7b829c38bb4a`. Resume publication follows.

## M265 — Port IOCIOPOST CCB idle/deaccess transition, 24 November 2026

Added the bounded `IOCIOPOST.MAR` DIRPOST CCB step: decode the IRP's source negative byte-offset channel token, decrement `CCB$W_IOC` with 16-bit wrap, and when it reaches idle, transfer a queued `CCB$L_DIRP` to an `IOC$WAKACP` callback while preserving the source count increment and clear order. If a deaccess packet is pending but the caller has no ACP wake binding, the helper defers without mutating the CCB. Mapped original lines 972-1000 as partial. This helper is resident-linked and host-tested, but the IOPOST process path does not call it yet; IOSB, event flag, AST, process lookup and live ACP completion are still absent.

Verification: `make iopostendcheck` PASS, including CCB busy-to-idle, queued deaccess wake, callback-missing no-mutation, word underflow wrap and malformed channel no-mutation. `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0). Coverage validation PASS: 2,024 ranges; 445/4,566 original files indexed; 17,016 mapped, 29,505 partial, 52,102 unresolved, 7,799 missing, 26,237 excluded, 917 hardware-excluded. `make qemu-sysboot-m217` PASS through the real 700-byte Files-11 SYS.EXE DQA0 DDT/IRQ14 read, then stopped at incomplete INIT; this CCB completion helper was not exercised in QEMU. Transcript `logs/serial/qemu-m217-run-local-20261124T133934960381Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. Login, DCL, DECnet and SET HOST 0 remain unverified and incomplete.

M265 implementation commit: `1ef9ce67cfd957913c380dfa529ece2b9f8d02c5`. GitHub publication and resume commit details follow.

## M266 — Port IOCIOPOST DIRPOST completion ordering, 24 November 2026

Added a bounded native completion helper for the process-side portion of `IOCIOPOST.MAR` DIRPOST, source lines 1001-1015. It validates IRP/process/IOSB/AST inputs, then applies callbacks in source order: process/PHD accounting, M265 CCB completion and optional ACP deaccess wake, guarded IOSB status write, event flag post, and AST queue or IRP return. Preflight rejects malformed state before mutation. This remains a callback-driven helper: no live process lookup, IOSB translation, event flag, AST queue, or IRP lookaside binding is wired to resident IOPOST/public SYS$QIO.

Host regression `make iopostendcheck` PASS: verifies completion ordering and fields, queued deaccess wake, event/AST sequencing, no-AST free path, and malformed channel atomic rejection. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0). Coverage generation and validation PASS: 2,025 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,520, unresolved 52,087, missing 7,799, excluded 26,237, hardware-excluded 917. `make qemu-sysboot-m217` PASS: actual DQA0 DDT/IRQ14 read of the 700-byte Files-11 SYS.EXE chain, then stop at the documented incomplete INIT boundary. M266 process completion was not invoked by the guest. Transcript `logs/serial/qemu-m217-run-local-20261124T134316501733Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No public SYS$QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M266 implementation commit and publication details follow.

Remaining work: bind the completion helper to actual IOPOST process and IOSB/event/AST structures; public SYS$QIO dispatch and DQA0 process IRP queue; Files-11 ACP and deaccess integration; native INIT continuation; process image activation; genuine login/DCL and DECnet Phase IV nested SET HOST 0 acceptance.

M266 implementation commit (local): `af98aa6c3cdecf52104c22eeba8b8ed7bfd3b4d8`. GitHub publication follows.

## M267 — Port IOCIOPOST direct-I/O locked-page release, 24 November 2026

Added a source-ordered, bounded direct-I/O buffer unlock helper for `IOCIOPOST.MAR` lines 166-210. It computes page counts from each SVAPTE, byte offset, and transfer count; traverses the root IRP's IRPE chain; and unlocks both recorded buffer regions for every extension. It preflights the full chain before calling unlock callbacks, bounds extension count, and rejects malformed addresses without partial page release. PAGIO, SWAPIO, and segmented virtual-I/O paths explicitly defer to their separate source handlers. Coverage now maps lines 166-210 partially; lines 211-971 remain unresolved. The helper is compiled into resident SYS.EXE but is not yet connected to a live process completion path.

`make iopostunlockcheck` PASS: direct span rounding, two-area IRPE traversal, zero next-link termination, malformed-chain no-mutation, and deferred paged/virtual cases. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0). Source coverage generation/validation PASS: 2,026 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,565, unresolved 52,042, missing 7,799, excluded 26,237, hardware-excluded 917. Real `make qemu-sysboot-m217` PASS for the existing 700-byte SYS.EXE chain read via IOC-selected DQA0 DDT/CRB/IRQ14, then stopped at incomplete INIT. This new unlock helper was not invoked by QEMU. Latest transcript `logs/serial/qemu-m217-run-local-20261124T135322203589Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No public SYS$QIO, login, DCL, DECnet, or SET HOST 0 occurred.

M267 implementation commit (local): `6ea0c7f66e62eca5bf8b28646a24f081c6ee38ca`. GitHub publication and resume commit details follow.

Remaining IOPOST work includes actual process/PHD resolution and unlock callback binding, buffered and segmented virtual completion, IOSB access fault behavior, event/AST delivery, and IRPE deallocation. Public SYS$QIO, DQA0 process I/O, Files-11 ACP, native INIT continuation, process image activation, login/DCL and DECnet acceptance remain incomplete.

## M268 — Port IOCIOPOST DIRPOST PHD accounting, 24 November 2026

Added a native PHD counter primitive for `IOCIOPOST.MAR` DIRPOST lines 972-975. It selects `PHD$L_DIOCNT` or adjacent `PHD$L_BIOCNT` from the buffered-I/O selector and increments the source 32-bit counter with wrap behavior. Offsets are derived from the original `SYSDEFMP.MDL` PHD layout and checked against the existing PHD layout constants. The caller must still resolve `SCH$GL_CURPCB` to its live PHD; this primitive does not resolve process IDs and is not yet wired into IOPOST.

`make iopostaccountcheck iopostendcheck` PASS; M267's `make iopostunlockcheck` PASS. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0), including the IOPOST account/unlock regressions. Coverage generation and validation PASS: 2,028 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,551, unresolved 52,056, missing 7,799, excluded 26,237, hardware-excluded 917. Real `make qemu-sysboot-m217` PASS: 700-byte Files-11 SYS.EXE chain read through IOC-selected DQA0 DDT/CRB/IRQ14, then halted at the documented incomplete INIT boundary. PHD accounting was not exercised by the guest. Transcript `logs/serial/qemu-m217-run-local-20261124T135615688213Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No public SYS$QIO, login, DCL, DECnet or SET HOST 0 occurred.

M268 implementation commit and publication details follow.

Remaining: resolve current process PHD and connect direct-I/O unlock/accounting to real IOPOST; port measurement and diagnostic buffer processing; finish IOSB/POSTEF/AST/IRPE release integration; implement public SYS$QIO and Files-11 ACP; continue native INIT and activate real VMS process images. End-to-end DECnet Phase IV and nested SYSTEM/DCL SET HOST 0 acceptance remains unmet.

M268 implementation commit (local): `4f2af3da19b51805e23974e75390ba1601749b5b`. GitHub publication follows.

## M269 — Port IOCIOPOST positive-PID direct completion route, 24 November 2026

Added the process branch after the negative-PID kernel end-action route. It extracts the source PID process index, resolves the PCB through the source `INI_PSTATE` process vector, checks the full PCB PID sequence to reject recycled slots, increments the source 16-bit `PCB$W_DIOCNT`, invokes the M267 direct page-unlock path, and queues the completion KAST through a scheduler callback. Failed preflight/deferred paths restore the PCB count. Buffered, paging, swap and segmented virtual completions remain deferred. This route is resident-linked and host-tested; the live IPL$_IOPOST worker still only drains bootstrap kernel end-action IRPs, so no real process request currently invokes it.

`make iopostprocesscheck iopostunlockcheck iopostaccountcheck iopostendcheck` PASS. Process-route regression verifies source PCB-vector resolution, PID sequence rejection, 16-bit counter wrap, direct-buffer unlock, KAST handoff, and no mutation for buffered/virtual/stale-PID requests. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0). Source coverage generation/validation PASS: 2,029 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,551, unresolved 52,056, missing 7,799, excluded 26,237, hardware-excluded 917. Real `make qemu-sysboot-m217` PASS through the existing 700-byte SYS.EXE Files-11 chain read via IOC-selected DQA0 DDT/CRB/IRQ14; it then stopped at incomplete INIT. The new positive-PID route was not exercised in QEMU. Transcript `logs/serial/qemu-m217-run-local-20261124T140106829178Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No public SYS$QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M269 implementation commit and publication details follow.

## M270 — Port IOCIOPOST IRPE deallocation chain, 24 November 2026

Added a bounded IRPE chain planner for IOCIOPOST.MAR lines 1015-1035 and integrated its release ordering into DIRPOST. The planner resolves and validates each extension record before DIRPOST mutates process accounting/CCB state; release callbacks run after IOSB/POSTEF and before AST queueing or root IRP release, matching the source order. Missing live resolver/lookaside bindings defer. Malformed, unaligned, unresolved, or over-limit chains fail without partial DIRPOST mutation. The helper has explicit limits of 32 IRPE records. System-buffer MOVBUF/PROBEW and subsequent buffered/paging/segmented paths remain unresolved.

`make iopostendcheck` PASS, including two-record chain traversal/release ordering, over-limit rejection, missing-resolver defer, null-root rejection, and DIRPOST event → IRPE free → AST ordering. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS (exit 0). Source coverage generation and validation PASS: 2,030 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,571, unresolved 52,036, missing 7,799, excluded 26,237, hardware-excluded 917. `make qemu-sysboot-m217` PASS for the actual 700-byte SYS.EXE Files-11 DQA0 DDT/CRB/IRQ14 read; native INIT still halts at the documented incomplete boundary. M270 completion callbacks were not invoked by the guest. Transcript `logs/serial/qemu-m217-run-local-20261124T140728620382Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No public SYS$QIO, SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M270 implementation commit: `71e5d5c3d606c6243eaa8957e27f106adeeb26fd`. Published to `origin/main`; resume update follows.

Remaining: bind process postprocessing and IRPE lookaside callbacks to live structures; port system-buffer move/PROBEW, buffered/paging/virtual completion and IOSB access-fault behavior; connect real public SYS$QIO and Files-11 ACP; continue native INIT and process image activation; verify DECnet Phase IV and nested SYSTEM/DCL SET HOST 0 acceptance.

## M271 — Reject cyclic IOCIOPOST IRPE chains, 24 November 2026

Hardened the M270 IRPE preflight and release helper against repeated extension addresses. A self-loop or longer cycle now fails before DIRPOST mutates accounting, CCB state, IOSB/event state, or deallocates any packet; the release callback also rejects externally supplied duplicate plans before freeing anything. This prevents cyclic IRPE corruption from causing repeated deallocation.

`make iopostendcheck` PASS with new self-cycle and duplicate-release atomicity cases. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS for the existing 700-byte SYS.EXE read through DQA0 DDT/IRQ14, then incomplete INIT stop. Coverage generation/validation PASS (2,030 ranges; counts unchanged). QEMU did not invoke the IRPE completion helper. Transcript `logs/serial/qemu-m217-run-local-20261124T141647296893Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

M271 implementation commit: `8b9ea2f6ada07f15381c64360611d1c59358f7d1`. Published to `origin/main`; resume update follows.

## M272 — Preserve DQ FUNCTAB modifier semantics, 24 November 2026

Corrected the native DQ DDT action lookup to select the FDT/ DDT entry from the original six-bit `IO$_` function number while retaining upper modifier bits in the full IRP function word. This matches the source FUNCTAB indexing rule; only READPBLK/WRITEPBLK have native private action slots, and the guarded physical bootread/write handlers still reject unsupported modified requests.

`make dqddtcheck` PASS with modifier-bit READPBLK/WRITEPBLK lookup cases. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS for 700 SYS.EXE bytes through published DQA0 DDT/CRB and IRQ14, then the known incomplete INIT stop. Modifier lookup was host-tested only. Coverage generation/validation PASS, 2,030 ranges with inventory counts unchanged. Transcript `logs/serial/qemu-m217-run-local-20261124T142046860315Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. Public SYS$QIO, ACP, SYSTEM login, DCL, DECnet and SET HOST 0 remain incomplete.

M272 implementation commit: `27eee971f2532f87ee10899478d7d21a2a88f025`. Published to `origin/main`; resume update follows.

## M273 — Compose local SYS$ASSIGN lookup and CCB binding, 24 November 2026

Added a source-ordered local assignment front end that searches the actual native device database with `IOC$SEARCHDEV`, bounds the selected UCB within that database, and invokes the existing `IOC$FFCHAN`/local ownership/refcount/CCB helper. The channel output is cleared before lookup, matching SYSASSIGN entry behavior. This composes DQA0 lookup and channel binding in resident SYS.EXE. Descriptor access probing/translation, EXE$MAXACMODE, mailbox, spooled/network routes, and the public system-service vector are still absent; this is not a complete public SYS$ASSIGN or SYS$QIO path.

`make dqddtcheck` PASS with a native DQA0 search → CCB assignment → VCHAN resolve sequence and failed-search output clearing. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS, including SYS.EXE rebuild/image installation. `make qemu-sysboot-m217` PASS through the real 700-byte Files-11 SYS.EXE read through published DQA0 DDT/CRB/IRQ14, then the documented incomplete INIT stop; M273 assignment was host-tested only. Coverage generation/validation PASS: 2,032 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,573, unresolved 52,034, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T142847180747Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL or DECnet acceptance occurred.

M273 implementation commit: `eec283ff286c4c63111228a04b6c42b48dc6e066`. Published to `origin/main`; resume update follows.

## M274 — Bind assigned CCB lookup into SYSQIOREQ preflight, 24 November 2026

Added a PID-bound adapter from the assigned source CCB table to `SYSQIOREQ`'s channel resolver. It checks the caller PID, resolves the VMS channel token with CCB access-mode data, requires the CCB's UCB to match the selected DQA0 device, bounds the UCB against the native device database, and returns the associated DQ FDT for service preflight. The host integration now performs IOC$SEARCHDEV → local SYS$ASSIGN CCB creation → SYSQIOREQ EFN/channel/FDT preflight and verifies the direct-I/O quota charge. This stops at preflight; public service-vector dispatch, IRP submission to DQ STARTIO, real process PCB/PHD binding, completion and AST delivery remain incomplete.

`make dqddtcheck` PASS with the integrated assignment-to-QIO-begin sequence, PID mismatch rejection, and CCB/FDT binding. Full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS through the actual 700-byte SYS.EXE DQA0 DDT/CRB/IRQ14 read, then the documented incomplete INIT stop. M274 service-preflight integration was host-tested only. Source coverage validation PASS: 2,032 ranges; 445/4,566 indexed; mapped 17,016, partial 29,573, unresolved 52,034, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T143338945563Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL or DECnet acceptance occurred.

M274 implementation commit: `b3f618c56ef14f7aa354b560f863ca437b48f28a`. Published to `origin/main`; resume update follows.

Remaining: wire positive-PID completions to live IPL$_IOPOST using the real native PCB vector, PHD/page-map resolution and scheduler KAST queue; add buffered/paging/virtual paths and DIRPOST IOSB/event/AST/IRPE handling; implement real public SYS$QIO and Files-11 ACP; finish INIT and process activation; verify DECnet Phase IV and nested SYSTEM/DCL SET HOST 0.

M269 implementation commit (local): `7ec6ea951cc41cfdd9ffb1f25120467f6d632f24`. GitHub publication follows.

## M275 — Compose assigned DQA0 channel through standard IRP reservation, 24 November 2026

Extended the DQA0 host integration to follow IOC$SEARCHDEV → SYS$ASSIGN CCB creation → PID-bound channel resolve → SYSQIOREQ preflight → INIT packet lookaside reservation → 136-byte standard IRP construction and physical-I/O privilege marking. The test checks PID/UCB/function/negative channel fields, quota charge, and packet return. It remains a host integration test: no public service-vector entry dispatches it, and it does not submit the constructed IRP to DQ STARTIO or execute process completion/AST semantics.

Verification: `make dqddtcheck` PASS; full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make dqddtcheck` PASS; one `make qemu-sysboot-m217` PASS for the real 700-byte SYS.EXE Files-11 read via DQA0 DDT/CRB/IRQ14 then incomplete INIT stop. Coverage generation/validation PASS: 2,032 ranges; 445/4,566 indexed; mapped 17,016, partial 29,573, unresolved 52,034, missing 7,799, excluded 26,237, hardware-excluded 917. Latest serial transcript: `logs/serial/qemu-m217-run-local-20261124T143713933296Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

Coverage for SYSQIOREQ remains partial; its integration note now records the host-tested CCB-to-standard-IRP composition while preserving unresolved live service entry, IOC accounting, DQ submission, and process completion. No original DEC source or binary image was added.

Remaining: live public SYS$QIO dispatch and channel IOC accounting; DQ STARTIO submission with real IRP and IRQ completion; Files-11 ACP and AST path; native INIT continuation/process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

M275 implementation commit: `ba06b28ca92d8d1eab5dde42424cea59aa49b039`.

## M276 — SYSQIO CCB accounting through IOPOST completion, 24 November 2026

Added PID- and access-mode-bound SYSQIO channel accounting under the native I/O database lock. The host route now builds the source standard IRP, increments its assigned CCB's `CCB$W_IOC`, then feeds the IRP's negative channel index through the source DIRPOST completion helper and verifies the count returns to zero. A caller beyond the assigned access limit is rejected without changing the count. The charge/completion pair is host-tested only; the IRP is not submitted to the DQA0 driver and no live service or AST is delivered.

Verification: full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make dqddtcheck` PASS with SYSQIO/IOPOST CCB accounting and access denial; `make qemu-sysboot-m217` PASS through the real 700-byte Files-11 SYS.EXE DQA0 DDT/CRB/IRQ14 read then incomplete INIT stop. Source coverage generation/validation PASS: 2,032 ranges; 445/4,566 indexed; mapped 17,016, partial 29,573, unresolved 52,034, missing 7,799, excluded 26,237, hardware-excluded 917. Serial transcript `logs/serial/qemu-m217-run-local-20261124T144254406496Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No public $QIO, login, DCL, DECnet or SET HOST 0 occurred.

M276 implementation commit: `7640afd87f94036fce3095d643e247de0fcce972`.

Remaining: public SYS$QIO entry, IPL/deletion protection and EXE$ABORTIO; real DQ FDT/STARTIO submission with user buffer locking and asynchronous IRQ completion; ACP, process-aware AST delivery, PHD/native contexts, INIT continuation/process activation, SYSTEM login/DCL, DECnet Phase IV and nested SET HOST 0 acceptance.

## M277 — SYS$ASSIGN descriptor and access-mode front end, 24 November 2026

Added the source entry stage that probes and copies the caller's 8-byte VMS string descriptor and counted device name through the supplied caller-memory access interface, clears the output channel before any lookup, and applies `MAXACMODE=max(requested,current)` before IOC$FFCHAN/local UCB assignment. The DQA0 test now enters assignment through this descriptor path and verifies access-mode maximization and output clearing on descriptor access failure/oversize. It remains host-tested; channel output probing through a live process address space, public SYS$ASSIGN dispatch/status translation, mailbox/network/remote branches and real process CCB storage are not connected.

Verification: full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make dqddtcheck` PASS; `make qemu-sysboot-m217` PASS through the real 700-byte SYS.EXE Files-11 read over published DQA0 DDT/CRB/IRQ14 then incomplete INIT stop. Coverage generation/validation PASS: 2,033 ranges; 445/4,566 files indexed; mapped 17,016, partial 29,578, unresolved 52,029, missing 7,799, excluded 26,237, hardware-excluded 917. QEMU transcript `logs/serial/qemu-m217-run-local-20261124T145100251421Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login/DCL/DECnet/SET HOST 0.

M277 implementation commit: `ad480415dd7358d93c4fce2b5b79a0f0b827f28d`.

## M278 — Probe and publish SYS$ASSIGN channel output, 24 November 2026

Added a host-tested service boundary that probes and clears the caller-supplied 16-bit channel output before descriptor processing, then writes the assigned channel only after successful local DQA0 assignment. The test verifies successful output and that an inaccessible descriptor leaves the channel output cleared. Caller memory is still an abstract interface; this is not wired to live process address translation or a public SYS$ASSIGN system-service vector. Mailbox, network, and remote assignments remain unresolved.

Verification: full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck` PASS; `make dqddtcheck` PASS; one `make qemu-sysboot-m217` PASS through the actual 700-byte SYS.EXE Files-11 read via DQA0 DDT/CRB/IRQ14, then the known incomplete INIT stop. Coverage generation/validation PASS: 2,035 ranges; 445/4,566 original files indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. QEMU transcript `logs/serial/qemu-m217-run-local-20261124T145457597414Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet, or SET HOST 0 occurred.

M278 implementation commit: `03fe4b8448b8b4ee30d91e0b4caae9260336fc59`.

Remaining: live SYS$ASSIGN dispatch and process CCB storage; public SYS$QIO service-vector entry and DQA0 STARTIO submission; Files-11 ACP; process-aware IOPOST/AST scheduling; native INIT and image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M279 — Bind executive caller memory to native page maps, 24 November 2026

Implemented `port_exe_user_access_init` to adapt the executive caller-memory interface to a process page map and kernel physical-page resolver. Reads/writes now use the existing checked copy-in/copy-out paths; read/write probes validate user PTE permissions. Integrated the adapter into the kernel link and tested cross-page descriptor-style reads/writes, writable/read-only protection, and translated physical aliases. The current kernel does not yet bind a live process context to SYS$ASSIGN or its public service entry.

Verification: `make -j2 all test sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS through the genuine 700-byte SYS.EXE DQA0 DDT/CRB/IRQ14 read, then incomplete INIT stop. Coverage generation/validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T145952768575Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

M279 implementation commit: `2c14bf089100d7805f15c6950c6312b8f7704d2c`.

Remaining: bind live process memory and CCB storage to SYS$ASSIGN; wire public SYS$QIO and real DQA0 STARTIO; implement Files-11 ACP; connect process-aware IOPOST/AST scheduling; finish native INIT and image activation; verify SYSTEM login, DCL, DECnet Phase IV and nested SET HOST 0 acceptance.

## M280 — Link caller-memory translation into resident SYS.EXE, 24 November 2026

Linked the M279 executive caller-memory adapter into resident SYS.EXE, reusing its existing page-map copy primitives without duplicate symbols. Extended the resident image check to require the adapter and checked read/write translation entry points. This makes the helper resident, but no live process page-map resolver or SYS$ASSIGN/SYS$QIO dispatch invokes it yet.

Verification: `make -j2 all test dqddtcheck sysexecentrycheck` PASS; resident-symbol check PASS; `make qemu-sysboot-m217` PASS through actual SYS.EXE loading and 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T150246205469Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M280 implementation commit: `b8bd1524e07b1324c52e01c0354dfef7fb4347e0`.

Remaining: connect live process memory/CCB context to the resident public SYS$ASSIGN vector; implement public SYS$QIO dispatch and real DQA0 STARTIO; Files-11 ACP; process-aware IOPOST/AST scheduling; native INIT and image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M281 — Exercise SYS$ASSIGN through translated IA-32 caller pages, 24 November 2026

Replaced the SYSASSIGN host fixture's abstract byte callbacks with the production executive user-page adapter. The integrated test now places the VMS string descriptor, counted device name, and channel result in a mapped IA-32 user page, executes output probe/clear and descriptor/name copies through page translation, and preserves failed-descriptor output clearing. Source coverage now records this precise translated-page integration. It remains a host invocation of resident helper code; the live process map/CCB and public SYS$ASSIGN entry are not wired.

Verification: `make -j2 all test dqddtcheck sysexecentrycheck` PASS; DQA0 SYSASSIGN→CCB→SYSQIO preflight host regression PASS; `make qemu-sysboot-m217` PASS through actual SYS.EXE load and 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T150603291318Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

M281 implementation commit: `dcc9ad21a08636b09048032648998e14e1f340bb`.

Remaining: actual public SYS$ASSIGN/SYS$QIO service dispatch with live process map and CCB; DQ STARTIO request submission; Files-11 ACP; real process-aware IOPOST/AST scheduling; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M282 — Preserve SYSASSIGN invalid-name status codes, 24 November 2026

Corrected descriptor validation to return the VMS `IVLOGNAM` status for zero-length or overlong device names and `ACCVIO` for a nonempty descriptor with a null name pointer. Added regression assertions for oversize-name and inaccessible-name behavior. Previously these cases leaked a private IOC status, which did not match SYSASSIGN's documented service statuses.

Verification: `make -j2 all test dqddtcheck sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS through SYS.EXE load and the real 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T150854594676Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

M282 implementation commit: `99e06bce6bdee75880d77936ed665e246e867e38`.

Remaining: live public SYS$ASSIGN/SYS$QIO service dispatch and process context; DQA0 STARTIO submission; Files-11 ACP; process-aware IOPOST/AST scheduling; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M283 — Translate SYSASSIGN IOC errors to VMS service statuses, 24 November 2026

Added public-boundary status translation for invalid physical device names (`SS$_IVDEVNAM`) and absent devices (`SS$_NOSUCHDEV`), rather than returning private IOC parser/search codes. Host integration verifies both through mapped descriptor/name/channel pages and confirms output channel clearing on failure. Other incomplete routes, including logical/network, mailbox, no-channel and process/service dispatch, remain open.

Verification: `make -j2 all test dqddtcheck sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS through SYS.EXE load and the real 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T151215272158Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

M283 implementation commit: `e213a540203065eda1658ea474bbab9aac16a9e4`.

Remaining: public SYS$ASSIGN/SYS$QIO dispatch with live process context; DQA0 STARTIO submission; Files-11 ACP; process-aware IOPOST/AST scheduling; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M284 — Qualify SYSASSIGN status mapping by failing stage, 24 November 2026

Fixed an ambiguity in the M283 translation: private IOC and CCB enums reuse numeric values, so translating a status by number alone could turn `SS$_ACCVIO` into `SS$_NOSUCHDEV`. The local path now reports whether failure occurred during validation, IOC search, or CCB binding; the descriptor front end translates only statuses from the matching stage. Added a channel-exhaustion case verifying `SS$_NOIOCHAN` and output clearing, alongside prior invalid-name and missing-device cases.

Verification: `make -j2 all test dqddtcheck sysexecentrycheck` PASS; `make qemu-sysboot-m217` PASS through SYS.EXE load and the 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T151554417523Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login/DCL/DECnet/SET HOST 0.

M284 implementation commit: `6293df505891713d066cc1838b3d786bc35daff0`.

Remaining: public SYS$ASSIGN/SYS$QIO dispatch and live process context; actual DQA0 STARTIO from user IRPs; Files-11 ACP; process-aware IOPOST/AST scheduling; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M285 — Bind SYSASSIGN channels to source-PCB process state, 24 November 2026

Added per-process CTL channel-table/high-water fields to the source-PCB-bound SYSQIO process view and a validated channel-state binder. SYSASSIGN descriptor handling now derives PID, ARB, CCB base, channel capacity and high-water storage from that bound process when supplied, overriding standalone request fields. The integration regression creates a native PCB/PHD process, binds its CCB table, deliberately clears the request's duplicate CCB/PID/ARB fields, and assigns DQA0 through the process-owned state. It also verifies invalid table rebinding is atomic and channel exhaustion updates the process high-water semantics correctly. The scheduler still does not supply a live process CTL allocation or dispatch public SYS$ASSIGN.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS; `make qemu-sysboot-m217` PASS through resident SYS.EXE load and the 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop; coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,580, unresolved 52,027, missing 7,799, excluded 26,237, hardware-excluded 917. Transcripts: `logs/serial/qemu-m217-run-local-20261124T152152518212Z.log` and `logs/serial/qemu-m217-run-local-20261124T152331029638Z.log`, each 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M285 implementation commit: `38c12a15228ac2aa692194548bd146e564dc8675`.

Remaining: bind running process page maps and CTL allocations to a genuine public SYS$ASSIGN/SYS$QIO service entry; dispatch standard user IRPs through DQA0 STARTIO; Files-11 ACP; real process-aware IOPOST/AST scheduling; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M286 — Read public SYS$QIO argument lists through caller page maps, 24 November 2026

Added a SYSQIO public-entry adapter that reads the caller's argument-count word, rejects undersized lists through the existing parameter validation path, validates/copies the fixed 52-byte VMS argument list through the checked user-page adapter, then invokes the source-ordered SYSQIO begin/IRP-reservation path. The DQA0 integration regression submits its AP from a translated CPL3 page and verifies an inaccessible AP returns `SS$_ACCVIO` without charging process I/O, posting an event, or reserving an IRP. Driver submission and the live system-service trap remain unbound.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS; `make qemu-sysboot-m217` PASS through actual SYS.EXE load and the 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage validation PASS: 2,035 ranges; 445/4,566 indexed; mapped 17,016, partial 29,585, unresolved 52,022, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T152758556364Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M286 implementation commit: `4e35124e889a2fb790728257a3580f582f8094ff`.

Remaining: dispatch this process-bound QIO path from a live IA-32 system-service trap; submit the standard IRP through DQA0 STARTIO and real interrupt completion; implement Files-11 ACP, process-aware IOSB/event/AST delivery, full native INIT and process image activation; verify SYSTEM login, DCL, DECnet Phase IV and nested SET HOST 0 acceptance.

## M287 — Preserve SYSQIOFDT one-parameter IRP field, 24 November 2026

The standard SYSQIO IRP builder now applies the original `EXE$ONEPARM` FDT action by copying P1 to `IRP$L_MEDIA`; the host regression verifies the field value in the generated standard IRP. Added an explicit SYSQIOFDT coverage record: lines 177–179 are partial, all other lines remain unresolved. This does not provide live FDT dispatch or DQA0 submission.

Verification: the named build/test goals `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS; an extra chained goal `coverage-validate` does not exist in Makefile and returned a make error. `python3 tools/gen_source_coverage.py` PASS (2,038 ranges, 446/4,566 original files indexed, no missing mapped functions); `python3 tests/test_coverage_ranges.py` PASS. Coverage totals: mapped 17,016; partial 29,588; unresolved 52,887; missing 7,799; excluded 26,237; hardware-excluded 917. `make qemu-sysboot-m217` PASS through SYS.EXE load and the actual 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261124T153304693602Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M287 implementation commit: `265d50d9144f8db9aec6c1b599bfee900ab3b0a1`.

Remaining: live system-service trap dispatch; transfer-buffer page locking and proper IRP SVAPTE/BCNT setup; DQA0 STARTIO submission and real interrupt completion; Files-11 ACP; process-aware IOSB/event/AST delivery; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M291 — Integrate SYSQIO transfer access rollback, 24 November 2026

Connected M290 caller-buffer accessibility checking to `port_v37_sysqio_prepare_user_irp` after source-order channel/IOSB/I/O/IRP/AST reservation and device privilege checks. A READ request now rejects a read-only user page with SS$_ACCVIO, returns its private IRP, restores direct/buffered I/O and AST charges, and posts the requested event before returning. A valid transfer proceeds with the prepared standard IRP and its READ/WRITE BCNT/CARCON fields. The host DQA0 integration test covers this path with an actual translated user page and checks rollback. This is still preparation only: the page is not locked, SVAPTE/BOFF are unset, and the IRP is not submitted to the DQA0 DDT or completed asynchronously.

Verification: `python3 tools/gen_source_coverage.py` PASS (2,058 ranges, zero missing mapped functions); `python3 tests/test_coverage_ranges.py` PASS; `make sysqioargscheck dqddtcheck` PASS; full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make qemu-sysboot-m217` PASS. QEMU performed the actual 700-byte Files-11 SYS.EXE chain read across two sectors using the DQA0 IRQ14 route, then stopped at the known incomplete INIT boundary. No login or DCL observed. Runner exited. Transcript: `logs/serial/qemu-m217-run-local-20261124T160009276547Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`.

Coverage remains 446/4,566 original files indexed; 2,058 ranges; mapped 17,016 lines, partial 29,660, unresolved 52,815, missing 7,799, excluded 26,237, hardware-excluded 917. These are inventory counts only. Public source/artifact/secret scan and `git diff --check` passed; no redistribution-restricted original source or binary image was added.

M291 implementation commit: `9c13bc24396fdab26a0e127d38b43a84726eca61`.

Remaining priority: native page locking with rollback and correct IA-32 IRP buffer mapping, then actual DQA0 DDT STARTIO submission and interrupt completion. Public system-service vector dispatch, Files-11 ACP, process-aware IOSB/event/AST delivery, native INIT/process activation, SYSTEM login/DCL, DECnet Phase IV and genuine nested SET HOST 0 remain incomplete.

## M292 — Verify SYSQIO write-buffer permissions, 24 November 2026

Extended the source-integrated SYSQIO host regression to check the WRITE direction after the READ access-fault rollback. The same read-only translated user page is rejected as a READ destination and accepted as a WRITE source; the prepared WRITEPBLK IRP retains its 512-byte BCNT and has no READ status bit. This confirms the two VMS access modes through caller-memory preparation without claiming page locking or DQA0 submission.

Verification: `make dqddtcheck` PASS under AddressSanitizer and UndefinedBehaviorSanitizer. This was a host regression-only change; no QEMU run was needed. The last real QEMU boot remains M217, which verified the 700-byte Files-11 chain read over DQA0 IRQ14 and stopped at incomplete INIT (log and SHA-256 recorded in M291). Coverage counts and implementation are unchanged from M291. `git diff --check` PASS.

M292 test commit: `abeea001210827c22b915523b34fcff7f6950545`.

Remaining priority: implement native page locking and restart/rollback semantics, populate the native buffer mapping in the IRP, then connect to DQA0 STARTIO and process-aware completion. Public system-service vector dispatch, Files-11 ACP, native INIT/process activation, SYSTEM login/DCL, DECnet Phase IV and genuine nested SET HOST 0 remain incomplete.

## M289 — Populate SYSQIO read/write standard IRP fields, 24 November 2026

The IRP builder now sets `IRP$V_FUNC` from the classified READ action, copies low byte P4 to `IRP$B_CARCON`, and zero-extends low word P2 to `IRP$L_BCNT` for READ/WRITE/MODIFY FDT classes. Host coverage uses actual DQA0 FDT classification for READPBLK and WRITEPBLK and checks these standard IRP fields. SYSQIOFDT coverage now maps these source statements as partial. P1 access validation, page locking, BOFF/SVAPTE, driver queueing and completion are still not implemented; this does not enable a live SYS$QIO request.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); source coverage generation PASS with 2,048 ranges across 446/4,566 indexed original files and no missing mapped functions; `python3 tests/test_coverage_ranges.py` PASS; `make qemu-sysboot-m217` PASS through SYS.EXE load and the real 700-byte DQA0 DDT/IRQ14 Files-11 read, then incomplete INIT stop. Coverage totals: mapped 17,016; partial 29,592; unresolved 52,883; missing 7,799; excluded 26,237; hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T154301768656Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M289 implementation commit: `c82a4666b2cd61f6f8dc306a4a623a2aefe8045d`.

## M290 — Check SYSQIO transfer-buffer access, 24 November 2026

Added user transfer-range permission checks for DQA FDT READ and WRITE requests. The native page walk checks every user PDE/PTE over the complete low-word P2 byte count (up to 65,535 bytes), rejects overflow and inaccessible pages, uses write access for READ destinations and read access for WRITE sources, and returns SS$_ACCVIO without mutating the planned buffer. Zero-length transfers skip the probe. READ/WRITE IRP BCNT and CARCON population remains as M289. A source review corrected the DQA FDT MODIFY classification: DQA ACP MODIFY is not SYSQIOFDT EXE$MODIFY, so it gets no transfer-buffer probe or READ/WRITE BCNT/CARCON setup. This helper is not yet wired into the live public SYS$QIO path; page locking, BOFF/SVAPTE construction, driver queue submission, restart/fault behavior and completion remain incomplete.

Verification after the correction: `python3 tools/gen_source_coverage.py` PASS (2,058 ranges, zero missing mapped functions); `python3 tests/test_coverage_ranges.py` PASS; `make sysqioargscheck` PASS; full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make qemu-sysboot-m217` PASS, with a real IRQ14 DQA0 read of 700 SYS.EXE bytes across two sectors, then the same incomplete INIT boundary. No login or DCL observed. The runner exited before returning. Raw COM1 transcript: `logs/serial/qemu-m217-run-local-20261124T155309410219Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. A repeated verified M217 run is retained at `logs/serial/qemu-m217-run-local-20261124T155001348823Z.log`.

Source coverage inventory: 446/4,566 original files indexed; 2,058 ranges; mapped 17,016 lines, partial 29,660, unresolved 52,815, missing 7,799, excluded 26,237, hardware-excluded 917. These counts are inventory only and do not establish functional completion. Public diff audit found no DEC source, executable/disk image or recognized secret material.

M290 implementation commit: `0033750575b1289f068a83109946f45ebe176ae5`. Evidence-log commit: `6d72f9f18e7d545bc606b34455d63edccd5cb353`.

Remaining priority: connect checked transfer-buffer access to the live service in source order with page locking and rollback semantics, then route through DQA0 DDT/CRB/IRQ and genuine process completion. Files-11 ACP, native INIT, process scheduling/image activation, SYSTEM login, DCL, DECnet Phase IV and genuine SET HOST 0 remain unfinished.

Remaining: public system-service vector dispatch; caller buffer accessibility and lock/restart semantics; actual IRP SVAPTE/BOFF setup and DQA0 STARTIO queue/IRQ completion; Files-11 ACP; process-aware IOSB/event/AST delivery; full native INIT and image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M288 — Verify classified one-parameter SYSQIO FDT action, 24 November 2026

Strengthened the M287 regression so `IO$_SEEK` is classified from the actual DQA0 FDT before its `EXE$ONEPARM` behavior is checked in the native IRP builder. The test no longer forces the action class manually. This remains host-side source-action verification; no QIO service dispatch or live DQ seek hardware action is connected.

Verification: `make sysqioargscheck` PASS; `python3 tests/test_coverage_ranges.py` PASS; `python3 tools/gen_source_coverage.py` PASS (2,038 ranges, no missing mapped functions). No QEMU run was needed for this host-only test change. No system login, DCL, DECnet or SET HOST 0 acceptance has passed.

M288 implementation commit: `5166e22635c59ed12ea24453903d9b5f99ba8eac`.

Remaining: live system-service trap dispatch; transfer-buffer page locking and proper IRP SVAPTE/BCNT setup; DQA0 STARTIO submission and real interrupt completion; Files-11 ACP; process-aware IOSB/event/AST delivery; native INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M293 — Add rollback-safe native transfer page pins, 24 November 2026

Added reference-counted physical-frame pins that prevent demand-zero VMem release while a pinned frame is owned, plus a user transfer-range helper that pins every translated IA-32 page (including the 17-page maximum) and rolls back partial acquisition on failure. Host regressions cover pin counts, denied free/reclaim, max-span pin/unpin, and failure rollback. This is groundwork only: SYS$QIO does not yet retain these locks through IRP completion, populate SVAPTE/BOFF, submit to DQA0, or unlock through IOPOST. VMS IOLOCK page-fault/restart behavior and system PTE mapping remain incomplete.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); standalone userptr/physical allocator/VMem host regressions PASS; `python3 tools/gen_source_coverage.py` PASS (2,058 ranges, zero missing mapped functions); `python3 tests/test_coverage_ranges.py` PASS; `git diff --check` PASS. `make qemu-sysboot-m217` PASS through actual 700-byte SYS.EXE Files-11 chain read across two sectors using DQA0 DDT/IRQ14, then stopped at the existing incomplete INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T161106987579Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

Coverage inventory unchanged: 446/4,566 indexed original files, 2,058 ranges; mapped 17,016 lines, partial 29,660, unresolved 52,815, missing 7,799, excluded 26,237, hardware-excluded 917. No new original-source coverage is claimed by these primitives. The source metadata inventory confirms all 4,566 source files are present in the separate local source tree; no original DEC source or binary image was added to this commit. Public-service integration, ACP, INIT/process activation, login/DCL, DECnet Phase IV and SET HOST 0 remain incomplete.

M293 implementation commit: `447dca00e8f5de0e49db11ebee59f4c0075d0140`.

## M294 — Retain pinned frames in SYSQIO prepared requests, 24 November 2026

Connected M293 page pinning to the source-ordered `port_v37_sysqio_prepare_user_irp` path. Successful nonempty READ/WRITE buffer checks now pin the translated frames and retain the lock set with the prepared request; the explicit release routine models the post/abort owner. Pin failure returns the reserved IRP, restores I/O/AST charges, and posts SS$_ACCVIO. DQA0 host integration checks READ and WRITE direction, one-page retained ownership/release, and quota/event rollback on pin failure. This is still not live service/driver integration: IRP SVAPTE/BOFF remain unset, release is not called by IOCIOPOST, and VMS fault/restart semantics remain incomplete.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make dqddtcheck` PASS under ASan/UBSan; `make qemu-sysboot-m217` PASS through the actual 700-byte SYS.EXE Files-11 chain read across two DQA0 DDT/IRQ14 sectors and then the known incomplete INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261124T161817380729Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. Coverage generator and range regression remain PASS at 2,058 ranges, with no new full original-source coverage claim. No login, DCL, DECnet or SET HOST 0 occurred.

M294 implementation commit: `f8f39ec054dcd26dda9552881389cd0c3b6ab249`.

## M295 — Transfer data through pinned SYSQIO frames, 24 November 2026

Added stable physical-page copy helpers to the pinned range and connected SYSQIO READ destinations and WRITE sources to them. Copies translate through the retained frame list rather than relying on the process's current page map, support noncontiguous physical pages and page crossings, and preflight the full span before modifying data. Host regressions verify 600-byte cross-page transfer, noncontiguous backing, out-of-range rejection, DQA0 READ/WRITE direction, and frame release. The native DQA0 driver still does not consume these prepared requests; SVAPTE/BOFF, real STARTIO submission, completion-driven unlock and public service dispatch remain unfinished.

Verification: full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make dqddtcheck` PASS under ASan/UBSan; standalone userptr host regression PASS; `python3 tools/gen_source_coverage.py` PASS (2,058 ranges, zero missing mapped functions); `python3 tests/test_coverage_ranges.py` PASS. `make qemu-sysboot-m217` PASS through the real 700-byte SYS.EXE Files-11 chain read via DQA0 DDT/IRQ14 and stopped at incomplete INIT. Transcript `logs/serial/qemu-m217-run-local-20261124T162454064223Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No login, DCL, DECnet or SET HOST 0 occurred.

Coverage inventory unchanged at 446/4,566 originals indexed and 2,058 ranges; no additional original source lines are claimed covered by the transfer helpers.

M295 implementation commit: `104055b9e5e2c1fe21c2edb681160d596734f165`.

## M296 — Map SYSQIO page pinning coverage, 24 November 2026

Updated the generated original-source coverage for SYSQIOFDT READ and WRITE buffer-lock selection (lines 249-253). The native preparation path pins all translated IA-32 transfer pages after IRP reservation and rolls back failures; the record remains partial because it does not construct VMS SVAPTE/BOFF mappings, implement IOLOCK fault/restart behavior, submit a live DQA0 QIO, or complete through IOCIOPOST. No additional functional coverage is claimed beyond M293-M295.

Verification: full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` completed with all targets passing; `python3 tools/gen_source_coverage.py` PASS (2,058 ranges, zero missing mapped functions); `python3 tests/test_coverage_ranges.py` PASS; `python3 tools/validate_source_ranges.py` PASS. Counts: 446/4,566 indexed original files; mapped 17,016; partial 29,664; unresolved 52,811; missing 7,799; excluded 26,237; hardware-excluded 917. This metadata-only coverage correction did not require a QEMU run; latest real QEMU remains M217 at `logs/serial/qemu-m217-run-local-20261124T162454064223Z.log`, confirming the 700-byte SYS.EXE Files-11 chain read and incomplete INIT stop. No login, DCL, DECnet or SET HOST 0 observed.

M296 coverage commit: `ba4bda0abcacde0b183614dd3a9a2e3044b5c871`.

Remaining: native SVAPTE/BOFF mappings and IOLOCK semantics; live SYS$QIO service dispatch into DQA0 STARTIO and real process-aware completion; Files-11 ACP, full INIT, scheduling/process image activation, SYSTEM login/DCL, DECnet Phase IV and genuine SET HOST 0 acceptance.

## M297 — Retain caller IOSB pages for asynchronous completion, 24 November 2026

SYS$QIO caller preparation now pins the optional eight-byte IOSB before transfer-buffer pinning and retains both page-lock sets with the prepared request. Any IOSB or transfer pin failure returns the reserved IRP, releases every acquired pin, restores I/O/AST charges and posts SS$_ACCVIO. Added a completion helper that writes status, transferred byte count and device status through the pinned physical alias, independent of the current user page map. Host DQA0 integration invalidates the IOSB PTE before completion, verifies the eight output bytes still land in its pinned frame, and checks all pins release. This helper is not called by a live IOCIOPOST path; public SYS$QIO service dispatch, DQA0 queue submission, IOSB/event/AST process completion and completion-owned unlock remain unimplemented.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make qemu-sysboot-m217` PASS through the actual 700-byte SYS.EXE Files-11 chain read across two DQA0 DDT/IRQ14 sectors, then incomplete INIT stop. No login, DCL, DECnet or SET HOST 0 occurred. Coverage generator/range regression and source-range validator PASS: 2,058 ranges, zero missing mapped functions, 446/4,566 original files indexed; mapped 17,016, partial 29,664, unresolved 52,811, missing 7,799, excluded 26,237, hardware-excluded 917. No additional functional original-source lines were upgraded. QEMU COM1 transcript `logs/serial/qemu-m217-run-local-20261124T164211830378Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`; an earlier same-command run is also retained. Only native source, coverage metadata and serial evidence were included; no DEC source or binary image.

M297 implementation commit: `6b046c279e702e1b7f0f72e8be60abe6a71fdd11`.

Remaining: live SYS$QIO dispatch and DQA0 STARTIO submission; actual IOCIOPOST IOSB/event/AST delivery and one-owner pin release; native Files-11 ACP, INIT, scheduling/process image activation, SYSTEM login/DCL, DECnet Phase IV and genuine SET HOST 0 acceptance.

## M298 — Add retained SYSQIO completion path to DIRPOST, 24 November 2026

IOCIOPOST DIRPOST now accepts an optional prepared SYSQIO completion pair. It validates PID/IRP/IOSB ownership before changing PHD accounting or CCB state, then invokes completion after CCB decrement and before POSTEF, matching source order. The completion callback can write the IOSB through M297's retained physical pages and release transfer/IOSB pins; requests using the existing guarded user-copy callback remain compatible. Host regressions prove invalid ownership defers without accounting/CCB mutation, and valid completion receives the IRP identity, caller mode, IOST1/IOST2 and runs before event posting. The callback is not bound to a live queue or scheduler; public SYS$QIO dispatch and process-aware completion remain incomplete.

Verification: full `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make iopostendcheck` PASS under UBSan; source coverage generation/range validation PASS (2,058 ranges, zero missing mapped functions); `make qemu-sysboot-m217` PASS through the actual 700-byte SYS.EXE Files-11 chain read across two DQA0 DDT/IRQ14 sectors, then incomplete INIT stop. No login/DCL/DECnet/SET HOST 0 observed. Coverage inventory unchanged: 446/4,566 originals indexed; mapped 17,016, partial 29,664, unresolved 52,811, missing 7,799, excluded 26,237, hardware-excluded 917. Transcript `logs/serial/qemu-m217-run-local-20261124T164659383508Z.log`, 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`.

M298 implementation commit: `1c6a6c00186b325f705bcb8115d1f08618188b3d`.

Remaining: bind retained SYSQIO state to actual DQA0 submission and running IOCIOPOST; real PCB/PHD accounting, event/AST scheduling and single-owner pin release; Files-11 ACP, full INIT and process image activation; SYSTEM login/DCL, DECnet Phase IV and genuine SET HOST 0 acceptance.

## M299 — Retain SYSQIO pins through bounded DIRPOST completion, 24 November 2026

Added a 32-entry resident SYSQIO pending table. It validates standard IRPs against PID, IRP VA, IOSB and request mode, rejects duplicate ownership, atomically takes ownership of the prepared request and its pinned IOSB/transfer pages, and supplies DIRPOST validation/completion callbacks. Completion writes status, information and device status through the retained IOSB page alias, then releases both pin sets. The DIRPOST host regression now exercises the actual table, deferred validation on corrupted IRP IOSB, successful IOSB write and exactly-once unpin. This is a tested callback composition only: no live public SYS$QIO dispatcher submits to DQA0 or binds this table to a running process/scheduler.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make iopostendcheck` PASS under UBSan; `make qemu-sysboot-m217` PASS with actual 700-byte Files-11 SYS.EXE chain read across two DQA0 DDT/IRQ14 sectors, then the existing incomplete INIT stop. The new run transcript `logs/serial/qemu-m217-run-local-20261124T191314310380Z.log` is 5,350 bytes, SHA-256 `8ff4be5730e7a2bf66155c8372459b2abeaabed345df51718f5cec790712629c`. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred. `python3 tests/test_coverage_ranges.py`, `python3 tools/validate_source_ranges.py` and staged source whitespace checks PASS; source inventory is 446/4,566 indexed originals, 2,058 ranges, mapped 17,016, partial 29,664, unresolved 52,811, missing 7,799, excluded 26,237, hardware-excluded 917. No original DEC source, image or secret was added; generated coverage and raw serial evidence only accompany native code.

M299 implementation commit: `f876849c40230a06a4aeed4d8174eb6f224a0f79`.

Remaining: connect public SYS$QIO system-service dispatch to DQA0 STARTIO and the pending table; implement SVAPTE/BOFF and IOLOCK restart semantics; connect real IOCIOPOST to process PHD/CCB/event/AST completion and scheduler ownership; Files-11 ACP; complete native INIT and process image activation; SYSTEM login and DCL; DECnet Phase IV; genuine nested `SET HOST 0`/second login/LOGOUT end-to-end QEMU acceptance.

## M300 — Release retained SYSQIO pins on pre-submit abort, 24 November 2026

Added a pending-table abort operation for requests whose driver submission fails before activation. Under the table's IPL synchronization it claims only the matching PID/IRP/IOSB/mode entry, removes it before releasing both retained pin sets, and refuses repeat or nonmatching aborts. DIRPOST host coverage now verifies ownership transfer, successful post-failure rollback, exactly two unlocks, and duplicate-abort rejection.

Verification: `make iopostendcheck` PASS under UBSan; `make -j2 all sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); source coverage generation and range consistency PASS (2,058 ranges, zero missing mapped functions); source-range validator PASS, 446/4,566 originals indexed. No QEMU rerun: the change affects only the isolated pending-table abort path and does not change the boot path; latest actual M217 transcript remains recorded under M299. Public SYS$QIO dispatch, DQA0 submission, process AST scheduling and all login/DECnet acceptance remain incomplete.

M300 implementation commit: `fa66c84728fc44c8e64ea4344cac0b0147a49287`.

Remaining: integrate this ownership rollback into live public SYS$QIO/DQA0 request submission and its real failure/cancel paths; construct native SVAPTE/BOFF mapping and IOLOCK restart semantics; connect real IOCIOPOST to PCB/PHD/event/AST and scheduler ownership; Files-11 ACP; full INIT/process activation; SYSTEM login/DCL; DECnet Phase IV; genuine nested SET HOST 0 acceptance.

## M301 — Exercise Files-11 ACP RDHEDR through published DQA0, 24 November 2026

SYS.EXE's post-publication DQA0 phase now calls the native F11B `port_f11b_read_header` on the actual SYS.EXE FID 11. FID-to-INDEXF lookup and FH2 validation use the boot volume's HOME/INDEXF data; the resulting header LBN is read through the published DQA0 DDT and CRB IRQ14 completion path, then compared byte-for-byte with the already-opened header. The guest checks the returned resident header pointer, one physical read and FID/sequence. This validates one ACP header operation in the boot path; it does not integrate ACP request queues, public SYS$QIO, or a process ACP context.

Verification: `make -j2 all sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `make qemu-sysboot-m217` PASS and emitted `INIT: Files-11 ACP RDHEDR read SYS.EXE FID 11 through DQA0 DDT/CRB IRQ14.` It then stopped at `INIT stopped: remaining original initialization dependencies incomplete.` Transcript `logs/serial/qemu-m217-run-local-20261124T203552759894Z.log`, 5,426 bytes, SHA-256 `d2ef9511c7352cd841188dcb8ba1f5269c42782572d67d58e38848b616d578ba`. QEMU terminated; no runner remains. Coverage generation, range regression and original-source range validation PASS; 2,058 ranges, 446/4,566 original files indexed, mapped 17,016, partial 29,664, unresolved 52,811, missing 7,799, excluded 26,237, hardware-excluded 917. No login, DCL, DECnet or SET HOST 0 occurred.

M301 implementation commit: `211450a`.

Remaining: full Files-11 ACP requests, public SYS$QIO dispatch and live DQA0 completion binding; process-aware AST and scheduler integration; full INIT and process image activation; SYSTEM login/DCL; DECnet Phase IV and genuine nested SET HOST 0 acceptance.

## M302 — Materialize SYS.EXE FCB in the guest ACP path, 24 November 2026

Extended the real boot-time F11B ACP operation beyond header retrieval: the verified SYS.EXE FH2 now passes through `port_f11b_create_fcb` using the resident FCB allocator, then the guest verifies FID, sequence, volume, header LBN and nonzero Files-11 allocation. The resulting FCB lives in SYS.EXE resident storage. This is one ACP open component; it does not provide ACP request queueing, UCB-backed VCB lifetime, window management, or public SYS$QIO integration.

Verification: `make -j2 all sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); QEMU `make qemu-sysboot-m217` PASS and emitted `INIT: Files-11 ACP RDHEDR/F11B FCB opened SYS.EXE FID 11 through DQA0 DDT/CRB IRQ14.` It then stopped at the existing incomplete INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T203842336736Z.log`, 5,437 bytes, SHA-256 `860a8ae069bc779100a3b61f2d5e964140aa69ea54a66f2d755a94fc8e6e4c26`. QEMU exited; no runner remains. Coverage generator, range regression and source-range validation PASS; source inventory unchanged at 2,058 ranges and 446/4,566 indexed originals. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M302 implementation commit: `6b8b2ebbfa238e1c4bf5a076f0f0088f166e1e82`.

Remaining: make ACP state and requests persistent across VMS channels/processes; connect real SYS$QIO dispatch to DQA0 STARTIO and IOCIOPOST; implement native INIT's remaining dependencies, scheduling and process image activation; SYSTEM login/DCL; DECnet Phase IV and nested SET HOST 0 acceptance.

## M303 — Implement the resident nonpaged first-fit allocator, 24 November 2026

Ported the variable nonpaged-pool allocation core from MEMORYALC: source-ordered free blocks, eight-byte rounding, first-fit search, low-address splitting, sorted return and adjacent-block coalescing. The allocator uses the executive's existing IPL-preserving synchronization contract. SYS.EXE initializes it over the actual free tail of its resident nonpaged area after INIT's fixed records and logical-name records; F11B now obtains its SYS.EXE FCB from this pool. The live port still lacks SRP/LRP/general ALONONPAGED lookaside fallback, paged pool, pool extension, resource waits/wakeup and ACP deallocation binding.

Verification: `make -j2 all exenppoolcheck sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); allocator host regression PASS under ASan/UBSan for alignment, first-fit split, allocation ordering, sorted coalescing, duplicate-free rejection, range checks and IPL restoration. Coverage generator/range test/source validator PASS: 2,061 ranges; 446/4,566 indexed originals; mapped 17,016, partial 29,742, unresolved 52,733, missing 7,799, excluded 26,237, hardware-excluded 917. `make -j2 all` PASS and real `make qemu-sysboot-m217` PASS; serial confirms `INIT: Files-11 ACP RDHEDR/F11B allocated SYS.EXE FID 11 FCB from native EXE$ALONONPAGED pool via DQA0 DDT/CRB IRQ14.` Boot then stops at the known incomplete INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T204827125237Z.log`, 5,469 bytes, SHA-256 `5b905642cec14dc790b891654ba216c65126f2aeca4d76183c609a78576076cb`. QEMU exited; no runner remains. No login, DCL, DECnet or SET HOST 0 occurred.

M303 implementation commit: `891b9e2871f4beb01743fcc291a6cb9f477fa51a`.
M303 boot-evidence commit: `a8c3f40b89467648960e7a35df0e6c9bfdf974d9`.

Remaining: connect public SYS$QIO to DQA0 STARTIO and completion; finish ACP channel/window/deaccess and release paths; port remaining INIT memory/paging/PHD/context dependencies and activate SYSTEM/DCL; start DECnet Phase IV and verify genuine SET HOST 0 second login and LOGOUT end-to-end.

## M304 — Reclaim failed Files-11 FCB allocations, 24 November 2026

Added an ACP deallocation callback and made F11A/F11B FCB construction return its nonpaged allocation when FH2-derived FCB initialization fails. SYS.EXE binds the callback to the resident variable-pool coalescing routine. The host ACP regression corrupts the header map start, confirms FCB initialization fails, and confirms the allocated block is returned; the normal live SYS.EXE open still obtains its FCB from the resident pool. Successful FCB deaccess and window teardown remain incomplete.

Verification: `make files1187check exenppoolcheck all sysexecentrycheck` PASS (exit 0); Files-11 ACP regression PASS and the native pool regression passes under ASan/UBSan. `make qemu-sysboot-m217` PASS through ACP FCB allocation from the real SYS.EXE nonpaged pool and DQA0 DDT/CRB IRQ14; boot then stops at incomplete INIT. Transcript `logs/serial/qemu-m217-run-local-20261124T205054652300Z.log`, 5,469 bytes, SHA-256 `5b905642cec14dc790b891654ba216c65126f2aeca4d76183c609a78576076cb`. Coverage generator/range test/source validator PASS; 2,061 ranges, 446/4,566 indexed originals, mapped 17,016, partial 29,742, unresolved 52,733, missing 7,799, excluded 26,237, hardware-excluded 917. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M304 implementation commit: `a2762279d44c327fb2c396a132d9d0934f536bb7`.

Remaining: successful ACP FCB/window teardown and channel request processing; public SYS$QIO dispatch to live DQA0 STARTIO/IOCIOPOST; complete INIT/PHD/context activation and SYSTEM/DCL; DECnet Phase IV and end-to-end SET HOST 0 second-login/LOGOUT acceptance.

## M305 — Close read-only Files-11 ACP accesses, 24 November 2026

Added the no-update F11B DEACCESS path for a single allocated WCB: it validates ACP ownership and counters, unlinks the window from the FCB queue, decrements access/no-writer/no-truncate and VCB transaction counts, clears process ACP context and returns the WCB to the native nonpaged pool. CREWIN records the allocation base and size and reclaims its block on failed window setup. The host ACP regression verifies successful read-only deaccess and that a write access is rejected as incomplete without changing ownership. DEACCS write/attribute, expiration, truncate, delete, spool, multi-segment window and full cleanup paths are not implemented; this close helper is not yet connected to a running VMS process ACP request.

Verification: `make -j2 all test exenppoolcheck files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (exit 0); `files1187check` PASS; coverage generation/range regression/source validator PASS, 2,064 ranges and 447/4,566 original files indexed (mapped 17,255; partial 29,742; unresolved 52,733; missing 7,799; excluded 26,310; hardware-excluded 917). `make qemu-sysboot-m217` PASS as boot regression through ACP SYS.EXE FCB allocation and real DQA0 DDT/CRB IRQ14, then the existing incomplete INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261124T205638496094Z.log`, 5,469 bytes, SHA-256 `5b905642cec14dc790b891654ba216c65126f2aeca4d76183c609a78576076cb`. QEMU exited; no runner remains. No login, DCL, DECnet or SET HOST 0 occurred.

M305 implementation commit: `1fee5f1ec973dfc5861a1880019933d38ca10d7c`.

Remaining: complete DEACCESS semantics and integrate process ACP requests; public SYS$QIO dispatch into DQA0 STARTIO/IOCIOPOST; native INIT and SYSTEM PHD/page map/context activation; SYSTEM login/DCL; DECnet Phase IV and real nested SET HOST 0 acceptance.

## M306 — Native IRP/LRP/SRP allocation selection, 24 November 2026

Extended the native lookaside queue primitive to preserve caller-supplied DYN tags for IRP, LRP and SRP packet classes, and added a MEMORYALC allocator that selects a source queue by request size and falls back to the live nonpaged variable pool when no suitable slot exists. Address-based deallocation returns lookaside packets to their originating queue or frees/coalesces variable-pool blocks. Host tests exercise all three queue classes, structure overwrite before free, variable fallback, duplicate detection, and IPL restoration. MEMORYALC coverage maps the allocation and release ranges as partial. The broader allocator is not wired into resident ACP: attempts to route ACP FCB/WCB allocations through it caused QEMU INIT to reject the DQA0 publication prerequisites. That runtime wiring was removed; the existing M305 FCB variable-pool route remains in the verified image.

Verification at implementation commit `34c34fdf0f73951354b4a17f15030e10b9d0e0c5`: `make -j2 all test exenppoolcheck files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS (host suite reached M49); `exenppoolcheck`, Files-11 ACP, SYSQIO, DQ DDT and resident SYS.EXE checks PASS; coverage generation and range regression PASS (2,064 ranges, no missing mapped functions); `tools/validate_source_ranges.py` PASS (447/4,566 indexed source files; mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917); serial manifest verification PASS (171 raw logs). QEMU M217 with the retained M305 ACP runtime path PASS: actual 700-byte Files-11 chain read through IOC-selected DQA0 DDT/CRB IRQ14, then the known incomplete INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T210848440134Z.log`, 5,469 bytes, SHA-256 `5b905642cec14dc790b891654ba216c65126f2aeca4d76183c609a78576076cb`. Two real runs with experimental ACP lookaside routing are retained as failures; both stopped at DQA0 DDT publication prerequisites. QEMU terminated after each run; no QEMU process remains. No SYSTEM login, DCL, DECnet or SET HOST 0 occurred.

M306 implementation commit: `34c34fdf0f73951354b4a17f15030e10b9d0e0c5`. ACP runtime lookaside selection remains to be diagnosed and integrated before public process I/O; all M305 remaining work still applies.

## M307 — Match source lookaside capacity selection, 24 November 2026

Corrected the generic lookaside allocator to let a request use any packet whose configured capacity is at least the rounded request, matching MEMORYALC's LRP/SRP size checks. Queue validation still checks full packet capacity, and deallocation restores that full slot to its source list. The host regression now requests a 144-byte LRP allocation from a 160-byte slot, overwrites it, and returns it successfully.

Verification at implementation commit `9ac8973b94c7ced5505c392a8d0b3b92e6782cbb`: `make -j2 all test exenppoolcheck files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` PASS; the host suite reached M49 with no failures. `exenppoolcheck` and `irpalloccheck` PASS; source range test/validator PASS (2,064 ranges; 447/4,566 original source files indexed; mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917); serial manifest validation PASS (174 logs). QEMU M217 PASS with the unchanged M306 production ACP path: real 700-byte Files-11 chain read over IOC-selected DQA0 DDT/CRB IRQ14, followed by incomplete INIT. Log `logs/serial/qemu-m217-run-local-20261124T212246587545Z.log`, 5,469 bytes, SHA-256 `5b905642cec14dc790b891654ba216c65126f2aeca4d76183c609a78576076cb`. Experimental ACP lookaside wiring still fails before DQA0 publication; a temporary diagnostic identified the CRB-read-ready prerequisite as absent despite the earlier CRB-read milestone. Diagnostic changes were removed. QEMU was terminated; no process remains. No login, DCL, DECnet or SET HOST 0 occurred.

M307 implementation commit: `9ac8973b94c7ced5505c392a8d0b3b92e6782cbb`. Next: diagnose why the CRB-read-ready state is lost when the expanded allocator is wired into SYS.EXE, then retest ACP integration before proceeding to public SYS$QIO and process INIT activation.

## M308 — ACP allocator state-loss trace, 24 November 2026

Instrumented an experimental resident ACP FCB route to capture the first SYS.EXE phase observed after `executive_dq_crb_read_ready` became unavailable. The QEMU run still stopped before DQA0 publication and reported phase value `01`; this does not identify the writer or reconcile with the visible kernel call sequence, so the state-loss cause remains unresolved. All diagnostic and ACP-routing code was removed. No production change is claimed.

Verification/evidence: no code verification claimed for the removed experiment. A separate paused-QEMU write watchpoint on `executive_dq_crb_read_ready` at `0x003466a8` observed zero initialization and a transition to one, then observed no further write before the guest halted; this conflicts with the phase tracker report and leaves the state-loss cause unresolved. The debug COM1 trace is `logs/serial/qemu-m217-run-local-20261124T213000000000Z.log`, 4,961 bytes, SHA-256 `9218f8dfdc5c3e173edadf3594ce72de73e47848a63b42ab800f6c20ebae10b0` (same serial bytes as `logs/serial/qemu-m217-run-local-20261124T212705998322Z.log`). Earlier M307 host/build/QEMU pass results remain at commit `9ac8973b94c7ced5505c392a8d0b3b92e6782cbb`. Both diagnostic QEMU instances terminated; no QEMU process remains.

## M309 — Isolate ACP callback execution before DQA0 publication, 24 November 2026

Retried the FCB allocator route using a stack-local queue descriptor, without a static allocator object. The QEMU publication guard again reported missing mask `0x001` (CRB-read-ready). A temporary callback counter contributed mask bit `0x100` if `executive_acp_alloc` ran before that guard; the reported mask did not include it. This narrows the regression to the resident SYS.EXE build/runtime state rather than an early ACP allocator call, but does not identify why the flag is unavailable. All diagnostic and FCB-routing changes were removed.

Evidence: four serial transcripts for the sequential failed M217 experiments are retained in the manifest. Latest: `logs/serial/qemu-m217-run-local-20261124T213858404334Z.log`, 4,961 bytes, SHA-256 `1f4303a27bc69be0ca01771bec70eb56595713b6707b0e654618a5d2746ae8ef`. These are failures, not boot passes. No production code change is claimed; latest verified production baseline remains M307 commit `9ac8973b94c7ced5505c392a8d0b3b92e6782cbb` and its passing M217 trace.

## M310 — Reverify retained DQA0 boot path, 24 November 2026

Rebuilt and reran the current production tree without the experimental ACP allocator route. `make -j2 all test exenppoolcheck files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` completed successfully; the host suite reached M49. `make qemu-sysboot-m217` passed: the real guest read 700 SYS.EXE bytes across two IOC-selected DQA0 DDT sectors via IRQ14 and then halted at the existing incomplete INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T214249084245Z.log`, 5,469 bytes, SHA-256 `5b905642cec14dc790b891654ba216c65126f2aeca4d76183c609a78576076cb`. The sequential QEMU runner exited; no QEMU process remains. No login, DCL, DECnet or SET HOST 0 occurred. Production behavior remains unchanged; ACP allocator routing is still unresolved after M308/M309 experiments.

Coverage remains at the M307 inventory: 2,064 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917. Latest production source commit remains `9ac8973b94c7ced5505c392a8d0b3b92e6782cbb`; M308/M309/M310 commits contain diagnostic evidence and verification records only.

Remaining: diagnose the CRB-read-ready publication prerequisite regression before wiring the expanded allocator into ACP; implement live SYS$QIO submission and process-aware completion; finish ACP request/deaccess and native INIT/process activation; then boot SYSTEM/DCL, initialize DECnet IV and verify real nested SET HOST 0/login/LOGOUT.

## M311 — Route resident ACP FCB allocation through MEMORYALC lookaside, 24 November 2026

Bound the resident SYS.EXE Files-11 ACP FCB allocation/deallocation callbacks to the M306/M307 source-sized IRP/LRP/SRP selector with variable-pool fallback. The guest now validates that the allocated FCB lies in the selected resident storage region and that variable-pool accounting matches the selected source. The real M217 QEMU transcript reports `INIT: ACP FCB source=3`, which is the SRP lookaside class, while executing F11B RDHEDR for SYS.EXE through the published DQA0 DDT/CRB and IRQ14. The QEMU harness now requires that source marker. Address-based ACP deallocation is bound but the successful retained FCB is not released in this INIT probe; public process ACP requests remain absent.

The first routed run failed because the old guest invariant required exactly one variable-pool allocation. The failure transcript is retained. Updated checks accept and validate the selected variable/lookaside origin; subsequent QEMU runs passed through the SRP-backed FCB and stopped at the unchanged incomplete INIT boundary. This does not complete public SYS$QIO, process ACP requests, AST completion, process activation, login/DCL or DECnet.

Verification at implementation commit `eb9f0594456f40d4ff547effbff5034324173681`: clean GCC build and full `make test` PASS; targeted MEMORYALC, Files-11 ACP, resident SYSQIO, CCB, SYSQIO argument, resident entry and DQA DDT checks PASS; QEMU M217 PASS with the required `ACP FCB source=3` marker and the real 700-byte SYS.EXE read over two IOC-selected DQA0 DDT sectors via IRQ14, followed by the explicit incomplete-INIT stop. Final raw COM1 transcript `logs/serial/qemu-m217-run-local-20261124T215756656357Z.log`, 5,375 bytes, SHA-256 `ea17fb683e55e6531059f6af38f6f7beaef502c8b62d0bf5ec2491376e5548f8`. `python3 tools/validate_source_ranges.py` PASS: 2,064 ranges, zero mapped-function errors; 447/4,566 original files indexed (mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917). Serial manifest validation PASS (185 logs). QEMU exited; no runner remains. No restricted DEC source, disk image or secret was staged.

Implementation commit `eb9f0594456f40d4ff547effbff5034324173681` is pushed to active `main`. Remaining: connect ACP request/window/data completion to a live process and public SYS$QIO, finish AST and scheduler integration and native INIT/process activation, then boot SYSTEM/DCL and verify DECnet Phase IV SET HOST 0, second login/DCL and LOGOUT end-to-end.

## M312 — Verify failed ACP FCB cleanup returns its SRP packet, 24 November 2026

Extended resident SYS.EXE's real Files-11 ACP boot path with the M304 invalid-FCB rollback case. After creating the valid SYS.EXE FCB from the real FH2 header, the guest corrupts a RAM copy of that header, calls F11B CREFCB, requires the source BADFILEHDR result, and verifies the failed allocation returns to the same allocator class without changing the SRP free count or nonpaged variable-pool allocation count. The QEMU serial transcript confirms `ACP FCB source=3` followed by the rollback marker; the runner requires both. This verifies source-addressed EXE$DEANONPAGED return for a failed SRP-backed FCB. It does not implement ACP process requests, successful FCB/window deaccess, public SYS$QIO or process AST completion.

Verification at implementation commit `dc885fd1f4eba4edf2658b1e5b06b45f666f1bbd`: clean GCC build and full `make test` PASS; targeted nonpaged allocator, Files-11 ACP, SYSQIO, CCB, resident entry and DQA DDT checks PASS; QEMU M217 PASS with both ACP allocation and rejected-FCB-reclaim assertions, actual 700-byte Files-11 SYS.EXE chain read through IOC-selected DQA0 DDT/CRB IRQ14, and the explicit incomplete-INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261124T220325016864Z.log`, 5,440 bytes, SHA-256 `128789c86fc6b577cf320bb2adbbaa5bc89c2f8991f6e39af600c7ad3392a023`. Source-range validation PASS (2,064 ranges, zero mapping errors; 447/4,566 original files indexed; mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917). Serial manifest verification PASS (186 logs). QEMU exited; no runner remains. No restricted DEC source or image was staged.

Implementation commit `dc885fd1f4eba4edf2658b1e5b06b45f666f1bbd` is pushed to active `main`. Remaining: connect ACP request/window/data completion to a live process and public SYS$QIO, finish AST/scheduler and native INIT/process activation, then boot SYSTEM/DCL and verify DECnet Phase IV SET HOST 0, second login/DCL and LOGOUT end-to-end.

## M313 — Exercise read-only ACP WCB access/deaccess on the resident boot path, 24 November 2026

Corrected ACP dynamic type tags to match the original VMS DYN tags (FCB 7, WCB 18), replacing the port's incorrect FCB/WCB tag values 1/2. SYS.EXE's live INIT probe now allocates a WCB through MEMORYALC for the actual SYS.EXE FCB, executes F11B MAKACC with read/no-write/no-truncate access, then executes the supported read-only DEACCS path. It checks FCB access and writer counters, the FCB window list, VCB transaction count, ACP open/cleanup state, return to the originating lookaside/pool and allocator integrity. The probe then retains the existing invalid-FCB rollback check. Host fixture tags and QEMU's required transcript markers were updated. This exercises the narrow read-only path; ACP process request queues, user arguments, data I/O and completion callbacks remain unintegrated.

Verification at implementation commit `1bd357e`: `make all`, targeted `files1187check` and `sysexecentrycheck`, full `make test`, source-range generation/validation, and serial-log manifest verification PASS. QEMU M217 PASS requires `INIT: ACP read-only WCB MAKACC/DEACCS balanced FCB, VCB, and MEMORYALC state.` and confirms the existing real 700-byte SYS.EXE Files-11 chain read through IOC-selected DQA0 DDT/CRB IRQ14; boot then halts at the existing explicit incomplete-INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T220914042996Z.log`, 5,520 bytes, SHA-256 `af17df39edddba2f6519e6c6d1db2c19b22e239f4212a0cc98b8c534ff6c3310`. Coverage remains 2,064 ranges with zero mapped-function errors; 447/4,566 original files indexed (mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917). Serial manifest verification PASS (187 logs). QEMU exited; no runner remains. No restricted DEC source or disk image was staged.

Remaining: public SYS$QIO dispatch and process ACP requests, real DQA process I/O completion, full DEACCS updates/cleanup, process-aware AST completion, native INIT continuation and process activation, SYSTEM login/DCL, DECnet Phase IV and end-to-end SET HOST 0.

## M314 — Verify ACP allocations return across all lookaside classes, 24 November 2026

Strengthened the resident ACP boot probe to snapshot all IRP, LRP and SRP free counts before allocating its WCB, then require all three queues and the variable-pool allocation count to be restored after read-only DEACCS. The same all-queue check now covers CREFCB's rejected-FH2 rollback. This closes the prior probe's evidence gap where it only compared the FCB's originating queue and could miss a WCB leak into another queue.

Verification at implementation commit `db6f51e`: `make all sysexecentrycheck` PASS; SYS.EXE host INIT checks PASS; QEMU M217 PASS at the unchanged incomplete-INIT boundary with the ACP MAKACC/DEACCS marker and real DQA0/IRQ14 SYS.EXE chain read. Final transcript `logs/serial/qemu-m217-run-local-20261124T221756284948Z.log`, 5,520 bytes, SHA-256 `af17df39edddba2f6519e6c6d1db2c19b22e239f4212a0cc98b8c534ff6c3310`; an earlier successful repeat is also archived. Source-range validation PASS (2,064 ranges, zero mapping errors; 447/4,566 indexed, same counts as M313). Serial manifest verification PASS (189 logs). QEMU exited; no runner remains. No restricted DEC source or disk image was staged.

Remaining work and final acceptance conditions are unchanged from M313; no public SYS$QIO process service, native INIT continuation, login/DCL or DECnet session is claimed.

## M315 — Add resident ACP GETFIB probe; fresh-build QEMU regression, 24 November 2026

Added a SYS.EXE INIT probe that calls native F11B GETFIB with an allocated WCB for the actual SYS.EXE FCB, verifies the returned FID number/sequence/volume and FIB length, and checks that the no-charge bit is cleared. Added a result assertion, serial marker, and source coverage note. This code compiles and the SYS.EXE host check passes; QEMU did not reach the probe, so guest GETFIB execution is not verified. The freshly linked kernel exceeded the old 251-block boot-loader cap, so the loader and image builder now allow 260 blocks (this build uses 257).

Build/host verification: `make all files1187check sysexecentrycheck` PASS before the final fault-reporting-only kernel edit; `make all` PASS after it. Files-11 ACP and resident SYS.EXE host checks passed. Source generation and `tools/validate_source_ranges.py` PASS (2,064 ranges, no mapping errors; 447/4,566 indexed; mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917). `python3 tools/checkpoint_serial_logs.py verify` PASS (194 entries before these five new failure logs; manifest records them).

Fresh QEMU M217 runs FAIL before SYSBOOT completes: after `IA-32 VMS CPL1-to-CPL2 frame dispatch and interrupt return passed.` and the SYS.EXE retrieval-map marker, the guest reports IA-32 GP vector 13, error `0x10`, EIP `0x00028D52` at `pop %es` in `arch_isr_common`; the diagnostic shows GDT entry 2 contents `0000FFFF 00CF9300`. One earlier run faulted with selector error `0x30`. The attempted kernel-segment restoration after the ring-2 probe did not resolve this interrupt-return fault. The runner now treats x86 faults as failures immediately. Failed logs: `logs/serial/qemu-m217-run-local-20261124T222504964691Z.log` (205 bytes), `...222914170207Z.log` (185), `...223050250827Z.log` (185), `...223102634433Z.log` (185), and `...223119860646Z.log` (248; SHA-256 `7cc260e4175f0e82555c53a56e606965d8bb919f6ff78011c115710e150a437b`). QEMU exited after each run. No ACP GETFIB guest success, INIT completion, login, DCL, DECnet, or SET HOST 0 is claimed.

Implementation commit `030967049661d807d01415254e9d9669d3f7a7cc` was pushed to active `main` in repository commit `3b2b5f6df1bae6bec81792ac874a6651ea3c0a10`. At that checkpoint the ISR restore fault, M217 boot, and GETFIB guest result were still unverified; M316 supersedes that state.

## M316 — Separate bootstrap stack from kernel BSS and reverify M217, 24 November 2026

The QEMU page fault during the M315 DQ boot-read phase was caused by the 0x90000 bootstrap stack growing down into the kernel BSS: QEMU showed a kernel callback pointer at 0x8c540 overwritten by the active stack reaching 0x8c40c. The pointer was then invoked by IRQ0, branching into SYS.EXE BSS at 0x00347280. Moved the protected-mode bootstrap stack top to 0x9f000, above the BSS, and updated the active BIOS-loader probe. Kept the earlier segment-safe ISR restore and 260-sector image bound needed by the fresh image. Also corrected clean-build dependency ordering and the Files-11 boot-path check link inputs. The source coverage inventory was regenerated; all serial results from this investigation, including failed runs, are in the manifest.

Verification at implementation commit `0e5daf11f30941938b8e6bb6b436edf03fb3b136`: clean `make clean && make all test files1187check sysexecentrycheck qemu-sysboot-m217` PASS; `make f11bootpathcheck` PASS; source-range validation PASS (447/4,566 indexed; mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917; zero mapping errors); serial manifest verification PASS (225 logs). QEMU M217 passed the real IRQ14 boot-read path, DQA0 DDT/CRB path, 700-byte Files-11 read, ACP FCB reclaim and MAKACC/DEACCS, and resident GETFIB probe, then halted at the explicit incomplete-INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T231423920512Z.log`, 5,597 bytes, SHA-256 `f947cac9e008b309f42f7267ef1a6800ca3650d2d3792768d4598d4cb79e59ef`. No login, DCL, DECnet Phase IV or SET HOST 0 occurred. The QEMU runner exited; no QEMU process remains. No original DEC source or disk image was staged.

Implementation commit `0e5daf11f30941938b8e6bb6b436edf03fb3b136`; verification record commit `de1e9154fc9879c227e178807a68958b3b9da2c7`. Both are pushed to active `main`. Remaining: finish public SYS$QIO with process-aware AST completion, integrate Files-11 ACP requests into real process I/O, continue native INIT and process activation, then verify SYSTEM login/DCL and the two-session DECnet SET HOST 0/LOGOUT sequence end-to-end.

## M317 — Reverify resident DQA0 and ACP baseline, 24 November 2026

Rebuilt/check-tested the existing production tree with `make all test exenppoolcheck files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck`; all emitted host, sanitizer, layout, resident-image and DQA checks passed, including the complete host suite through M49. `tools/validate_source_ranges.py` PASS: 447/4,566 source files indexed, mapped 17,255; partial 29,814; unresolved 52,661; missing 7,799; excluded 26,310; hardware-excluded 917; zero mapping errors. `tools/checkpoint_serial_logs.py verify` PASS (226 logs).

A fresh single-runner `make qemu-sysboot-m217` PASS. Real guest read 700 SYS.EXE bytes across IOC-selected DQA0 DDT sectors via IRQ14, exercised resident ACP FCB/WCB rollback, MAKACC/DEACCS and GETFIB probes, then stopped at the explicit incomplete-INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261124T232110096842Z.log`, 5,597 bytes, SHA-256 `f947cac9e008b309f42f7267ef1a6800ca3650d2d3792768d4598d4cb79e59ef`. QEMU runner exited. No SYSTEM login, DCL, DECnet Phase IV or SET HOST 0 occurred. No implementation or source-coverage status change in this verification checkpoint.

Verification base: `4ad5afbcff47e032ddc7cb411ee40d0867675688`; publish this record with the next resume commit. Remaining public SYS$QIO/process completion, ACP user requests, AST delivery, native INIT and process activation, login/DCL and DECnet acceptance work remain incomplete.

## M318 — Establish SYS$QIO ownership before DDT submission, 24 November 2026

Added `port_v37_sysqio_submit`: it registers PID/IRP ownership and transfers pinned caller IOSB/transfer-page ownership before invoking a DDT submission callback, so a fast completion can resolve the pending entry. If the callback rejects the request, it removes the entry and releases those pins. It leaves prepared ownership untouched when pending registration itself fails. Host coverage exercises callback-visible ownership, rejection rollback, accepted ownership, and later DIRPOST completion/unpin. This is a transaction boundary only: the live public SYS$QIO vector, DQA0 queue callback, IRP/quota rollback, scheduler wait and AST delivery are still not wired.

Updated `SYSQIOREQ.MAR` coverage lines 771-773 as partial to this callback boundary, explicitly leaving `EXE$INSIOQ`/`IOC$INITIATE` unimplemented. Coverage inventory: 2,066 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,817; unresolved 52,658; missing 7,799; excluded 26,310; hardware-excluded 917; zero mapping errors.

Verification at implementation commit `fed4464d59837562e49221e121ce2a8bc05e1aee`: `make iopostendcheck` PASS with UBSan; full `make all test files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck` completed with all host, layout, sanitizer and DQA checks passing. `make qemu-sysboot-m217` PASS at the existing incomplete-INIT boundary; actual 700-byte Files-11 chain read through DQA0 DDT/IRQ14 and ACP probes passed. Transcript `logs/serial/qemu-m217-run-local-20261124T232605575918Z.log`, 5,597 bytes, SHA-256 `f947cac9e008b309f42f7267ef1a6800ca3650d2d3792768d4598d4cb79e59ef`. Coverage validator PASS (zero errors); serial manifest validation PASS (227 logs). No SYSTEM login, DCL, DECnet IV or SET HOST 0 observed. No original DEC source or disk image was included in the commit.

Implementation commit: `fed4464d59837562e49221e121ce2a8bc05e1aee`. Remaining acceptance work is unchanged: integrate a real SYS$QIO service and DQA0 request queue with process-aware completion, complete ACP and native INIT/process activation, then verify SYSTEM/DCL and the two-session DECnet SET HOST 0/LOGOUT sequence.

## M319 — Preserve source IRP priority ordering, 25 November 2026

Initially changed the private DQA queue to select higher unsigned priority first and newer equal-priority packets first. M324 inspection of the actual `SYSQIOREQ.MAR` compare/branch and intrusive queue links found that interpretation was reversed; M324 supersedes it with the source behavior and updates the host/QEMU checks. This stage never wired the private queue to public `EXE$INSIOQ`/`IOC$INITIATE`.

Several incremental M217 runs stopped during DQ boot-read after the packet structure changed. Those actual failed serial captures and their manifest entries are retained. A clean rebuild removed the inconsistent incremental artifacts: `make clean all` followed by `make all test files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck dqprivateioccheck dqwriteioccheck` exited successfully; all requested host/sanitizer/layout checks passed and the host suite reached M49. `python3 tools/validate_source_ranges.py` PASS: 2,072 coverage ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,835; unresolved 52,609; missing 7,799; excluded 26,341; hardware-excluded 917; zero mapping errors. `python3 tools/checkpoint_serial_logs.py verify` PASS (235 raw logs).

Fresh single-runner `make qemu-sysboot-m217` PASS after the clean rebuild and host suite: actual ATA priority-ordered writes/readbacks/restoration and the existing 700-byte Files-11 read through IOC-selected DQA0 DDT/CRB IRQ14 passed; guest stopped at the explicit incomplete-INIT boundary. Transcript `logs/serial/qemu-m217-run-local-20261125T005653469733Z.log`, 5,609 bytes, SHA-256 `efc794757bbdd2220e92f28cc5a6a1c147198aa419edd3fc3a199cc1414f8994`. QEMU exited; no runner remains. No SYSTEM login, DCL, DECnet Phase IV or SET HOST 0 occurred. No restricted DEC source or disk image was committed.

Implementation commit `817088dfe401b59bb7d588949a2cf9f56cba9c69`. Remaining public SYS$QIO dispatch/ownership, DQA0 live IOC and process-aware completion, ACP process requests/deaccess, AST delivery, native INIT and process activation, SYSTEM/DCL, DECnet Phase IV and nested SET HOST 0/LOGOUT remain incomplete.

## M320 — Add a real IRQ-driven ATA read path, 25 November 2026

Added `port_ata_x86_read28_async` for one outstanding primary-master READ SECTORS operation. It returns while the command is armed, transfers the 256 words from the real device in the IRQ14 handler, and calls a bounded completion callback in interrupt context. A second concurrent start returns BUSY. The M217 QEMU boot binds the CRB first, disables interrupts across the start/return boundary, proves no callback ran before the API returned, restores interrupt state, and then checks exactly one IRQ14 completion, CRB dispatch, ATA status and the real LBN1 data. This is an ATA hardware primitive only: it is not yet reached through DQA DDT STARTIO, public SYS$QIO, a process IRP, IOCIOPOST or AST delivery. Coverage notes were corrected to preserve that boundary; no DQA/QIO completion is claimed.

Verification at implementation commit `c0e1fc5afd37808e6e69f66366874554c8960d4f`: `make all test files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck dqprivateioccheck dqwriteioccheck` PASS (host/sanitizer/layout and resident checks; host suite through M49). `make qemu-sysboot-m217` PASS with the asynchronous read returning before IRQ14, busy rejection, real sector transfer via the bound CRB and the pre-existing DQA0/ACP probes, followed by the explicit incomplete-INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261125T010614024193Z.log`, 5,701 bytes, SHA-256 `896e81a8c5f0f3750853fe57c3becf5d7e2f3b95edb2cbb34d38eac1f8fce530`. Two preceding real transcripts are retained; their first version used a misleading “DQA0 asynchronous” label and was corrected before the final verified run. Coverage validator PASS (2,072 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,835; unresolved 52,609; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors). Serial manifest validation PASS (238 logs). QEMU exited; no runner remains. No login, DCL, DECnet IV or SET HOST 0 occurred.

Implementation commit `c0e1fc5afd37808e6e69f66366874554c8960d4f`. Next integration work must attach this IRQ completion to native DQA STARTIO/IOC queues and retained process IRPs, then complete SYS$QIO/ACP/AST and actual process activation; final acceptance remains wholly outstanding.

## M321 — Queue private DQ IOC completion from real IRQ14, 25 November 2026

Connected the private DQ IOC queue to the one-request ATA asynchronous sector engine. The queue selects by IRP priority, retains the packet while hardware is active, records terminal status at IRQ level, and copies data/returns IOSB only when the caller performs IOC post. Host tests cover priority ordering, BUSY while active, deferred data/IOSB completion, and rejected starts. The M217 guest test starts with interrupts disabled and confirms return precedes IRQ, then observes real IRQ14/CRB completion and posts the retained packet. This is still a private IOC queue probe; it is not connected to published DQA0 STARTIO/UCB arbitration, a process IRP, public SYS$QIO, AST delivery, or user IOSB handling.

The flat kernel grew to 135,336 bytes (265 sectors); the boot count bound and image/layout checks now allow 265 sectors. The load ends at 0x31200, below linker BSS at 0x32000. Clean build and required regression set passed: `make clean all test files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck dqprivateioccheck dqwriteioccheck`; private IOC host test reports 33 distinct regressions. `tools/kernel_image_verify.py`, image/boot/DMA tests and source-range validation passed. Coverage inventory: 2,074 ranges; 447/4,566 source files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero mapping errors. Serial manifest verification passed (241 logs).

Fresh single-runner `make qemu-sysboot-m217` PASS: actual async DQ queue completion through IRQ14, with the existing DQA0 DDT/CRB and 700-byte Files-11 chain-read/ACP probes; then the guest stopped at the explicit incomplete-INIT boundary. A first run failed its test assertion because the expected CRB IRQ delta omitted the earlier synchronous DDT IRQ; the assertion was corrected and a fresh clean-build run passed. Captures retained: failed `logs/serial/qemu-m217-run-local-20261125T011447347295Z.log`; passing `...T011502240028Z.log` and `...T012101104444Z.log`. Final capture: 5,668 bytes, SHA-256 `fce6cf2471d0673128645ec28bd7705e0c06679421e137babb4a2a348c37710c`. QEMU exited; no runner remains. No SYSTEM login, DCL, DECnet Phase IV or SET HOST 0 occurred. Audit of staged changes found no original DEC source, disk image, credentials, or new external build dependency.

Implementation commit: `2d23db1c220c380c5d769b533a79973a4a9599ef`. Next work remains the public SYS$QIO to live DQA DDT/process IRP path, ACP integration, process-aware AST completion, native INIT and process activation, then genuine SYSTEM/DCL and DECnet nested SET HOST 0/LOGOUT acceptance.


## M322 — Route async DQ IOC through the published DQA0 DDT, 25 November 2026

Added resident `port_dqddt_startio_async` admission for privileged physical READPBLK. It validates the native DDT/FDT and published DDB/UCB relationship, applies the existing physical-I/O preflight to a retained supervisor IRP, then arms the async ATA engine. The guest starts only after DQA0 publication; IOC$SEARCHDEV confirms the bound unit, the IRQ commits the source IRP/UCB result using a function pointer validated inside loaded SYS.EXE, and private IOC post copies data and retires the packet after IRQ return. Host coverage checks accepted and rejected starts, callback deferral, LBN/function validation. This remains a single private physical read: DQA0 UCB busy arbitration, public process SYS$QIO, process IOSB, AST delivery, ACP requests and scheduler waits remain unimplemented.

A clean build and complete requested regression set passed: `make clean all test files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck dqprivateioccheck dqwriteioccheck` (host/layout/sanitizer suite through M49 and all targeted checks). `make qemu-sysboot-m217` PASS after the clean build: the published DQA0 async READPBLK retained its supervisor IRP through real IRQ14/CRB completion and deferred private IOC post; prior Files-11/ACP probes passed, then the guest stopped at explicit incomplete INIT. Coverage validator PASS: 2,074 ranges; 447/4,566 source files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest validation PASS (246 logs). Final capture `logs/serial/qemu-m217-run-local-20261125T014153341030Z.log`, 5,713 bytes, SHA-256 `1dbf49e0c9b39559737964f667544f7f5ec0d8ece8097920215dcb3b4fa99cb0`. QEMU exited and no runner remains.

During integration, an initial kernel link referenced resident helpers directly; the dispatch was corrected to use validated function pointers returned by SYS.EXE. One QEMU transcript was marked harness-fail because the success marker moved after DQA0 publication while the expected output list still placed it earlier; ordering was corrected and the next and final runs passed. A host fixture initially reused an IRP with prior completion fields; resetting it to a new source IRP made the asynchronous admission test pass. No DEC source, disk image, or new external build dependency was included.

Implementation commit: `99ab0382d31619e998cb92157b2672c4f755ac07`. Remaining work: DQA0 UCB busy/request arbitration and public SYS$QIO integration with process IRPs/quota, ACP process I/O, AST completion, native INIT and process image activation, SYSTEM login/DCL, DECnet Phase IV, and genuine two-session SET HOST 0/LOGOUT.


## M323 — Serialize private DQA IOC with the source UCB busy bit, 25 November 2026

Added a DQA unit-queue layer that uses the source SYSDEF `UCB$W_STS.BSY` bit (offset 88, bit 8), interrupt exclusion, and the private IRP priority queue. The first submission sets busy and enters validated published-DQA async READPBLK; later submissions remain queued while busy. Deferred post returns each IOSB and starts the next request without clearing busy; the final post clears busy. The QEMU path exercises two retained supervisor IRPs through IOC$SEARCHDEV, published native DDT preflight, actual IRQ14/CRB completion, and deferred IOC post, asserting busy remains set between operations and clears after the second. This remains private kernel I/O: standard user IRP dispatch, public SYS$QIO, user IOSB, process AST, and ACP/scheduler integration are still absent.

Clean verification passed: `make clean all test files1187check sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck dqprivateioccheck dqwriteioccheck`; the host suite reached M49 and the expanded private IOC suite reports 34 regressions PASS. Fresh `make qemu-sysboot-m217` PASS: two serialized DQA0 reads completed through IRQ14; the guest then stopped at explicit incomplete INIT. Coverage validation PASS: 2,074 ranges; 447/4,566 source files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest verification PASS (251 logs). Final transcript `logs/serial/qemu-m217-run-local-20261125T015240635212Z.log`, 5,710 bytes, SHA-256 `f7b8c82f50f6d2bfcbb9d928fff97bdd49888ca2ab69645eebb7c7bbdc79c988`. QEMU exited and no runner remains.

Three earlier QEMU captures showed the new probe's combined assertion failing; split diagnostics localized the remaining mismatch to an incorrect test assumption that the first word of boot LBN0 is zero. The second request now rereads known LBN1; the final clean-build run passed. Failed captures are retained in the serial manifest. No DEC source, disk image, or new external build dependency was added.

Implementation commit: `cb6f1e292775b2c4d2e35b0608bc590f5b91aa94`. Remaining: public process SYS$QIO integration and standard IRP queueing, user IOSB and AST completion, ACP process requests, native INIT/process activation, SYSTEM login/DCL, DECnet Phase IV, and genuine nested SET HOST 0/LOGOUT.

## M324 — Port source IRP queue order and intrusive links, 25 November 2026

Compared `EXE$INSERTIRP` at `SYSQIOREQ.MAR` 897-905 directly with the source `IRP$L_IOQFL/IOQBL` and `UCB$L_IOQFL/IOQBL` layouts in `SHRLIB/SYSDEF.MDL`. The source scans backward from the tail while the new unsigned priority byte is less than the current entry, then inserts after that entry. Therefore the forward queue is ascending numeric priority and equal priorities are FIFO. Corrected both private DQA read/write queue selectors and all priority-order tests; the DQA write probe now preserves its write-then-restore order using source-correct priorities. Added a bounded native intrusive IRP/UCB queue implementation with address resolver, source 136-byte IRP checks, reciprocal-link validation, priority insertion and removal. It is linked into resident SYS.EXE and host-tested; public SYS$QIO dispatch and live DQA submission are still not wired to it.

Verification: clean `make clean all test sysqioresidentcheck v37ccbcheck dqddtcheck dqprivateioccheck dqwriteioccheck sysirpqueuecheck` passed; the full host suite reached M49, private DQ IOC reports 34 regressions PASS, and the new SYS IRP queue test passed. After correcting the QEMU priority fixture to match the source ordering, fresh `make qemu-sysboot-m217` passed through the real IRQ14/DQA0 DDT, 700-byte Files-11 read, ACP probes, two-request UCB-busy probe, and scratch-sector write/read/restore checks, then stopped at the explicit incomplete-INIT boundary. The preceding run failed its fixture expectation after it observed a valid completion; its raw log is retained. Failed capture `logs/serial/qemu-m217-run-local-20261125T020520469196Z.log`, SHA-256 `f5b3d69deee1797c7531ec110e2fd71cbc93de8295c746cf23a2eb1da0cc6b95`. Passing capture `logs/serial/qemu-m217-run-local-20261125T020654554493Z.log`, SHA-256 `f7b8c82f50f6d2bfcbb9d928fff97bdd49888ca2ab69645eebb7c7bbdc79c988`. `tools/validate_source_ranges.py` PASS: 2,074 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest verification PASS (253 logs); kernel ELF layout verification PASS. QEMU exited; no QEMU process remains. No login, DCL, DECnet Phase IV or SET HOST 0 occurred.

`SYSQIOREQ.MAR` 897-905 now maps partial to `src/vms_irp_queue.c:port_v37_irp_queue_insert`; live public service/driver dispatch remains unresolved. Implementation commit `01fa6705aba6e83d136327b3fe75ff4dd6b61bd3` was pushed to `main`. The focused `make sysirpqueuecheck` rerun passed after the final bounds guard; the full host regression suite and M217 QEMU run passed as recorded above. End-to-end acceptance remains incomplete: public process SYS$QIO, ACP process requests, AST scheduling, native INIT/process activation, SYSTEM login/DCL and DECnet SET HOST 0/second-session LOGOUT.

## M325 — Validate source queue ordering invariants, 25 November 2026

Strengthened the source-layout intrusive UCB/IRP queue validator: it now rejects forward-linked queues whose unsigned `IRP$B_PRI` values descend, preventing insertion from accepting or propagating a queue that violates `SYSQIOREQ.MAR` `EXE$INSERTIRP` ordering. The host regression corrupts an otherwise valid queue's priority order and proves both validation and a subsequent insertion reject it; equal priorities remain valid and FIFO insertion/removal tests still pass. This hardens the native queue primitive only; public SYS$QIO and live DQA dispatch remain unwired.

Verification: `make sysirpqueuecheck` PASS; `make all test sysqioresidentcheck dqddtcheck dqprivateioccheck dqwriteioccheck` PASS (resident SYSQIO checks, full host suite through M49, and DQA/IOC regressions); `make qemu-sysboot-m217` PASS through the real DQA0 DDT/CRB IRQ14 and Files-11 read probes, then stopped at explicit incomplete INIT. Capture `logs/serial/qemu-m217-run-local-20261125T021401513460Z.log`, 5,710 bytes, SHA-256 `f7b8c82f50f6d2bfcbb9d928fff97bdd49888ca2ab69645eebb7c7bbdc79c988`. Coverage validator PASS: 2,074 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest validation PASS (254 logs); kernel image layout verification PASS. QEMU exited. No login, DCL, DECnet Phase IV or SET HOST 0 occurred. Full acceptance remains incomplete.

Implementation commit `f852531259fefc28388f08a07d3e9cdee81a4ee7` was pushed to active `main`.

## M326 — Build QIO queue priority from the source PCB, 25 November 2026

Compared `SYSQIOREQ.MAR` SUCCES lines 396-402 with the native IRP builder. The source rotates `PCB$B_PRIB` into `IRP$B_PRI`; the builder had accepted a separate caller-supplied priority even when given a bound source PCB. It now validates that PCB's fixed length, dynamic type, PID identity and priority range, then takes `IRP$B_PRI` from `PCB$B_PRIB`. Unbound host fixtures retain the explicit priority input. Regression proves a conflicting supplied priority cannot override the PCB and a mismatched PID is rejected. Source coverage description updated; this remains a partial builder, not live SYS$QIO dispatch.

Verification: `make all test sysqioresidentcheck v37ccbcheck sysqioargscheck dqddtcheck dqprivateioccheck dqwriteioccheck sysirpqueuecheck` PASS (full host suite through M49 plus resident SYSQIO, CCB, DQA/IOC and IRP queue checks); fresh `make qemu-sysboot-m217` PASS through existing live DQA0/IRQ14/Files-11 probes, then stopped at explicit incomplete INIT. Capture `logs/serial/qemu-m217-run-local-20261125T022003259589Z.log`, 5,710 bytes, SHA-256 `f7b8c82f50f6d2bfcbb9d928fff97bdd49888ca2ab69645eebb7c7bbdc79c988`. Coverage validator PASS: 2,074 ranges; 447/4,566 originals indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest PASS (255 logs); kernel image verification PASS. QEMU exited; no login/DCL/DECnet/SET HOST occurred. No restricted source or image was committed. Implementation commit `905471834b0a347d5d15e24ce0526cc2a722b191` was pushed to active `main`.

## M327 — Stage retained QIO process-buffer transfers, 25 November 2026

Added bounded, direction-checked copy helpers for the retained SYS$QIO pending ownership registry. Transfers use the process's pinned physical page aliases, require append-only offsets, cap bytes to the IRP request, serialize concurrent transfer/abort/completion, and require the IOSB information count to equal bytes staged before completion can mutate DIRPOST accounting or publish completion. Host regression coverage verifies read and write staging, rejected gaps, exact completion counts, deferred completion while incomplete, and pin release on abort. This is an unattached native primitive: the DQA0 IOC path still does not dispatch public SYS$QIO and no process I/O service vector, ACP request, AST delivery, or login path is established.

Verification: `make all test iopostendcheck iopostunlockcheck iopostaccountcheck iopostprocesscheck sysqioresidentcheck sysqioargscheck dqddtcheck dqprivateioccheck dqwriteioccheck sysirpqueuecheck` PASS; the host suite through M49 and the SYSQIO/DQA/IOC/IOPOST focused regressions passed. Fresh single-runner `make qemu-sysboot-m217` PASS for existing DQA0 DDT/CRB IRQ14 and Files-11 boot probes, then stopped at the explicit incomplete-INIT boundary. Capture `logs/serial/qemu-m217-run-local-20261125T022909868382Z.log`, 5,710 bytes, SHA-256 `f7b8c82f50f6d2bfcbb9d928fff97bdd49888ca2ab69645eebb7c7bbdc79c988`; runner exited and no QEMU process remains. Coverage validator PASS: 2,074 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest validation PASS (256 logs); kernel ELF layout verification PASS. No restricted DEC source, binary image, secret, or new external build dependency is included. No SYSTEM login, DCL, DECnet Phase IV, SET HOST 0, second login, or LOGOUT occurred.

M327 implementation commit `d9d33050657628a5b3bc18884b9559d640d901c6` was created after the recorded verification and includes the raw QEMU transcript and manifest entry. Source coverage remains partial/unresolved for public process QIO and DQA dispatch; this commit only adds the transfer staging primitive and completion preflight.

## M328 — Add IRQ-completed ATA PIO writes, 25 November 2026

Added a native asynchronous ATA WRITE SECTORS path. It starts a one-sector LBA28 PIO write, supplies the sector while interrupts are masked across the data phase, and returns before IRQ14 completion; the IRQ handler validates final device status and invokes the completion callback after releasing operation ownership. The resident M202 disposable-media regression now writes a marked scratch sector through this API, reads back all 512 bytes, restores the original full sector through IRQ14, and verifies the restoration. This is verified hardware substrate only: the existing DQ DDT WRITEPBLK route still uses its synchronous handler, and public SYS$QIO, process-buffer dispatch, Files-11 ACP, AST delivery and native INIT remain incomplete.

Verification: `make -j2 all test` PASS (native SYS.EXE/kernel/image build and host/layout suite through M49); `python3 tests/test_ata_irq_layout.py` PASS. The first fresh `make qemu-sysboot-m217` guest completed the async write/readback/restore probe, but the harness correctly rejected the transcript because its newly added marker was ordered after a later boot marker; this raw capture is retained as a harness failure. After correcting expected marker order, a second fresh single-runner `make qemu-sysboot-m217` PASS. Transcript `logs/serial/qemu-m217-run-local-20261125T023403125598Z.log`, 5,779 bytes, SHA-256 `11d131c65e0dde1fa8ccae5f3f272b20e6d4c894d5ee54f699cd118dd80ecfc0`; it shows `INIT: asynchronous ATA WRITE completed by IRQ14; scratch restored.` and then the explicit incomplete-INIT stop. The QEMU runner exited; no QEMU process remains. Coverage validator PASS: 2,074 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest validation PASS (258 logs); kernel ELF layout verification PASS. No restricted DEC source, binary image, secret, or new external build dependency was committed. No SYSTEM login, DCL, DECnet Phase IV, SET HOST 0, second login or LOGOUT occurred.

M328 implementation commit `ed8acfb9f63cfe619f63e512fa42fb13b098fa1c` includes the asynchronous ATA write implementation, guest verification, and both raw captures. The first capture's harness failure is an expected-marker ordering error only; its serial output already shows successful write completion and scratch restoration. Source coverage still records DQ write integration as partial because the existing DDT WRITEPBLK path remains synchronous and public process SYS$QIO is not connected.

## M329 — Exercise asynchronous physical writes through published DQA0 DDT checks, 25 November 2026

Added `port_dqddt_startio_async_write`, which verifies the active native DQ DDT/FDT action, published DQA0 UCB/DDT/CRB links, physical WRITEPBLK IRP fields, LBN and sector buffer before starting the asynchronous ATA write. SYS.EXE now exposes the adapter and the deferred `port_dq_bootwrite_commit` callback in the DQA publication result. The resident marked-media probe runs the write and restore through that adapter, waits for the real IRQ14 callback, commits the UCB/IRP result after interrupt return, verifies readback, and restores the original 512-byte sector. Host DQ DDT regressions cover accepted writes, rejected backend starts, and completion callback order. Coverage keeps the DQ write path partial and names its remaining boundary: this adapter does not run through the standard DQA IOC unit queue or public process SYS$QIO; WRITEVBLK, ACP and user completion remain unimplemented.

Verification: `make -j2 all dqddtcheck` PASS; `make test` initially exposed a stale static marker assertion after the M328 diagnostic changed, which was corrected; rerun `make test` PASS (host/layout suite through M49 and QIO/IOC/DQ/IOPOST checks). `make qemu-sysboot-m217` first failed when the new adapter pointers were not populated in the DQA publication result; the guest reached the expected scratch-write failure and the raw transcript is retained. After exporting those pointers from the actual SYS.EXE DQA publication phase, fresh single-runner `make qemu-sysboot-m217` PASS through the real IRQ14 DQA0 read/write probes, then stopped at the explicit incomplete-INIT boundary. Failed capture `logs/serial/qemu-m217-run-local-20261125T023757747689Z.log`, SHA-256 `61249616bd0bfd182b23ea48444b06f109d5f85bcbf3867b9280cba47930afa1`. Passing capture `logs/serial/qemu-m217-run-local-20261125T023835622964Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; it contains the DQA0 async physical write completion and verified scratch restoration. Runner exited; no QEMU process remains. Coverage validator PASS: 2,074 ranges; 447/4,566 original files indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors. Serial manifest validation PASS (260 logs); kernel ELF layout and ATA IRQ layout checks PASS. No restricted DEC source, binary image, secret, or new build dependency is included. SYSTEM login, DCL, DECnet Phase IV, SET HOST 0, second login, and LOGOUT remain unverified and incomplete.

M329 implementation commit `2cf91196e9b2013a7ea883ffc19ebe40131a2888` contains the DQ DDT async physical-write adapter, source-coverage update, host/QEMU checks, and both real transcripts. The initial QEMU failure above is retained and was resolved by publishing the function pointers in the DQA0 publication result; final QEMU and host checks passed.

## M330 — Preserve SYSQIOFDT transfer byte count, 25 November 2026

M330 initially misread the EXE$READ/WRITE common path. The source code at SYSQIOFDT.MAR 240-247 uses MOVZWL P2(AP),R1 before READLOCK/WRITELOCK, so the original VMS transfer byte count is the zero-extended low word of P2. The M330 full-longword builder change and upper-word test were source-inaccurate; M335 corrects them.

Verification: `make -j2 all test sysqioresidentcheck v37ccbcheck sysqioargscheck sysexecentrycheck dqddtcheck dqprivateioccheck dqwriteioccheck sysirpqueuecheck` PASS; full host/layout suite reached M49 and targeted SYSQIO/DQA regressions passed. Source-range validation PASS (2,074 ranges; 447/4,566 indexed; mapped 17,255; partial 29,776; unresolved 52,668; missing 7,799; excluded 26,341; hardware-excluded 917; zero errors). Guest-manifest and kernel ELF layout checks PASS. Fresh single-runner `make qemu-sysboot-m217` PASS for the existing DQA0 DDT/CRB IRQ14 and Files-11 probe, then stopped at explicit incomplete INIT. Capture `logs/serial/qemu-m217-run-local-20261125T024613826966Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; runner exited. No login, DCL or DECnet initialization occurred. M335 source coverage now records the correct zero-extended low-word P2 behavior at SYSQIOFDT.MAR 260-261; buffer access/locking and live submission remain unresolved.

Implementation commit: `e344392b1118f5a3c0d1800d5f85441f729a231f`. Public SYS$QIO dispatch, ACP integration, process AST/scheduler handoff, native INIT, process image activation, SYSTEM login/DCL and genuine DECnet SET HOST 0 remain incomplete.
## M331 — Preserve longword SYSQIO buffer lengths, 25 November 2026

M331 extended the same incorrect full-P2 assumption into buffer preflight and copy bounds. M335 restores low-word semantics and the 65,535-byte SYSQIOFDT limit after comparison with source MOVZWL. The range checker remains bounded to that source transfer size.

Verification: `make -j2 all test` completed build and integrated regression work; an initial explicit-target invocation failed because `userptr_hosttest` is an executable target, not a Make target, and the subsequent `make test` PASS completed the full suite. Direct `userptr_hosttest` compile/run PASS. Source-range validation PASS (2,074 ranges; 447/4,566 indexed; 0 errors). Guest manifest and kernel ELF checks PASS. Fresh `make qemu-sysboot-m217` PASS, then stopped at explicit incomplete INIT. Transcript `logs/serial/qemu-m217-run-local-20261125T025230480860Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited. No login, DCL or DECnet initialization occurred.

M335 coverage now maps the zero-extended low-word P2 through IRP construction and buffer checks; retained pin capacity and live dispatch remain partial/unresolved. Implementation commit: `6d7b5b1125dd20523e4199dd87e226a8613823bc`. Public SYS$QIO dispatch, Files-11 ACP process requests, AST scheduling, native INIT, image activation and acceptance sequence remain incomplete.
## M332 — Retain QIO page pins across the managed physical pool, 25 November 2026

M332 documentation claimed an expansion to 512 retained pages, but its implementation commit changed only resume.md and source-coverage.json; the native page-lock array remained 17 entries. M335 corrects the record and preserves the source-derived maximum 17 pages for 65,535-byte transfers.

Verification: `make test` PASS (host/layout suite through M49 and resident SYSQIO/DQA regressions). Fresh `make qemu-sysboot-m217` PASS for the established DQA0 DDT/CRB IRQ14 and Files-11 path, then explicit incomplete INIT. Capture `logs/serial/qemu-m217-run-local-20261125T025544381333Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; runner exited. Source-range validation PASS: 2,074 ranges, 447/4,566 indexed, zero errors. Guest-manifest validation PASS.

Implementation commit: `9a0e8d19fe0b313efa89d61cce8387ed84dd9b80`. SYSQIOFDT coverage remains partial for buffer fault/restart, live queue dispatch and process completion. Native INIT, process activation, SYSTEM login/DCL and DECnet nested SET HOST 0 acceptance remain incomplete.
## M333 — Verify complete native page-pin capacity, 25 November 2026

M333 test arithmetic used PORT_USER_PIN_MAX_PAGES, which was 17, so it exercised 17 pages, not the 512 described in the initial resume note. Its range length exceeded the source SYSQIOFDT count domain. M335 corrects the fixture to pin the true maximum 65,535-byte range from a page-end offset (17 pages) and verify one additional byte fails closed.

Verification: `make test` PASS (full host/layout regression suite through M49); direct QIO user-pointer regression PASS. Fresh `make qemu-sysboot-m217` PASS, then explicit incomplete INIT. Capture `logs/serial/qemu-m217-run-local-20261125T025829332000Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited. Implementation commit `6ab05ba577c2f183290f16a9085478ea07f83860`.

Public SYS$QIO remains unwired; full pin coverage does not provide FDT submission, ACP completion, AST delivery or process activation. Login/DCL and DECnet SET HOST acceptance remain incomplete.
## M334 — Port SYSQIOFDT zero-parameter IRP behavior, 25 November 2026

Implemented the original EXE$ZEROPARM action by explicitly clearing `IRP$L_MEDIA` for a classified ZERO FDT function. The host regression fills the destination IRP with nonzero bytes, builds a `IO$_NOP` request, and verifies the media longword is zero. Updated the source-coverage generator to map `SYSQIOFDT.MAR` 209-210 as partial while retaining the following `EXE$QIODRVPKT` queue transfer and other SYSQIOFDT setup as unresolved.

Verification: `make -j2 all test` PASS (full host/layout suite through M49 and resident SYSQIO/DQA tests). Fresh `make qemu-sysboot-m217` PASS; guest again stopped at explicit incomplete INIT. Two raw captures from before and after the zero-action code change are retained; final `logs/serial/qemu-m217-run-local-20261125T030345215846Z.log` is 5,788 bytes with SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`. Source coverage generation PASS (2,076 ranges, zero unmapped functions); validator PASS (447/4,566 files indexed, zero errors; 29,778 partial, 52,666 unresolved, 7,799 missing, 26,341 excluded, 917 hardware-excluded). Guest-manifest validation PASS. No login or DECnet behavior was exercised.

Implementation commit: `9f2ca7185a6f2a8bf7e4e4d42f5da6c5951ddedf`. Public SYS$QIO dispatch, DQA unit queue submission of process IRPs, ACP completion, process AST/scheduler activation, SYSTEM login/DCL and DECnet SET HOST 0 remain incomplete.
## M335 — Restore SYSQIOFDT P2 word semantics, 25 November 2026

Audited original `SYSQIOFDT.MAR` lines 240-247. EXE$READ/EXE$WRITE execute `MOVZWL P2(AP),R1` before the buffer-lock routine; `R1` is therefore the zero-extended low word, even though the argument-list slot and IRP$L_BCNT are longwords. M330-M333 incorrectly treated the high P2 word as transfer length. Restored 16-bit effective transfer counts in IRP construction, buffer plans, pinned-copy bounds and page-range validation; retained pin capacity is 17 pages. Host coverage checks that `P2=0x1234ffff` yields BCNT `0xffff`, `P2=0x10000` is a zero-length transfer, and a 65,535-byte unaligned range pins exactly 17 pages. Corrected the M330-M333 resume claims and source-coverage generator.

Verification: `make -j2 all test` PASS (host/layout suite through M49, SYSQIO and DQA regressions); source generation PASS (2,076 ranges, zero unmapped functions); `tools/validate_source_ranges.py` PASS (447/4,566 indexed, zero errors; 17,255 mapped, 29,778 partial, 52,666 unresolved, 7,799 missing, 26,341 excluded, 917 hardware-excluded); `tests/test_m47_qemu_manifest.py` PASS. Fresh `make qemu-sysboot-m217` PASS for DQA0 DDT/CRB IRQ14 Files-11 read, then stopped at explicit incomplete INIT. Raw capture `logs/serial/qemu-m217-run-local-20261125T030909592903Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited. No public SYS$QIO or user/session behavior was exercised. This correction supersedes M330-M333 transfer-width claims; their historical commits remain unchanged. Implementation commit: `676749095ef32489dd84355b63080e30a54a47f3`.

## M336 — Use original INIT SYSPARAM defaults, 25 November 2026

Added a native default table for the SYSPARAM.MAR values that drive INIT's lock tables, process vector, PHV and swap map: MAXPROCESSCNT=72, REALTIME_SPTS=0, LOCKIDTBL=128, RESHASHTBL=32, DEADLOCK_WAIT=10, BALSETCNT=36 and WSMAX=1024. These replace the prior port-local capacities (32 processes, 32 lock IDs, 37 hash request, 11 balance slots and 64 working-set pages). SYSBOOT PARAMS.DAT override loading remains unresolved, so the values are explicitly defaults. Increased the SYS.EXE resident lookaside/nonpaged backing region from 8 KiB to 16 KiB; QEMU first demonstrated the real 4 KiB free nonpaged region could not hold the source-default PHV+WSMAX map, then passed after backing the required allocation. Added host coverage for the parameter set and default PHV/swap-map sizes. Source coverage now records the exact seven original SYSPARAM.MAR declaration ranges as partial, with all other parameter/override handling unresolved.

Verification: `make -j2 all test` PASS (full host/layout suite through M49, including `inisysparamscheck`); `make iniphvswapcheck` PASS for default and arbitrary layout cases; source generation PASS (2,091 ranges, zero unmapped functions); `tools/validate_source_ranges.py` PASS (448/4,566 indexed, zero errors; 17,255 mapped, 29,835 partial, 55,616 unresolved, 7,799 missing, 26,341 excluded, 917 hardware-excluded); `tests/test_m47_qemu_manifest.py` PASS. Fresh `make qemu-sysboot-m217` PASS; native boot passed the 72-process vector, lock database, PHV and 1,024-page swap-map setup, then stopped at the existing explicit incomplete INIT boundary. Raw capture `logs/serial/qemu-m217-run-local-20261125T032228272926Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited. Earlier M336 capacity-failure captures are retained in the serial manifest.

SYSBOOT PARAMS.DAT overrides, full SYSPARAM state, running swapper/working-set behavior, public SYS$QIO, process image activation, SYSTEM login/DCL and DECnet SET HOST 0 remain incomplete.

Implementation commit: `92b087a3b2d961ad6bf9d62bde17c9e6b0884250`.

## M337 — Use source SYSPARAM packet and NPAGEDYN defaults, 25 November 2026

Extended the native INIT SYSPARAM default table with original IRPCOUNT=60, LRPCOUNT=4, LRPSIZE=576, SRPCOUNT=120, SRPSIZE=96 and NPAGEDYN=64000. SYSBOOT now sizes the resident IRP/LRP/SRP lists from those packet defaults; each LRP allocation is 640 bytes after the BOO header/alignment rules. SYS.EXE has 96 KiB of resident backing for lookaside packets and INIT nonpaged structures, but bounds native INIT allocations to NPAGEDYN's 64,000 bytes. PARAMS.DAT overrides and NPAGEVIR virtual mapping remain unimplemented. The baseline SYS.EXE host fixture and original parameter declaration ranges were updated. The `relocdrvcheck` target now links its M134 relocation and M197 CRB dependencies, which its previous recipe omitted.

Verification: first `make -j2 all test` exposed a 3,752-byte kernel stage-two overflow caused by dynamic packet-profile initialization crossing a 4 KiB load-image boundary; using a static profile for the current original defaults restored the existing 265-block limit. Rerun `make -j2 all test` PASS (native build and full host/layout suite); `make sysexecentrycheck iniphvswapcheck dqddtcheck relocdrvcheck f11channelcheck f11ioccheck sourcerangecheck` PASS after repairing the incomplete `relocdrvcheck` link recipe (the initial focused invocation failed at link time with missing `port_ddtreloc_apply` and `port_dq_crb_validate`). `python3 tools/validate_source_ranges.py` PASS: 2,103 ranges, 448/4,566 original files indexed, 17,255 mapped, 29,878 partial, 55,573 unresolved, 7,799 missing, 26,341 excluded, 917 hardware-excluded, zero errors. `tests/test_m47_qemu_manifest.py` PASS. Fresh single-runner `make qemu-sysboot-m217` PASS: guest initialized the source-default lookaside lists and previous real DQA0 DDT/CRB IRQ14/Files-11 probes, then stopped at the explicit incomplete INIT boundary. No login, DCL, DECnet or SET HOST behavior occurred. QEMU exited and no runner remains. Raw COM1 transcript `logs/serial/qemu-m217-run-local-20261125T033256339968Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; manifest validation PASS.

Coverage adds the six original SYSPARAM declaration ranges for these defaults; other INIT parameter resolution and SYSBOOT override loading remain unresolved. Public process SYS$QIO, ACP request integration, AST delivery, scheduler/image activation, SYSTEM login/DCL and DECnet SET HOST 0 remain incomplete. Implementation commit: `277c38f88808b6f2b651bada7ebb4240e1afd6ca`.

## M338 — Use original modified-page writer cluster default, 25 November 2026

SYSPARAM.MAR sets `MPW_WRTCLUSTER` to 96 pages. INIT had been constructing the MPW PTE/PHV map with a port-local 32-page profile. Changed the native default record and INIT map to the source default: 103 PTE/index slots, 630 bytes, swap increment 96. This initializes source-sized metadata only; no modified-page writer or swap I/O runs. PARAMS.DAT override resolution remains unresolved. Added the exact SYSPARAM declaration range to source coverage.

Verification: `make -j2 all test` PASS (native images and host/layout suite); `make inimpwcheck` PASS; `tests/test_m47_qemu_manifest.py` PASS; source-range validation PASS (2,105 ranges; 448/4,566 indexed; 17,255 mapped, 29,886 partial, 55,565 unresolved, 7,799 missing, 26,341 excluded, 917 hardware-excluded; zero errors). Fresh single-runner `make qemu-sysboot-m217` PASS and serial output confirms SYS.EXE initialized the modified-page-writer map with the new default before stopping at the explicit incomplete INIT boundary. Capture `logs/serial/qemu-m217-run-local-20261125T033916437186Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; manifest check PASS. No login, DCL, DECnet or SET HOST behavior occurred; QEMU exited.

Coverage adds SYSPARAM.MAR 1004-1011 as partial: default captured, configured value and MPW runtime remain incomplete. Implementation commit: `d2af565bc5ea5b40250057ae00cf7817bb298437`.

## M339 — Feed MPW SYSPARAM into resident INIT, 25 November 2026

Connected the native INIT MPW map request to `modified_page_fault_cluster` from the SYSPARAM defaults structure instead of repeating the numeric default in kernel startup. The configured SYSBOOT override remains unavailable, but the source-derived value now flows from the native parameter record to the executed resident SYS.EXE INIT phase.

Verification: `make -j2 all test` PASS (native images and full host/layout suite); `make inimpwcheck` PASS; fresh single-runner `make qemu-sysboot-m217` PASS and reached the explicit incomplete-INIT stop after initializing the MPW map. `tests/test_m47_qemu_manifest.py` and `python3 tools/validate_source_ranges.py` PASS (2,105 ranges, 448/4,566 original files indexed, zero errors; counts unchanged from M338). Capture `logs/serial/qemu-m217-run-local-20261125T034230162394Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`. No login/DCL/DECnet/SET HOST behavior occurred; QEMU exited.

Implementation commit: `f188336f72431a2c04011586c3cb6152ca5c78df`.

## M340 — Share native INIT SYSPARAM defaults, 25 November 2026

Moved the supported source default values into one native constants table and used those constants from both `port_vms37_init_param_defaults` and SYSBOOT's compiled BOO packet profile/validation. This prevents the IRP/LRP/SRP and MPW values from drifting between the INIT parameter table and boot-time sizing; the MPW request also consumes the parameter structure. The existing boot profile stays static so the 265-block stage-two limit is preserved. PARAMS.DAT overrides and other SYSPARAM consumers are still incomplete.

Verification: `make -j2 all test` PASS (native image build and host/layout suite); source-range validation PASS (2,105 ranges; 448/4,566 indexed; zero errors); `tests/test_m47_qemu_manifest.py` PASS; fresh single-runner `make qemu-sysboot-m217` PASS through existing SYS initialization and DQA0/IRQ14/Files-11 checks, then the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T034439575437Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`. No login, DCL, DECnet or SET HOST behavior occurred; QEMU exited.

Implementation commit: `01c8629d3ce44497f60c1e0ed9a9c360f1fdbb76`.

## M341 — Decode SYSBOOT PARAMS.DAT records, 25 November 2026

Added a bounded native decoder for the BOO$USEFILE record layout in SYSBOOT.MAR: 32 startup-file bytes, followed by fixed 20-byte parameter records with 16-byte name fields and 32-bit values, terminated by a zero longword. It applies the supported INIT SYSPARAM subset, uses the original SYSPARAM minimum/maximum limits, and commits values/header only after a complete valid terminator. Unknown names are skipped. The decoder is kept outside the kernel/SYS images until Files-11 parameter-file lookup and complete BOO$SEARCH descriptor handling are connected; boot still uses the defaults.

Verification: `make -j2 all test` PASS (native images and full host/layout suite); `inisysparamscheck` verifies valid rows, unknown-name skipping, bounds clamping, startup header retention, truncation and malformed-name atomic rejection. Source-range generation/validation PASS (2,107 ranges; 448/4,566 indexed; 17,255 mapped, 29,933 partial, 55,565 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded; zero errors). `tests/test_m47_qemu_manifest.py` PASS. Fresh single-runner `make qemu-sysboot-m217` PASS through existing DQA0/IRQ14/Files-11 checks, then stopped at incomplete INIT; the SYSBOOT decoder is not yet called by the guest. Transcript `logs/serial/qemu-m217-run-local-20261125T034943282147Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`. No login, DCL, DECnet or SET HOST behavior occurred; QEMU exited.

Coverage maps SYSBOOT.MAR 2421-2467 as partial; disk file open/read, full parameter descriptor search, parameter application during boot, remaining INIT dependencies and all acceptance conditions are incomplete. Implementation commit: `629d83e81f68a6f6457969be3c747fa1c204aefb`.

## M342 — Match BOO$USEFILE terminator handling, 25 November 2026

Compared the M341 SYSBOOT parameter decoder with original `BOO$USEFILE`. The original checks the zero longword at the current record pointer and stops; it does not require the rest of the READFILE buffer to be zero. Removed the stricter trailing-buffer validation and added a host regression with nonzero bytes after the terminator. This fixes the isolated decoder only; parameter-file opening/application is still not part of guest boot because BOO$USEFILE is an explicit SYSBOOT command and its complete command/descriptor path is unported.

Verification: `make -j2 all test` PASS (native images and full host/layout suite); `inisysparamscheck` verifies valid records, bound clamping, unknown-name skip, atomic truncation/malformed rejection, and acceptance of nonzero buffer tail after the source-accurate terminator. Fresh single-runner `make qemu-sysboot-m217` PASS for existing real DQA0 DDT/CRB IRQ14 and 700-byte Files-11 read across two sectors, then explicit incomplete INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T035249406694Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; runner exited and no QEMU process remains. Source-range validation PASS (2,107 ranges, 448/4,566 indexed, zero errors; 17,255 mapped, 29,933 partial, 55,565 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded). QEMU serial manifest validation and `git diff --check` PASS. The parameter decoder is not exercised by guest boot. No login, DCL, DECnet or SET HOST behavior occurred.

M342 implementation commit: `69471356e5f743c5b017baa93efe8320f47832c4`. Public SYS$QIO integration, Files-11 ACP, process-aware AST completion, scheduler/PHD/context activation, remaining INIT, login/DCL and DECnet SET HOST 0 remain incomplete.

## M343 — Read and merge named SYSBOOT parameter files, 25 November 2026

Added `port_sysboot_usefile`: resolve a named Files-11 file, follow its FH2 extension chain, read its allocation map into a caller-bounded buffer, then merge the supported INIT SYSPARAM subset using the BOO$USEFILE decoder. The API reports missing-file, bounded-buffer, I/O, and malformed-parameter failures separately; the parameter structure and startup-file output retain their prior values on a malformed list. Extended the Files-11 host fixture with a PARAMS.DAT directory entry, header, and parameter record. SYSBOOT's actual command interpreter/full BOO$SEARCH descriptor table do not invoke this helper yet; native guest boot still uses defaults.

Verification: `make -j2 all test` PASS (full host/layout suite); `make bootiocheck` PASS for directory lookup, header/FID validation, actual mapped block fetch through the host disk callback, applied MAXPROCESSCNT override, missing file, undersized buffer, and malformed-list atomicity. Source coverage generation/validation PASS (2,107 ranges; 448/4,566 indexed; zero errors; 17,255 mapped, 29,933 partial, 55,565 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded). Serial-manifest validation and `git diff --check` PASS. Fresh single-runner QEMU M217 PASS for existing DQA0 DDT/CRB IRQ14 and Files-11 700-byte cross-sector read, then explicit incomplete-INIT stop; the parameter-file helper was not called by the guest. Raw capture `logs/serial/qemu-m217-run-local-20261125T123837021857Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU runner exited, no QEMU process remains. No login, DCL, DECnet or SET HOST behavior occurred.

M343 commit: `9653bd5976086d998d614b188f6458456f686045`. The helper remains a source-matched SYSBOOT boundary, not configured boot-time SYSPARAM or public process I/O. Public SYS$QIO integration, Files-11 ACP, process-aware AST completion, scheduler/PHD/context activation, remaining INIT, login/DCL and DECnet SET HOST 0 remain incomplete.

## M344 — Resolve multiple process SYSQIO channels by assigned UCB, 25 November 2026

Replaced the one-UCB assumption in SYSASSIGN-to-SYSQIO channel resolution with an optional validated UCB-to-device/FDT binding table, retaining the single-binding compatibility path. Channel lookup now resolves the source CCB, selects that CCB's native device binding, validates the UCB window, and returns the matching device view. Per-channel IOC charging uses the same binding set and rejects absent/ambiguous bindings. Host integration assigns DQA0 and NLA0 to one process CCB table, resolves both channels to distinct device views, and charges each channel independently. NLA0 remains only the existing software DDT fixture; no NLA I/O or public SYS$QIO service dispatch is claimed.

Verification: `make dqddtcheck` PASS with address/undefined sanitizers; `make -j2 all test` PASS (full host/layout suite through M49; shell exit status 0); source generation and `python3 tools/validate_source_ranges.py` PASS (2,107 ranges; 448/4,566 originals indexed; 17,255 mapped, 29,933 partial, 55,565 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded; zero errors); `git diff --check` PASS. Fresh single-runner `make qemu-sysboot-m217` PASS through actual DQA0 DDT/CRB IRQ14 and Files-11 cross-sector read/write probes, then the explicit incomplete-INIT stop. No login, DCL, DECnet or SET HOST behavior occurred. Raw COM1 capture `logs/serial/qemu-m217-run-local-20261125T124614431909Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU runner exited.

Public diff audit for this checkpoint: only native C/header, host test, generated coverage records, source-coverage generator, QEMU serial evidence and this resume record. No original DEC source, disk image, credentials or private keys included. Public SYS$QIO dispatch, real ACP completion, AST delivery, scheduler/image activation, remaining INIT, SYSTEM login/DCL, DECnet Phase IV and nested SET HOST 0 remain incomplete.

M344 commit: 0bb5c936d2437fc0e251dc81ca595603eb51ff63. Pushed to active public repository main.

## M345 — Validate SYSQIO channel UCB/DDB association, 25 November 2026

Hardened the per-process SYSQIO binding lookup: each selected channel UCB must point to a bounded DDB in the native device database, and that DDB's first UCB must also be a bounded database record. Explicit bindings carry their expected DDB; the compatibility single-binding path derives the backlink from the UCB. Added a host case proving a DQA0 channel cannot be redirected through an NLA0 binding with a mismatched DDB. This validates native database identity only; NLA0 remains a software fixture without I/O, and public SYS$QIO dispatch remains absent.

Verification: `make dqddtcheck` PASS with address/undefined sanitizers; `make -j2 all test` PASS (full host/layout suite through M49; process exit status 0); coverage generation and `python3 tools/validate_source_ranges.py` PASS (2,107 ranges, 448/4,566 original files indexed, zero errors; counts unchanged); QEMU serial manifest verification PASS; fresh single-runner `make qemu-sysboot-m217` PASS through established DQA0 DDT/CRB IRQ14 and Files-11 reads/writes, then explicit incomplete-INIT stop. No login, DCL, DECnet or SET HOST behavior occurred. Raw COM1 transcript `logs/serial/qemu-m217-run-local-20261125T125134277374Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited.

M345 implementation commit: cac1e8b47c14c37e80de5596ee5f820986a2a32f; pushed to active public repository main.

## M346 — Bound SYSQIO UCB binding tables, 25 November 2026

Bound process SYSQIO UCB-to-device binding tables to the five UCB records present in the native DEVICEDAT database. Both QIO channel lookup and IOC charging now reject oversized binding counts before traversing caller-provided supervisor metadata. Host coverage exercises the bound and confirms rejection of an oversized table without dereferencing beyond its two-entry fixture. This is metadata validation only; public SYS$QIO, INIT completion, process activation, login/DCL and DECnet remain incomplete.

Verification: `make dqddtcheck` PASS with address/undefined sanitizers; `make -j2 all test` PASS (full host/layout suite through M49; process exit status 0); source generation and validation PASS (2,107 ranges, 448/4,566 original source files indexed, zero errors; inventory counts unchanged); fresh single-runner `make qemu-sysboot-m217` PASS through existing DQA0 DDT/CRB IRQ14 and Files-11 checks, then explicit incomplete-INIT stop. No SYSTEM login, DCL or DECnet behavior occurred. Raw COM1 transcript `logs/serial/qemu-m217-run-local-20261125T125607294875Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU runner exited.

M346 implementation commit: e50fc96d39267d1b869d5b239414653053f3cb46; pushed to active public repository main.

## M347 — Build the source-layout SYS.EXE WCB template, 25 November 2026

Corrected the live INI_SYSPCB SYSWCB overlay. The previous guest wrote the 48-byte length into offset 4 and the map count at offset 6, so its resident record was not a valid WCB. It now constructs the original 48-byte WCB header with WCB$W_SIZE at 8, DYN$C_WCB at 10, the source READ|NOTFCP|COMPLETE|CATHEDRAL mask (0x65) at 11, WCB$W_NMAP at 22, and WCB$L_STVBN=1 at 44, then compacts the actual SYS.EXE retrieval map behind it. Host tests verify these source-derived fields. Resident SYS.EXE builds the same header; kernel boot checks its DYN type, access mask and starting VBN before continuing. This is a valid SYS image mapping record, not full process paging or ACP file access.

Verification: `make inisyswcbcheck sysexecentrycheck` PASS (ASan/UBSan); first full `make -j2 all test` exposed a kernel image overrun (139,432 bytes versus 265-block/135,680-byte limit) after adding excessive inline validation. Reduced redundant boot checks while retaining guest checks for type/access/VBN; rerun `make -j2 all test` PASS (full host/layout suite through M49, exit status 0). Coverage generation and validation PASS (2,107 ranges, 448/4,566 source files indexed, zero errors; inventory counts unchanged). Fresh single-runner `make qemu-sysboot-m217` PASS: the actual guest passed the new WCB field checks, completed established DQA0 DDT/CRB IRQ14 and Files-11 probes, then reached the explicit incomplete-INIT stop. No login, DCL, DECnet or SET HOST behavior occurred. Raw COM1 transcript `logs/serial/qemu-m217-run-local-20261125T130234030043Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU runner exited.

M347 implementation commit: 554f4f0c799b6b55b1372a99c0f0ed10864c7001; pushed to active public repository main.

## M348 — Exercise VMS UAF account flags through login decoding, 25 November 2026

Removed the login test's custom `uaf_flags` callback for disabled/captive cases. The host test now mutates the VMS-format UAF$B_FLAGS byte in the on-disk account record, passes it through the existing 284-byte VMS 3.7 UAF decoder and native UAF fixture backend, then checks the real login boundary rejects DISACNT and enforces CAPTIVE CLI restrictions. This strengthens verification of original SYSDEF.MDL/LOGIN flag handling. It does not implement RMS indexed SYSUAF.DAT access; the login backend still reads the native V37UAFIX fixture, and SYSTEM login/DCL remain incomplete.

Verification: `make -j2 all test` PASS (native images and full host/layout suite; exit 0). Focused `make interactcheck v37uafcheck` PASS. Coverage generation/validation PASS (2,107 ranges; 448/4,566 originals indexed; zero errors; 17,255 mapped, 29,933 partial, 55,565 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded). QEMU serial manifest validation PASS. Fresh single-runner `make qemu-sysboot-m217` PASS through actual DQA0 DDT/CRB IRQ14, ACP SYS.EXE FCB/WCB checks and real Files-11 read/write probes, then the explicit incomplete-INIT stop. Transcript `logs/serial/qemu-m217-run-local-20261125T131124175359Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited.

One exploratory, uncommitted attempt to open `SYSUAF.DAT` from `SYSEXE.DIR` was rejected by the DQA publication phase because that path is not present in the current lab volume. The experiment was reverted; the baseline QEMU rerun above passed. No SYSUAF behavior is claimed.

Public staged-content audit: this checkpoint contains two test files and raw QEMU serial evidence/manifest only; no original DEC source, disk images, credentials, or private keys. Implementation commit: `549f794116b56613e0b5bf7a6a320e8c93714f34`.

The next login blocker is authentic SYSUAF RMS indexed-file access and account-record lookup. DQA0/public SYS$QIO process dispatch, real Files-11 ACP service integration, AST delivery, scheduler/image activation, remaining INIT, DECnet Phase IV and nested SET HOST 0 remain incomplete.

## M349 — Port UAF login hour and terminal restrictions, 25 November 2026

Added a source-matched helper for LOGIN.B32 `check_user_hours`: it reads the original fixed UAF primary/secondary hour masks, primary-day bits, operator day override, and DISNETWORK/DISDIALUP flags; it returns distinct rejection results for restricted hours, remote access, and dialup. Tests use the actual VMS 3.7 UAF fixed-record decoder and cover primary/secondary day selection, the explicit override, allowed/rejected hours, and both terminal restrictions. Coverage now indexes LOGIN.B32 lines 912-1010 as partial and updates SYSDEF.MDL UAF-layout mapping. This helper is not yet called by live LOGIN; real clock/DVI input, RMS SYSUAF access and process activation are still missing.

Verification: `make v37uafcheck` PASS; clean `make -j2 CC=gcc-12 all test` PASS (full native build and host/layout suite, exit 0; `build/kernel.bin` 135,336 bytes within 135,680-byte limit). A preceding default GCC 11 build failed because the kernel was 139,432 bytes, over the same limit; clean GCC 12 rebuild resolved the toolchain-size failure. Source coverage generation/validation PASS (2,110 ranges; 449/4,566 original files indexed; zero errors; 17,255 mapped, 30,032 partial, 56,921 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded). Serial-manifest validation PASS. Fresh single-runner `make qemu-sysboot-m217` PASS through established DQA0 DDT/CRB IRQ14 and Files-11 ACP SYS.EXE checks, then explicit incomplete-INIT stop. Captures `logs/serial/qemu-m217-run-local-20261125T131729027549Z.log` and `logs/serial/qemu-m217-run-local-20261125T131851921651Z.log` are each 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; both QEMU runners exited.

Public-content audit: only native C/header, test, coverage records, source-coverage generator and QEMU serial logs/manifest are included. No original DEC source/data, disk images, credentials or private keys are staged. Implementation commit: `1689a4bfa080971c81f8dc08ea7aafd7031fd6ec`.

Still incomplete: live SYS$QIO dispatch and process I/O, Files-11 ACP process services, AST delivery, scheduler/PHD/context activation, native INIT, authentic RMS SYSUAF lookup and LIVE LOGIN integration, DCL startup, DECnet Phase IV and genuine nested SET HOST 0.

## M350 — Apply UAF restrictions in the native console LOGIN path, 25 November 2026

The native interactive LOGIN now requires an access-policy callback after password validation. The kernel's current TTA0 console binding reads the live RTC, derives the VMS weekday/hour and applies the decoded UAF schedule using LOGIN.B32 semantics (set hour bit denies access; primary-day bit selects secondary schedule; EXE$V_EXPLICITP is not yet available). It fails closed on invalid RTC/terminal input and returns separate bad-hour, no-remote, and no-dialup conditions. Host login integration tests now exercise these outcomes through decoded VMS UAF records, in addition to DISACNT/CAPTIVE checks. This is only the native console LOGIN implementation path over the current test account backend; it does not provide RMS SYSUAF or a VMS process login.

Verification: `make interactcheck v37uafcheck` PASS; `make -j2 CC=gcc-12 all test` PASS (native image and full host/layout suite, exit 0; kernel 135,336 bytes); source coverage generation/validation PASS (2,110 ranges; 449/4,566 originals indexed; zero errors; 17,255 mapped, 30,032 partial, 56,921 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded); serial-manifest validation PASS. Fresh single-runner `make qemu-sysboot-m217` PASS through existing DQA0 DDT/CRB IRQ14 and Files-11/ACP checks, then the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T132201409677Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited. The boot did not run LOGIN.

Public-content audit: this checkpoint adds native login policy code, host tests, coverage index/generator updates and raw serial evidence only. No original DEC source/data, disk images, credentials or private keys are included. Implementation commit: `5a1cfed9adefaa0447afb5a06a9773ec3446875e`.

Still incomplete: authentic RMS SYSUAF lookup, operator day override and general GETDVI terminal classification, native process activation/login and DCL, live public SYS$QIO/ACP/AST/scheduling, complete INIT, DECnet Phase IV and real SET HOST 0.

## M352 - Files-11 ACP mapped data reads, 25 November 2026

Added an ACP VBN read operation for an accessed read-only FCB/WCB. It validates the FH2 header and file size, decodes retrieval pointers into LBNs, handles byte offsets and sector crossings, and reports partial byte counts and I/O errors. The SYS.EXE DQA0 executive probe compares a 700-byte unaligned ACP read with the independent FH2-chain bytes, requiring one header and two data-sector IOC reads. The host regression reads actual INDEXF.SYS mapped bytes from the generated disk image. This is an internal supervisor ACP path, not public SYS$QIO or process I/O. Extension-header retrieval maps and full ACP packet/AST semantics remain unimplemented.

Verification: make files1187check PASS against portdisk.img; make -j2 CC=gcc-12 all test PASS (native build and full host/layout suite, exit 0); make sourcerangecheck PASS (2,110 ranges, 449/4,566 original files indexed, zero errors; 17,255 mapped, 30,032 partial, 56,921 unresolved, 7,752 missing, 26,341 excluded, 917 hardware-excluded). QEMU M217 serial output confirmed the 700-byte ACP read through DQA0 DDT/IRQ14 matched the independent chain sample, then halted at the explicit incomplete-INIT boundary. Capture logs/serial/qemu-m217-run-local-20261125T133536079480Z.log, 5,885 bytes, SHA-256 0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb; runner exited. Local implementation commit: 7cc5beaea14cb2eb0a33894c56aa99a7ccdd1fba.

## M353 - ACP read boundary and partial-I/O regressions, 25 November 2026

Added host checks for zero-length reads, reads beyond EOF, header-read failure, and a data-sector failure after 212 bytes had completed. The M352 guest ACP probe remains compiled and exercised by the recorded QEMU serial run. This remains supervisor ACP functionality; process SYS$QIO, AST completion, and extension-header traversal are not covered.

Verification: make files1187check PASS; make -j2 CC=gcc-12 all test PASS; make sourcerangecheck PASS (2,110 ranges, zero errors); fresh single-runner make qemu-sysboot-m217 PASS with the 700-byte ACP/DQA0/IRQ14 comparison and explicit incomplete-INIT stop. Capture logs/serial/qemu-m217-run-local-20261125T134330222629Z.log, 5,885 bytes, SHA-256 0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb.

The initial GitHub API publication commit 0a967896b5a860e4b130259aa789bbb00a9e8e86 was found to contain truncated large text blobs after comparing the fetched tree with the verified local tree. The corrective M353 publication restores verified kernel.c, source-coverage.json, and resume.md content along with the ACP regressions. The corrective source/test publication has local commit af2fb1b11868cb8060cd2a9ee019c787ab91f54d and published GitHub main commit 78e34fec3fa6df96a8c332e56fbae2086eb48e2c. No invalid image or QEMU output is asserted.

Still incomplete: public SYS$QIO process dispatch, process ACP and AST completion, extension-header reads, remaining INIT, scheduler/image activation, authentic RMS SYSUAF/login, DCL, DECnet Phase IV, and genuine SET HOST 0.

## M354 - Follow Files-11 FH2 extension headers during VBN reads, 25 November 2026

The ACP VBN resolver now follows the source FH2 extension-FID chain with sequential segment checks, maps each extension FID through the active Files-11 index, and resolves VBNs in the corresponding retrieval map. Traversal is bounded to 256 headers. Host coverage builds a synthetic extension header on the generated volume, reads and compares data bytes from its mapped extent, and rejects a self-referential extension whose segment number is invalid. Process-level ACP, public SYS$QIO, AST completion, and large-file scale limits beyond the bounded chain remain incomplete.

Verification: make files1187check PASS; make -j2 CC=gcc-12 all test PASS (exit 0); make sourcerangecheck PASS (2,110 ranges, 449/4,566 original files indexed, zero errors); fresh single-runner make qemu-sysboot-m217 PASS through the existing 700-byte single-header ACP read via DQA0 DDT/IRQ14, followed by the explicit incomplete-INIT stop. QEMU does not exercise the synthetic extension header. Capture logs/serial/qemu-m217-run-local-20261125T135028615660Z.log, 5,885 bytes, SHA-256 0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb; runner exited.


M354 local implementation commit: 5b4fd33f5365e77bab30d0018ae38e100bb0a8ae; published GitHub main commit: e3996bc3915fc527ff8f304be4988460fb6f3777.
Still incomplete: public SYS$QIO process dispatch, ACP IRP/AST semantics, remaining INIT, scheduler/image activation, authentic RMS SYSUAF/login, DCL, DECnet Phase IV, and genuine SET HOST 0.

## M355 - Test Files-11 ACP reads across extension boundaries, 25 November 2026

Extended the synthetic FH2 fixture to read 64 bytes straddling the primary header's final VBN and the extension header's first VBN. The test requires separate reads through both retrieval maps and compares the returned bytes with the generated disk sectors. This covers host ACP extension-boundary resolution; QEMU SYS.EXE currently has no extension header.

Verification: make files1187check PASS; make -j2 CC=gcc-12 all test PASS (exit 0). No QEMU runner was needed for this host-test-only change. The M354 guest boot result remains recorded above. Local M355 commit: bd14d475a74ad37da74a670d0b3deaa302c6e819; published GitHub main commit: a5ad28978aa1bede9707d9129dc1aa1e73b63e02.


## M356 - Record source MAP_VBN coverage precisely, 25 November 2026

Added a source-to-native record for F11B/SRC/MAPVBN.B32. It marks native FH2 VBN selection, extension-FID reads, window map callbacks and data-sector reads as partial while keeping source window turning, remapping, retry behavior, live UCB selection and process completion explicitly unresolved. Coverage remains partial for the source routine as a whole.


Local M356 commit: e6488c14ecb618782a6bf19f98d23b756d0589a7; published GitHub main commit: a21df4cc8d231d5e55f3f06a4840246b81aa725b.
Verification: make sourcerangecheck PASS (2,116 ranges; 450/4,566 original files indexed; zero errors; 17,255 mapped, 30,163 partial, 56,921 unresolved, 7,752 missing, 26,412 excluded, 917 hardware-excluded). No runtime source changed; prior M355 host and M354 QEMU results remain recorded above.

## M357 - Enforce aggregate Files-11 file size across FH2 headers, 25 November 2026

The ACP read path now sums allocation across the bounded FH2 extension chain before validating the requested byte range. It rejects reads beyond the aggregate file size while allowing reads that span the primary and extension-header boundary. Host tests verify successful boundary reads, exact disk bytes, aggregate EOF, and malformed self-referential extension rejection. The kernel boot probe exercises the unchanged single-header SYS.EXE path; it does not exercise extension headers.

Verification: make files1187check PASS; make -j2 CC=gcc-12 all test PASS (exit 0); make sourcerangecheck PASS (2,116 ranges, 450/4,566 originals indexed, zero errors); fresh single-runner make qemu-sysboot-m217 PASS through the 700-byte ACP/DQA0/IRQ14 compare, then explicit incomplete-INIT halt. Capture logs/serial/qemu-m217-run-local-20261125T140134141905Z.log, 5,885 bytes, SHA-256 0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb; QEMU exited.


Local M357 commit: ef8d69b38110e553e293ffe990a5cfd9149104fd; published GitHub main commit: a011145d11d15e454076656a9d687d43b510db9d.
Still incomplete: live public SYS$QIO process dispatch, ACP window and AST semantics, remaining INIT, scheduler/image activation, authentic RMS SYSUAF/login, DCL, DECnet Phase IV, and genuine SET HOST 0.

## M358 - Decode extended FH2 extension FIDs correctly, 25 November 2026

Corrected F11B NEXT_HEADER parsing for extended FIDs: the extension number byte at FH2 offset 19 is no longer folded into the relative volume number at offset 18. The host ACP fixture now follows an extension whose file number uses the high extension byte, reads its mapped bytes, crosses the primary/extension boundary, checks aggregate EOF, and rejects a malformed segment cycle. This change affects extension-header resolution; regular FID behavior is unchanged.

Verification: make files1187check PASS; make -j2 CC=gcc-12 all test PASS (exit 0); make sourcerangecheck PASS (2,116 ranges, 450/4,566 originals indexed, zero errors); fresh single-runner make qemu-sysboot-m217 PASS through the existing SYS.EXE ACP read and DQA0 DDT/IRQ14 path, then explicit incomplete-INIT halt. QEMU does not use extended FIDs. Capture logs/serial/qemu-m217-run-local-20261125T140637045960Z.log, 5,885 bytes, SHA-256 0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb; runner exited.
Local M358 commit: 7dfdf135a9b4d17840c69776e71f65dd84d9e598; published GitHub main commit: 488df03aa9d5a6cdc6b6a147dbea89a0c8e8cc7a.


## M359 - Isolate extended FID parsing in the Files-11 host suite, 25 November 2026

Added a direct F11B NEXT_HEADER regression asserting that extended FID bytes preserve the low-byte relative volume and separate high-byte file-number extension. The ACP integration fixture independently exercises the same path through actual generated-volume sector reads. This is host coverage only; the M358 QEMU run does not use extended FIDs.


Local M359 commit: c338211994dcfb3156df5bf772b4989ffca83eca; published GitHub main commit: c9bd83e5c8d53438176ee03f60d6b4b0358b18d5.
Verification: make files1186check PASS; make files1187check PASS; make sourcerangecheck PASS (2,116 ranges, 450/4,566 originals indexed, zero errors). No QEMU runner was needed for this isolated host-test addition.

## M360 - Validate Files-11 window mapping bounds, 25 November 2026

Hardened the native MAP_VBN callback boundary against byte-count overflow and malformed MAP_WINDOW remainder values. The request block count is checked before multiplying to bytes; successful callbacks must report a remaining byte count within the request and aligned to 512-byte Files-11 blocks. Invalid/failing callback results return -1 and report all requested blocks unmapped. Host tests cover an unaligned remainder and UINT32_MAX count without invoking the mapper.

Verification: `make files1187check` PASS; `make sourcerangecheck` PASS (2,116 ranges; 450/4,566 original files indexed; zero validation errors; 17,255 mapped, 30,163 partial, 56,921 unresolved, 7,752 missing, 26,412 excluded, 917 hardware-excluded); `make -j2 CC=gcc-12 all test` completed with host, static-layout, and sanitizer checks PASS; fresh single-runner `make qemu-sysboot-m217` PASS through the existing SYS.EXE ACP/DQA0 IRQ14 checks, then explicit incomplete-INIT halt. Capture `logs/serial/qemu-m217-run-local-20261125T141351744857Z.log`, 5,885 bytes, SHA-256 `0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb`; QEMU runner exited and no QEMU process remains. The guest does not exercise malformed callback inputs. Still incomplete: MAP_WINDOW retry/window-turn semantics, live UCB remapping, and ACP process I/O integration.

Local M360 implementation commit: f0114f3dfe5c1df7d561906ea2a4bbdb26d9faab.

Published GitHub main implementation commit: 38742081cb12bdd1098b154615658773b6f52b8d.

## M361 - Preserve the MAP_WINDOW unmapped sentinel, 25 November 2026

The native MAP_VBN wrapper now rejects an otherwise-successful callback that returns LBN `UINT32_MAX`, preserving the original -1 no-map sentinel. Host cases verify a complete map, a failing callback, an invalid LBN sentinel, malformed remainder, and multiplication overflow; failures report the entire request as unmapped.

Verification: `make files1187check` PASS; `make sourcerangecheck` PASS (2,116 ranges; 450/4,566 original files indexed; zero errors); `make -j2 CC=gcc-12 all test` PASS (exit 0); fresh single-runner `make qemu-sysboot-m217` PASS through SYS.EXE Files-11 ACP, DQA0 DDT, IRQ14 read checks, then explicit incomplete-INIT halt. Capture `logs/serial/qemu-m217-run-local-20261125T141942138855Z.log`, 5,885 bytes, SHA-256 `0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb`; QEMU exited and no QEMU process remains. The guest does not produce the sentinel case. MAP_WINDOW retry/window-turn semantics, live UCB remapping, ACP process I/O remain incomplete.

Local M361 implementation commit: 27bf0616f70303d80d500ea14b8349a9e03b6d99.

Published GitHub main implementation commit: 4d26ef1c8a246e27fe7af505f62010b74eebd473.

## M362 - Reject wholly unmapped Files-11 windows, 25 November 2026

MAP_VBN now treats a successful MAP_WINDOW callback with every requested block still unmapped as the original no-map result (-1). It reports the full request as unmapped. The host suite distinguishes full mapping, partial mapping, all-unmapped, callback failure, invalid LBN sentinel, non-block remainder, and byte-count overflow.

Verification: `make files1187check` PASS; `make sourcerangecheck` PASS (2,116 ranges; 450/4,566 originals indexed; zero errors); fresh single-runner `make qemu-sysboot-m217` PASS through the same SYS.EXE ACP/DQA0 IRQ14 boot probe and explicit incomplete-INIT halt. QEMU capture: `logs/serial/qemu-m217-run-local-20261125T142227551232Z.log`, 5,885 bytes, SHA-256 `0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb`; runner exited. Guest does not exercise the all-unmapped callback result. Window retries/turning, live UCB remapping, process-aware ACP I/O remain incomplete.

Local M362 implementation commit: 780ac4a45019159de9a76f1c31f18f32a558eec7.

Published GitHub main implementation commit: dde0262e9922cc647d671fbe356a076ad85101a4.

## M363 - Port the Files-11 MAPVBN window-turn retry sequence, 25 November 2026

Added a native F11B MAPVBN coordinator for the original two MAP_WINDOW attempts. After a complete no-map result, it records the PMS window turn, reads the file header, calls TURN_WINDOW with VBN and STVBN, then retries mapping. It rejects missing callbacks, callback failures, invalid mappings, and a changed UCB. Host regressions verify callback order `MCRTM`, retry success/failure, no retry after first-attempt success, read-header failure, and UCB mismatch. Source coverage now records lines 183-197 as partial native retry/turn behavior; complete-file remap, live WCB callbacks, and source BADRVNWCB bugcheck semantics are still incomplete.

Verification: `make files1187check` PASS; `make sourcerangecheck` PASS (2,117 source ranges; 450/4,566 original files indexed; zero errors); `make -j2 CC=gcc-12 all test` PASS (exit 0); fresh single-runner `make qemu-sysboot-m217` PASS through existing SYS.EXE ACP/DQA0 IRQ14 checks, then explicit incomplete-INIT halt. The guest does not exercise MAP_WINDOW retry; it uses the separate resident ACP read path. Capture: `logs/serial/qemu-m217-run-local-20261125T142737975166Z.log`, 5,885 bytes, SHA-256 `0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb`; runner exited. Public SYS$QIO, live ACP process I/O, INIT continuation, login/DCL, and DECnet remain incomplete.

Local M363 implementation commit: 298066d42c30eb9b1c78340a490543a65be767fe.

Published GitHub main implementation commit: ac8cdd655cc8fe77da51f158fca5667f7105b684.

## M364 - Port IOSUBRAMS IOC$MAPVBLK logical-block mapping, 25 November 2026

Added a native WCB/FM2 mapper covering WCB validation and segment selection, retrieval-pointer traversal, one immediately contiguous extent, EOF clamping, RVN-to-UCB resolution, sequential-device mapping, original-UCB fallback and balanced IPL enter/leave callbacks. Host tests exercise mapped/unmapped extents, EOF, linked WCBs, RVN, sequential mapping, malformed WCB and callback balancing. This function is currently an isolated native routine: live UCB/WCB/FCB/RVT resolvers, executive bugcheck wiring, and DDT/CRB public I/O integration are not connected. The remaining IOSUBRAMS routines are unported.

Verification: `make iocmapvblkcheck` PASS; `make sourcerangecheck` PASS (2,121 ranges; 451/4,566 original files indexed; zero validation errors; 17,255 mapped, 30,263 partial, 56,921 unresolved, 8,032 missing, 26,438 excluded, 917 hardware-excluded); `make -j2 CC=gcc-12 all test` PASS (exit 0, native image, host, static-layout and sanitizer checks); fresh single-runner `make qemu-sysboot-m217` PASS through the existing 700-byte ACP read across IOC-selected DQA0 DDT sectors via IRQ14, then the explicit incomplete-INIT stop. The QEMU guest does not exercise IOC$MAPVBLK. Captures `logs/serial/qemu-m217-run-local-20261125T143703668577Z.log` and `logs/serial/qemu-m217-run-local-20261125T143806531085Z.log` each contain 5,885 bytes and have SHA-256 `0aa11d7d3fbd820217048d14a2b4e9358936ec15c6ab4531ce3843023dd00cfb`; both runners exited and no QEMU process remains.

Still incomplete: live IOC$MAPVBLK integration, public SYS$QIO/process I/O, Files-11 ACP packet and AST completion, native INIT, scheduling/image activation, authentic SYSTEM login/DCL, DECnet Phase IV and real SET HOST 0.

Local M364 implementation commit: aaf5e4a0a853f0d3d7d366d7dbe4a41eb94cd78e.

Published GitHub main implementation commit: 75afc4a3f720ca972eaf8f2bdcfef2189e9cacbe.

## M365 - Verify INIT SYSWCB output through IOC$MAPVBLK, 25 November 2026

Extended the IOC mapper regression to feed it the exact compact WCB record generated by `port_ini_syswcb_from_boot_map`, using multiple normalized boot extents. The mapper resolves VBN 10 through the generated second FM2 extent to LBN 142. This verifies host-side compatibility between the existing INIT map producer and the new mapping consumer; it does not wire the resident executable or real I/O path.

Verification: `make iocmapvblkcheck` PASS; `make -j2 CC=gcc-12 all test` PASS (exit 0); `make sourcerangecheck` PASS (2,121 ranges; 451/4,566 indexed originals; zero errors). Host-only integration; no QEMU runner required. Remaining live INIT/IOC integration and boot/session acceptance gaps are unchanged.

Local M365 commit: f0a4a4d5d98d256d536946058b208b85684e1c61; published GitHub main commit: d2fcb7f6d6dc7812edd60bf8922327e5d9e63756.

## M366 - Route SYS.EXE ACP data sectors through IOC$MAPVBLK, 25 November 2026

Connected the converted resident SYSWCB to the Files-11 ACP DQA0 read callback. For ACP data LBNs covered by the SYSWCB, the native path finds the corresponding SYS.EXE VBN, calls IOC$MAPVBLK under the DQA0 IPL lock, and dispatches the mapper-selected LBN through the published DQA0 DDT and IRQ14. INDEXF.SYS metadata sectors outside the SYS.EXE WCB continue through their ACP-provided physical LBNs. The guest asserts at least one data-sector mapping hit. The real SYS.EXE ACP sample matched its independently read FH2 chain.

Verification: `make -j2 CC=gcc-12 all test` PASS (exit 0); `make sourcerangecheck` PASS (2,121 ranges; 451/4,566 originals indexed; zero errors; 17,255 mapped, 30,263 partial, 56,921 unresolved, 8,032 missing, 26,438 excluded, 917 hardware-excluded); fresh single-runner `make qemu-sysboot-m217` PASS through the SYSWCB-to-IOC$MAPVBLK ACP sectors, DQA0 DDT/IRQ14 sample, async write/restore, then the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T150413292072Z.log`, 5,975 bytes, SHA-256 `edfa0c71aceb8c927480e60d42174b023f9da1d8ab274d6307876aff45b84193`; runner exited and no QEMU process remains. Earlier M366 runs failed and are retained as harness-fail evidence in the serial manifest: initial publication-prerequisite failure (`145648812112Z`), unresolved WCB extent for an INDEXF metadata sector (`150142589675Z`), and a pre-read WCB-hit assertion (`150219100580Z`, `150341778937Z`). Those cases were corrected before the passing run.

Still incomplete: general process WCB/FCB/RVT resolution, multi-volume mapping, source BUG_CHECK paths, public SYS$QIO completion/AST delivery, remaining INIT, process scheduling/image activation, SYSTEM login/DCL, DECnet Phase IV and genuine SET HOST 0.

Local M366 implementation commit: c231024803c834cb680b8a6116cd4900b8ddefa6; published GitHub main implementation commit: 2b0e54d8615809637a94c7aecbaa4a8e14cf3406.

## M367 - Independent M366 build and QEMU revalidation, 25 November 2026

No source changes. Re-ran `make -j2 CC=gcc-12 all test` and `make sourcerangecheck`; both PASS. The audit reports 2,121 ranges, 451/4,566 originals indexed, zero validation errors; 17,255 mapped, 30,263 partial, 56,921 unresolved, 8,032 missing, 26,438 excluded, and 917 hardware-excluded. Two sequential, non-overlapping `make qemu-sysboot-m217` runs PASSed the 700-byte Files-11 SYS.EXE chain read across IOC-selected DQA0 DDT sectors via IRQ14, then reached the explicit incomplete-INIT halt. Captures `logs/serial/qemu-m217-run-local-20261125T151413251291Z.log` and `logs/serial/qemu-m217-run-local-20261125T151522908674Z.log` each contain 5,975 bytes, SHA-256 `edfa0c71aceb8c927480e60d42174b023f9da1d8ab274d6307876aff45b84193`. Both runners exited; no QEMU process remains. This revalidation does not establish public SYS$QIO, process activation, login, DCL, DECnet, or SET HOST 0.

Local source commit remains `05fd5c33c2aec257dd37dc8239ae9d8d9dce53b3`; published implementation remains `2b0e54d8615809637a94c7aecbaa4a8e14cf3406` (GitHub checkpoint record `d587d4c9c9d8318656227bdd5d2650664e8194dd`).

## M368 - Return a validated IA-32 frame from SCH$SCHED, 25 November 2026

`port_v37_sched_select` now returns both the selected source-layout PCB and its mapped native restore-frame address. Before removing the PCB from the highest-priority resident COM queue, it resolves and validates the saved frame's CPL-matched code/data selectors, EFLAGS, and return stack. A malformed frame returns `PORT_V37_SCHED_CORRUPT` without dequeueing or changing the current PCB. This makes SCH$SCHED's native IRET context an explicit output; it does not yet connect SCH$RESCHED's saved trap frame, update TSS/CR3 in the live architecture trap path, or activate real VMS processes. The original SCHED.MAR source mapping now reflects this partial frame handoff and its remaining integration.

Verification: `make -j2 CC=gcc-12 v37schedcheck v37ia32ctxcheck` PASS; `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, and `python3 tests/test_coverage_ranges.py` PASS; `make -j2 CC=gcc-12 all test` PASS; `make sourcerangecheck` PASS (2,121 ranges; 451/4,566 originals indexed; zero validation errors; 17,255 mapped, 30,263 partial, 56,921 unresolved, 8,032 missing, 26,438 excluded, 917 hardware-excluded). One fresh `make qemu-sysboot-m217` PASSed the 700-byte Files-11 SYS.EXE read through IOC-selected DQA0 DDT/IRQ14, then reached the explicit incomplete-INIT halt. Capture `logs/serial/qemu-m217-run-local-20261125T152801897371Z.log`, 5,975 bytes, SHA-256 `edfa0c71aceb8c927480e60d42174b023f9da1d8ab274d6307876aff45b84193`; runner exited and no QEMU process remains. The initial SYS.EXE link exposed the new context-validator dependency; after adding the shared IA-32 context object to the SYS.EXE link, the full build and tests passed. An intermediate duplicate-object link attempt was removed before the passing build.

Local M368 implementation commit: `a0d1ca178cdec3bd3715c5984da3fc1bafa2675c`; published GitHub main implementation commit: `088874c3a6a2c0b183050940df61dc15edc72a34`.


## M369 - Generalize validated IA-32 restore-frame handoff, 25 November 2026

Replaced the probe-specific selected-frame global with arch_selected_restore_frame and added arch_prepare_restore_frame, which validates the full CPL-specific IRET frame before publishing it for the shared assembly restore path. The ring transition regression now uses this checked API. This is architecture restore plumbing only: live arch_trap scheduling still does not call port_v37_sched_select, preserve the outgoing VMS PCB context, update TSS/CR3, or activate the source NULL/SWAPPER PCBs. No increase in functional source-coverage status is claimed.

Verification: make -j2 all test PASS (host, static layout, and sanitizer checks; exit 0); python3 tools/gen_source_coverage.py, python3 tools/validate_source_ranges.py, and git diff --check PASS (2,121 ranges; 451/4,566 originals indexed; zero validation errors; totals unchanged). Two sequential make qemu-sysboot-m217 runners PASSed the real SYS.EXE Files-11 700-byte read through IOC-selected DQA0 DDT and IRQ14, then reached the explicit incomplete-INIT halt. Captures logs/serial/qemu-m217-run-local-20261125T153329750656Z.log and logs/serial/qemu-m217-run-local-20261125T153514922478Z.log each contain 5,975 bytes, SHA-256 edfa0c71aceb8c927480e60d42174b023f9da1d8ab274d6307876aff45b84193; both runners exited and no QEMU process remains. Serial manifest validation initially found three pre-existing duplicate entries; exact duplicate records were removed while retaining every unique raw log, and verification now passes.

Local M369 implementation commit: 8a763966dbe0b167ff9a69faf29e483026d57964.
Published GitHub main implementation commit: d893603f0a7561fead53677486b3a9ad72cdae62.

## M370 - Save outgoing native context in PCB/PHD, 25 November 2026

Added `port_v37_sched_save_current`: it validates the interrupted IA-32 restore frame, records its address in `PCB$L_PHYPCB`, writes native KSP/PC in the bound source PHD, and then requeues the current resident PCB. If the queue operation fails, the PCB/PHD context fields roll back. Host tests cover valid context save, malformed-frame rejection without mutation, queue-corruption rollback, and existing SCH$RESCHED/SCH$SCHED order. The live IDT/PIT trap still does not call this service or select the next frame; no process is activated.

Verification: `make v37schedcheck` PASS; full `make -j2 all test` PASS (exit 0); `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, `python3 tests/test_coverage_ranges.py`, and `git diff --check` PASS. Audit: 2,121 ranges, 451/4,566 original files indexed, zero errors; 17,255 mapped, 30,267 partial, 56,921 unresolved, 8,032 missing, 26,434 excluded, 917 hardware-excluded. After rebuilding SYS.EXE, a single-runner `make qemu-sysboot-m217` PASS confirmed the DQA0 IRQ14 boot regression still reaches the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T154755763516Z.log`, 5,975 bytes, SHA-256 `edfa0c71aceb8c927480e60d42174b023f9da1d8ab274d6307876aff45b84193`; QEMU exited and no runner remains. This does not exercise the newly added scheduler save routine, which remains unwired to the live trap path. No login/DCL/DECnet was observed.

Local M370 implementation commit: `25608c644a7fff91cc71590f342dcde3b2d43cd9`.
Published GitHub main implementation commit: `4655d3695800301059ea7dc8fb8b41c2f928869f`.

## M371 - Add interrupt-level scheduler transaction, 25 November 2026

Added `port_v37_sched_interrupt` to execute the resident scheduler's ordered interrupt operation: save/validate and requeue the current PCB context, then select and validate the next resident COM context. It returns without dequeue on context or frame errors. Host tests cover a full save/requeue/select transaction and malformed outgoing context rejection. The routine is linked in SYS.EXE but is not yet called by the live IDT/PIT path; VMS PHD/process activation, TSS/CR3 updates, and frame restoration remain incomplete.

Verification: `make v37schedcheck` PASS; `make -j2 all test` PASS (exit 0); `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, `python3 tests/test_coverage_ranges.py`, `make sourcerangecheck`, and `git diff --check` PASS. Audit remains 2,121 ranges across 451/4,566 originals, zero validation errors; 17,255 mapped, 30,267 partial, 56,921 unresolved, 8,032 missing, 26,434 excluded, 917 hardware-excluded. A single-runner `make qemu-sysboot-m217` PASS verified the rebuilt SYS.EXE DQA0/IRQ14 Files-11 read and reached the explicit incomplete-INIT stop; this guest path does not call the new scheduler routine. Capture `logs/serial/qemu-m217-run-local-20261125T155125445802Z.log`, 5,975 bytes, SHA-256 `edfa0c71aceb8c927480e60d42174b023f9da1d8ab274d6307876aff45b84193`; runner exited, no QEMU process remains. Serial manifest verification PASS (313 logs). No SYSTEM login, DCL or DECnet behavior was observed.

Local M371 implementation commit: `439b213fca92bbab70227d85bcd608898d27b3cf`.
Published GitHub main implementation commit: `d6370c91543f0923f26492fd214d2f086a466da2`.
## M372 - Invoke resident SCH$RESCHED/SCH$SCHED from IRQ0, 25 November 2026

Bound the resident SYS.EXE scheduler and interrupt transaction into the live PIT/IRQ0 path after validating both pointers are inside the mapped SYS image. IRQ0 passes the interrupted IA-32 IRET frame to port_v37_sched_interrupt; this saves/requeues a current VMS PCB context, selects the highest-priority resident COM PCB, and publishes a selected restore frame only after architecture validation. With NULL/SWAPPER PHDs absent, the source scheduler returns NO_CONTEXT without inventing an idle frame. Saturating counters provide guest evidence that IRQ0 reached this path. QEMU reports scheduler interrupts and contextless deferrals before the existing incomplete-INIT stop. No VMS process has yet been activated; TSS/CR3 switching, PHD initialization, selected-frame IRET and executive wait/wake integration remain incomplete.

The larger native kernel (139,432 bytes) exceeded the previous 265-sector stage-two limit (135,680 bytes). Expanded the bound to 273 sectors in the loader, image builder, layout tests and boot verifiers. The loader still uses single-sector CHS requests; 273 sectors end at physical 0x32200, below the 0x90000 boot-stack limit and within 1.44 MB floppy geometry.

Verification: make -j2 all test completed through host, sanitizer, layout and manifest regressions, including the M40 273-sector geometry/DMA test; make sourcerangecheck PASS (2,121 ranges, 451/4,566 original files indexed, zero validation errors; 17,255 mapped, 30,267 partial, 56,921 unresolved, 8,032 missing, 26,434 excluded, 917 hardware-excluded). A single make qemu-sysboot-m217 PASS verified the real 700-byte Files-11 ACP/DQA0 DDT/IRQ14 probe and observed live IRQ0 invoking the resident scheduler and deferring contextless NULL/SWAPPER; guest then stopped at incomplete INIT. Capture logs/serial/qemu-m217-run-local-20261125T160028460191Z.log, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6; runner exited. No login/DCL/DECnet/SET HOST sequence occurred. Public source diff contains native port/build/test code and serial evidence only; no original DEC source or disk image was staged.

Local M372 implementation commit: abcee37e4f1e57a12002941d15bb9a24657aaef1.
Published GitHub main M372 implementation commit: eaed7fcbf6d7ae1ceafcbcaba39a340ef30b8f3b.

## M373 - Assert live scheduler handoff in the QEMU regression, 25 November 2026

Strengthened the real M217 boot regression to require the live IRQ0 SCH$RESCHED/SCH$SCHED result before accepting the incomplete-INIT boundary. This prevents the serial check from passing if the resident scheduler callback is not invoked. It does not activate NULL/SWAPPER or claim process scheduling.

Verification: make qemu-sysboot-m217 PASS with the new ordered scheduler assertion; tests/test_m47_qemu_manifest.py PASS. Captured logs/serial/qemu-m217-run-local-20261125T171040327676Z.log (6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6). Runner exited; no QEMU process remains. The guest still stops at incomplete INIT; no process activation, login/DCL, DECnet or SET HOST occurred.

Local M373 implementation commit: fef0e092b663f0e49360439e9809e326585fb43e.
Published GitHub main M373 implementation commit: 8a380dae34d285304ad7126b0110cca436db3df1.


## M374 - Add source WQH wait and wake transitions, 25 November 2026

Added a SYSDEFQZ.MDL-layout WQH implementation and host-tested native scheduler transitions. SCH$WAITK saves the interrupted context to PCB/PHD, appends the current PCB with source wait state/count, clears current scheduling ownership, and selects a runnable context. The wake helper removes a validated waiter and reinserts it through the native CHSE priority/COM queue logic. Invalid WQH/list/frame inputs are rejected before mutation; host tests cover queue/state/context save, empty scheduler behavior, ready-queue wake, and corruption rejection. This is not yet called by a live process wait or I/O/AST path. WFLAND/WFLOR/WAITFR integration, wait quota/time charges, pending AST checks/delivery, source event/RPTEVT masks, PID-based WAKE/RESUME, full IPL semantics and process activation remain incomplete.

Verification: `make v37schedcheck` PASS; `make -j2 all test` PASS (exit 0); `make sourcerangecheck`, `python3 tests/test_coverage_ranges.py`, and `git diff --check` PASS. Corrected source mapping to attribute only SYSWAIT.MAR SCH$WAITK queue/context/select lines 311-327 to the partial native routine; RSE.MAR wake event tables remain unresolved because the native helper does not implement those source event actions. Coverage: 2,122 ranges, 451/4,566 originals indexed, zero missing mapped functions; 17,230 mapped, 30,267 partial, 56,921 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded. A single `make qemu-sysboot-m217` PASS verified the existing real DQA0 DDT/IRQ14/ACP and IRQ0 scheduler regression, then observed the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T172040429206Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`; runner exited and no QEMU process remains. The new wait/wake API is not exercised by the guest; no SYSTEM login, DCL, DECnet or SET HOST occurred.

Local M374 implementation commit: `58e91c4bf491ad6339c88f363cd9f777aea65251`.
Published GitHub main M374 implementation commit: `f14b04eb89091502ade02b60c7a251f69d3f9be9`.


## M375 - Distinguish committed blocked waits from retry-safe failures, 25 November 2026

SCH$WAITK already enqueues the current PCB before asking SCH$SCHED for another process. When no runnable context exists, returning the scheduler's ordinary EMPTY/NO_CONTEXT statuses was ambiguous and could make a caller retry a transition that had already committed. Added PORT_V37_SCHED_BLOCKED for that committed state and documented that it is not retry-safe. Runnable selection and other scheduler errors retain their original statuses; the host regression verifies the blocked PCB remains on the WQH with saved PHD context.

Verification: `make v37schedcheck` PASS; `make -j2 all test` PASS (exit 0); `python3 tools/gen_source_coverage.py` and `python3 tools/validate_source_ranges.py` PASS (2,122 ranges, zero validation errors; 451/4,566 original files indexed). A single `make qemu-sysboot-m217` PASS verified the DQA0 DDT/IRQ14/ACP/IRQ0 boot regression and reached the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T173001260376Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`; the runner exited. This guest path does not enter the new blocked-wait result path. No process activation, login, DCL, DECnet or SET HOST occurred.

Local M375 implementation commit: `c89f6c022b13e239cd8486410193ae5c1d109f64`.
Published GitHub main M375 implementation commit: `180f4ac816ab0ca56edb82bbe31e619363b50e07`.


## M376 - Verify ordered resident and nonresident WQH wakeups, 25 November 2026

Extended the scheduler host regression through a two-process source wait queue. A resident PCB waits behind an already blocked nonresident PCB; waking the resident tail leaves the head/count correct and returns it to COM through CHSE. Waking the nonresident head moves it to COMO and invokes the swapper wake callback. The test verifies queue links, state transitions, scheduler summary/accounting and WQH emptiness. This remains a host-tested scheduler API path, not a live VMS process or I/O completion path.

Verification: `make v37schedcheck` PASS; `make -j2 all test` PASS (exit 0); `python3 tools/validate_source_ranges.py` and `git diff --check` PASS (2,122 ranges, zero validation errors; 451/4,566 original files indexed). A single `make qemu-sysboot-m217` PASS verified the existing DQA0 DDT/IRQ14/ACP/IRQ0 boot regression and reached the explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T173716075234Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`; runner exited and no QEMU process remains. No process activation, login, DCL, DECnet or SET HOST occurred.

Local M376 implementation commit: `25cdfe1641e94074298375af63fc854a98346fbc`.
Published GitHub main M376 implementation commit: 434a386ca8b33e53f603344fe1806841104bb28a.


## M377 - Implement partial SCH$RSE wake-event dispatch, 25 November 2026

Added native state-mask dispatch for source FPGA, WAKE, RESUME and PFCOM events. Events for a PCB in an unrelated state are ignored without queue mutation; matching waiters are removed from their supplied WQH and made eligible through CHSE, including nonresident COMO/swapper wake handling. Host tests cover a PFCOM state mismatch and a matching WAKE after SCH$WAITK. Source AST and local/common event-flag completion, SETPRI/SWPOUT, PID lookup, wait-queue address selection, event-return register state and full IPL handling remain incomplete; this is not wired to live I/O/AST completion.

Verification: `make v37schedcheck` PASS; `make sourcerangecheck`, `python3 tests/test_coverage_ranges.py`, `make -j2 all test` PASS (exit 0), and `git diff --check` PASS. Coverage: 2,126 ranges, 451/4,566 originals indexed, zero validation errors; 17,230 mapped, 30,288 partial, 56,900 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded. One `make qemu-sysboot-m217` PASS reproduced the DQA0 DDT/IRQ14/ACP/IRQ0 boot regression and explicit incomplete-INIT stop. Capture `logs/serial/qemu-m217-run-local-20261125T174200593836Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`; QEMU runner exited. The new RSE routine remains host-only. No process activation, login, DCL, DECnet or SET HOST occurred.

Local M377 implementation commit: `daaa61f3183128d1b949c4fe3030e3802933c454`.
Published GitHub main M377 implementation commit: e76e10b5049fae0ab5e01ccf383e9ad96893b941.

## M378 - Complete saved LEF/CEF wait results, 25 November 2026

Extended partial SCH$RSE event handling to local-event (LEF), common-event (CEF), and nonresident local-event (LEFO) wait states. For resident LEF/CEF waiters, the native path advances saved PHD PC past CHMK, places SS$_NORMAL in saved PHD R0, then removes the process from the supplied WQH and applies source CHSE priority/ready-queue handling. PHD changes are rolled back if wake processing fails. Host regression covers CEF state matching, irrelevant-event rejection, WQH removal, saved return PC/R0 and executable transition. Event-flag bitmap evaluation/selection, common-cluster WQH resolution, LEFO syscall return state, live process waits, full IPL discipline and AST integration remain incomplete; the API is not live-wired.

Verification: `make v37schedcheck` PASS; full `make -j2 all test` PASS (exit 0); `make sourcerangecheck`, `python3 tests/test_coverage_ranges.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS. Coverage: 2,129 ranges, 451/4,566 originals indexed, zero validation errors; 17,230 mapped, 30,298 partial, 56,890 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded. No QEMU run: this change only affects an unbound scheduler API and the existing guest cannot exercise it. No process activation, login, DCL, DECnet or SET HOST occurred.

Local M378 implementation commit: `e539dd70677d9fc844af5ce5f65832b0d1fbf89c`.
Published GitHub main M378 implementation commit: `226f932c466b32b2c9403b0c78824e9f5b2118c9`.

## M379 - Verify LEF result rollback and commit, 25 November 2026

Expanded the RSE host regression to exercise source LEF completion separately from CEF. It corrupts the WQH tail after a valid LEF wait and verifies that the RSE saved-PC/R0 edits roll back without changing the blocked process; repairing the WQH allows saved syscall completion and CHSE wake to commit. This tests the existing host API and does not add live event-flag selection or scheduler integration.

Verification: `make v37schedcheck` PASS; `python3 tools/gen_source_coverage.py`, `python3 tests/test_coverage_ranges.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,129 ranges; zero validation errors; totals unchanged). No QEMU run; this test change does not affect the guest. Login/DCL/DECnet/SET HOST remain unobserved.

Local M379 test commit: `e519b6ff1bda6f35be0310646b20709227a8fb4b`.
Published GitHub M379 test commit: `9449b69b782363db78afc10a5eb9bc0dc8b5128a`.

## M380 - Validate source wait states before SCH$WAITK, 25 November 2026

The WQH validator previously accepted any nonzero state, allowing the native wait API to enqueue a process under executable or current state values. It now accepts only the contiguous SYSDEFQZ wait-state range COLPG through FPG (1-11), rejecting COM/COMO/CUR and out-of-range values before PCB/PHD mutation. Source state names are exposed in the scheduler header. The host regression verifies a COM WQH is rejected while preserving the current PCB and empty wait queue. This is validation for the scheduler API; actual wait callers, state-to-WQH resolution and live process scheduling remain incomplete.

Verification: `make v37schedcheck` PASS; full `make -j2 all test` PASS (exit 0); coverage generation, range regression, and `python3 tools/validate_source_ranges.py` PASS (2,129 ranges; 451/4,566 originals indexed; zero errors; inventory counts unchanged); `git diff --check` PASS. No QEMU run: the API remains unbound to the guest. No login/DCL/DECnet/SET HOST occurred.

Local M380 implementation commit: `6cf753854f4b7313e8426990610be09968689caf`.
Published GitHub main M380 implementation commit: `f093fd9f5b85daeb7cb34cacf02f31f76f53d7de`.

## M381 - Add partial SCH$RSE AST wake dispatch, 25 November 2026

Added the RSE AST event mask for the original eligible wait states (CEF, COLPG, FPG, HIB/HIBO, LEF/LEFO, MWAIT and PFW). A matching blocked process is moved through the existing WQH removal and CHSE path without altering its saved syscall return frame; unmatched states remain ignored. Host regression verifies MWAIT wake behavior and unchanged saved PC. AST request queueing, pending-state maintenance, AST delivery on process return, source WQH lookup, live AST integration and other RSE events remain incomplete; this only covers the scheduler wake portion.

Verification: `make v37schedcheck` and full `make -j2 all test` PASS (exit 0); coverage generation, range regression, and source-range validation PASS (2,131 ranges; 451/4,566 original files indexed; zero errors; 17,230 mapped, 30,312 partial, 56,876 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. No QEMU run because the event API remains unbound to guest processes. No AST delivery, process activation, login/DCL, DECnet or SET HOST is claimed.

Local M381 implementation commit: `bda81ffff38858be142603ac366aa1700b7a3257`.
Published GitHub main M381 implementation commit: `bd18ab3bc7c0bea200456bc5b40116e2cc54b34b`.

## M382 - Implement partial SCH$CHSEP and RSE SETPRI, 25 November 2026

Implemented native precomputed-priority queue movement for runnable resident COM and nonresident COMO PCBs. The transition validates source PCB/queue membership, removes it from its prior priority queue, updates COMQS and resident ready counts, sets the requested 0..31 priority, appends to the appropriate queue, and invokes reschedule or swapper callbacks. RSE SETPRI dispatch now accepts only COM/COMO states and routes its explicit priority to this helper; invalid priorities are rejected. Host tests cover resident and nonresident moves, invalid input, queue accounting, and swapper wake. Source CHSEP IPL bugcheck semantics remain partial; no live process invokes this event path.

Verification: `make v37schedcheck` PASS; full `make -j2 all test` PASS (exit 0); coverage generation, `tests/test_coverage_ranges.py`, and `tools/validate_source_ranges.py` PASS (2,135 ranges; 451/4,566 originals indexed; zero errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. Single `make qemu-sysboot-m217` PASS on the rebuilt image: DQA0 DDT/IRQ14/ACP path, real IRQ0 scheduler callback, then explicit incomplete-INIT stop. Raw log `logs/serial/qemu-m217-run-local-20261125T180541644356Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`; serial manifest verifies 320 logs. QEMU runner exited; no QEMU process remains. No process activation, login, DCL, DECnet or SET HOST occurred.

Local M382 implementation commit: `90345a06e102064df97836d5c6b039fb93635807`.
Published GitHub main M382 implementation commit: `d793f03ec0aee9de542c1f08bd3bdf214d04b44e`.

## M383 - Correct native LEF/CEF wait return frame, 25 November 2026

Corrected M378 saved wait completion for the actual IA-32 interrupt ABI. The x86 INT frame already carries EIP after the two-byte INT instruction; advancing PHD PC by four was incorrect and did not update the frame used by native IRET. The completion path now validates the saved IA-32 restore frame, mirrors its post-INT EIP to PHD PC, returns SS through PHD R0 and saved EAX, and rolls all three values back if WQH wake fails. Host regression verifies CEF/LEF return state and EAX rollback on corrupt WQH. Event-flag selection, IPL integration, live process wait, and scheduler binding remain incomplete.

Verification: make v37schedcheck, make -j2 all test (exit 0), coverage generation, 	ests/test_coverage_ranges.py, 	ools/validate_source_ranges.py, and git diff --check PASS. Coverage: 2,135 ranges, 451/4,566 original files indexed, zero errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded. One make qemu-sysboot-m217 PASS on rebuilt image; actual guest reached the explicit incomplete-INIT stop after DQA0/IRQ14/ACP and live IRQ0 callback. Capture logs/serial/qemu-m217-run-local-20261125T181133169793Z.log, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6; runner exited and no QEMU process remains. No process activation, login, DCL, DECnet or SET HOST occurred.

Local M383 implementation commit: 691010cd7f0f35fe8fe5efc921aa6c315ed3cfd0.

Published GitHub main M383 implementation commit: 202ef82276eeb7e22d3e77cee7905a2db9204749.
Local M383 resume commit: 5c1bac0042c7efcc755dd2b5e04ac748ee62d317.

Published GitHub main M383 full-tree commit: baab17807fa06952e727542facb69019f08d591d.
Local history-preserving merge commit: f385d5902687eeacfa00d21cd609e27433bba5ad.

## M384 - Reject malformed saved IRET frames during LEF wake, 25 November 2026

Added a scheduler regression that corrupts the saved LEF waiter CS while retaining kernel data selectors, then verifies RSE event completion rejects the malformed IA-32 IRET frame before changing PHD PC/R0, saved EAX, PCB wait state, or WQH membership. Restoring the original CS allows the existing WQH-corruption rollback and valid event completion tests to proceed. This adds validation coverage for saved-frame rejection; it does not wire wait/wake into live process execution.

Verification: make v37schedcheck, full make -j2 all test (exit 0), and git diff --check PASS. Source coverage unchanged at 2,135 ranges, 451/4,566 original files indexed, zero errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded. No QEMU run: this change only extends the host scheduler regression and does not change the guest image. No process activation, login/DCL, DECnet, or SET HOST is claimed.

Local M384 test commit: 977ccfa1573cc07bf8eb08bb416764d4692d22a6.

Published GitHub main M384 test commit: 237c288f22ce2475f3e4945d08ab5ef14c75f9cb.

## M385 - Source-layout SYSWAIT event-flag waits, 25 November 2026

Implemented native scheduler primitives for SYSWAIT WFLOR/WFLAND and POSTEF over the original PCB local EFCS/EFCU, common CEB EFC/WQH, EFWM, WEFC, and WALL fields. Supports immediate completion and resident local/master-common waiters, queue insertion, POSTEF bit setting, and wake completion into the saved IA-32 IRET frame. Slave CEB replication, public service dispatch/PID lookup, nonresident wake, AST/IPL integration, and live guest binding remain incomplete.

Verification: `make -j2 all test` exit 0; focused `make v37schedcheck v37eventwaitcheck` PASS; source coverage generator PASS (2,135 ranges, 0 missing mapped functions); `git diff --check` PASS. One `make qemu-sysboot-m217` PASS. Actual guest log `logs/serial/qemu-m217-run-local-20261125T184320914562Z.log`, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6; reaches DQA0 IRQ14, Files-11 ACP, SYSWCB, and IRQ0 scheduler activity, then the explicit incomplete-INIT stop. No QEMU process remains. This run does not exercise the new event wait code. No process activation, SYSTEM login, DCL, DECnet, or SET HOST is claimed.

Local M385 implementation commit: 8ecb92fe49b8e3b026b7eb0f5d6d0dcaf5885905.
Published GitHub main M385 implementation commit: f298b2d482c6f075a3f0d1d910e892267d2b8762.

## M386 - Wake nonresident CEF waiters through CHSE, 25 November 2026

Matched original RSE.MAR CEF behavior: resident CEF waiters receive SS$_NORMAL in their saved native return context; nonresident CEF waiters take EVENTE/CHSE to COMO without modifying unavailable process return state. POSTEF now evaluates both resident and nonresident CEF waiters. The host regression makes a WFLAND waiter nonresident before the satisfying event and verifies COMO wake, empty CEB WQH, and unchanged saved PC/R0/EAX. Slave CEB interprocessor request/replication and live guest event-service dispatch remain incomplete.

Verification: focused `make v37schedcheck v37eventwaitcheck` PASS; full `make -j2 all test` exit 0; `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,135 ranges, 451/4,566 original files indexed, zero validation errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). One `make qemu-sysboot-m217` PASS; log `logs/serial/qemu-m217-run-local-20261125T185204880022Z.log`, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6. Guest reaches the existing DQA0/IRQ14/ACP and IRQ0 scheduler checkpoints, then explicit incomplete-INIT stop; it does not exercise CEF wake. No QEMU process remains. No login/DCL/DECnet/SET HOST is claimed.

Local M386 implementation commit: 25bac35d928cbd7fa1b1f2f6fba925951d1955f8.
Published GitHub main M386 implementation commit: 08283fb15dac8650bea246795f5c15bc136dbbeb.

## M387 - Keep unsupported slave CEB operations unresolved, 25 November 2026

Separated an actually unassigned common event cluster from a nonzero but unresolved CEB address and from an assigned shared-memory slave CEB. Only a zero CEB pointer returns SS$_UNASEFC. Slave CEB wait/post now returns the internal unresolved status without setting a misleading VMS service status. Host regression verifies both APIs leave the caller's service-status output unchanged for a slave CEB and still report SS$_UNASEFC for an unassigned cluster.

Verification: `make v37eventwaitcheck` PASS after the final change; `make v37schedcheck v37eventwaitcheck` PASS; full `make -j2 all test` exit 0; coverage generation and source-range validation PASS (2,135 ranges, zero mapped-function misses, zero validation errors); `git diff --check` PASS. One `make qemu-sysboot-m217` PASS on rebuilt SYS.EXE. Actual log `logs/serial/qemu-m217-run-local-20261125T185532302311Z.log`, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6; expected DQA0/IRQ14/ACP and IRQ0 checkpoints then incomplete INIT. The guest does not invoke event services. No QEMU process remains. No process activation, login/DCL, DECnet or SET HOST is claimed.

Local M387 implementation commit: d4e37539129670b6ebbeb6d32017c7de2f9d3413.
Published GitHub main M387 implementation commit: 77138de66942acaf15e67f396b2148b115d4e750.

## M388 - Serialize WFLOR/WFLAND and POSTEF transactions, 25 November 2026

Added scheduler synchronization callbacks for the native event-flag APIs. WFLOR/WFLAND and POSTEF now require an enter/leave pair and hold the source IPL_SYNCH equivalent across event-flag checks, PCB wait-mask updates, WQH changes, CEF scans and wake transitions. SYS.EXE supplies a UP IA-32 callback that masks local interrupts and restores the caller's prior IF state. Host tests verify balanced enter/leave calls; scheduler-only users retain their existing caller-held synchronization contract. This is a UP interrupt-mask translation, not VAX IPL emulation or SMP locking, and the public event service dispatcher remains unbound.

Verification: focused `make v37schedcheck v37eventwaitcheck` PASS; full `make -j2 all test` exit 0; coverage generation and source-range validation PASS (2,135 ranges, 451/4,566 original files indexed, zero validation errors); `git diff --check` PASS. One `make qemu-sysboot-m217` PASS. Guest log `logs/serial/qemu-m217-run-local-20261125T190030458606Z.log`, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6; it verifies existing DQA0/IRQ14/ACP and IRQ0 checkpoints, then reaches incomplete INIT. The guest does not exercise WFLOR/WFLAND/POSTEF. No QEMU process remains. No login/DCL/DECnet/SET HOST is claimed.

Local M388 implementation commit: 2f743eda14fe4c306691b9dc304e9eb2950b7a0e.
Published GitHub main M388 implementation commit: 383c755e89bdcae383ea5a19163803b3bfb81738.

## M389 - Validate CEB size before event service access, 25 November 2026

The native event service now requires a resolved CEB to declare at least 32 bytes, enough to contain the source EFC and full WQH fields, before it reads those records. A zero pointer remains SS$_UNASEFC; an invalid nonzero pointer remains an internal unresolved status; truncated source records return corruption without mutating event flags or service status. Host regression verifies the short-CEB rejection. CEB shared-memory master/slave protocol and public service binding remain unresolved.

Verification: `make v37schedcheck v37eventwaitcheck` PASS; full `make -j2 all test` exit 0; coverage generation and `python3 tools/validate_source_ranges.py` PASS (2,135 ranges, zero mapped-function misses or validation errors); `git diff --check` PASS. One `make qemu-sysboot-m217` PASS. Log `logs/serial/qemu-m217-run-local-20261125T190547568006Z.log`, 6,148 bytes, SHA-256 94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6; reaches established DQA0/IRQ14/ACP and IRQ0 checkpoints then incomplete INIT. Guest does not call event services. No QEMU runner remains. No process activation, SYSTEM login, DCL, DECnet or SET HOST is claimed.

Local M389 implementation commit: f9ef0669cf8063c579c53a91bf396f8f140070db.
Published GitHub main M389 implementation commit: af3fe2dae4e82d4a9e7f07cdb93cc98b7d5bde84.

## M390 - Clear rejected SYS$QIO packet outputs, 25 November 2026

`port_v37_sysqio_prepare_irp` now clears packet/VA outputs on non-WAIT begin failures and IRP reservation failures. A stale caller value could otherwise appear to own an IRP after a failed request. The source channel-window WAIT result keeps its verified no-op output contract. Host regression checks that IVCHAN clears poisoned outputs and reports `SS$_IVCHAN`. This is a SYS$QIO preparation helper, not a live system-service dispatch or DQA0 driver submission.

Verification: focused `make sysqioargscheck` PASS; full `make -j2 all test` PASS (exit 0); generated coverage and `python3 tools/validate_source_ranges.py` PASS (2,135 ranges; 451/4,566 original files indexed; zero validation errors); `git diff --check` PASS. No QEMU run: this helper regression is host-only and no new guest syscall path was introduced. No SYS$QIO, process activation, SYSTEM login, DCL, DECnet or SET HOST guest execution is claimed.

Local M390 implementation commit: `b0e8cd3f029839451c5fcf4fca77bd01a79b8b94`.
Published GitHub main M390 implementation commit: `ad6ec34c76a54ecbe65173d6dfed8afeef249acb`.

## M391 - Recheck the published SYS image boot, 25 November 2026

Rebuilt M390 SYS.EXE completed the existing QEMU M217 integration regression: 700 SYS.EXE bytes were read across two IOC-selected DQA0 DDT sectors through real ATA IRQ14. The guest still stops at the known incomplete INIT boundary; no process activation, SYS$QIO call, login, DCL, DECnet or SET HOST was observed. This run does not exercise the M390 failure-output change.

Verification: `make qemu-sysboot-m217` PASS; serial manifest verification PASS (327 raw logs). Capture `logs/serial/qemu-m217-run-local-20261125T191808093864Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`; runner exited and no QEMU remains. No additional functional coverage is claimed.

Local M391 checkpoint commit: `3d89d0b8679e595896ae69f94d0efb608c7f4923`.
Published GitHub main M391 checkpoint commit: `4b5fa20070cce150afbec40977309fc31a2d190d`.

## M392 - Clear SYS$QIO outputs for invalid reservation inputs, 25 November 2026

`port_v37_sysqio_prepare_irp` now clears packet and virtual-address outputs when mandatory preparation inputs (other than the output pointers) are invalid. This keeps rejected requests from exposing stale caller values while preserving the verified WAIT no-op contract. Host regression supplies a null IRP specification and checks cleared outputs and unchanged process quota/prepared state. This helper remains disconnected from public SYS$QIO dispatch and guest process I/O.

Verification: focused `make sysqioargscheck` PASS; full `make -j2 all test` PASS (exit 0); `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,135 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). No QEMU run: host helper-only behavior changed; no guest dispatch was introduced. No process activation, SYSTEM login, DCL, DECnet, or SET HOST is claimed.

Local M392 implementation commit: `21422c6340f46c1fab1e428ed5c6ef59e1def8d0`.
Published GitHub main M392 implementation commit: `a4acbb942e319c7016bda7e923e3ef34d056c7b2`.

Remaining acceptance work is unchanged: INIT has no NULL/SWAPPER PHD/context activation; public SYS$QIO and process-aware AST completion, Files-11 ACP request integration, scheduler/process image activation, genuine login and DCL, DECnet Phase IV, and real SET HOST 0 remain incomplete. Source coverage is not complete (8,032 missing and 56,877 unresolved ranges remain in the indexed inventory).

## M393 - Return SYS$QIO bad-parameter status for invalid wrapper inputs, 25 November 2026

The SYS$QIO IRP-preparation wrapper now sets the caller's service-status output to `SS$_BADPARAM` when a mandatory input is missing, if that status pointer is available. It still clears both packet outputs and returns the preparation-stage error. Host regression starts with a poisoned status and null IRP specification, then verifies deterministic `SS$_BADPARAM`, cleared packet outputs, unchanged process I/O/AST quotas, and unchanged prepared-request storage. This is wrapper validation only; no guest SYS$QIO dispatch or DQA0 process request is wired.

Verification: focused `make sysqioargscheck` PASS; full `make -j2 all test` PASS (exit 0); `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,135 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). No QEMU run: the changed path is host-tested and remains outside guest dispatch. No process activation, login, DCL, DECnet, or SET HOST is claimed.

Local M393 implementation commit: `7e497c5a0780820433fec583793e312c62fcc669`.
Published GitHub main M393 implementation commit: `0f603b7ab5f61526b00fc4c1f3da41ac13e7f004`.

## M394 - Preserve the interrupted PCB when SCH$SCHED finds no native context, 25 November 2026

Fixed the scheduler interrupt path's unhandled transition: when a running process is saved and the highest-priority ready PCB has no PHD/IA-32 restore frame, SCH$SCHED returns `NO_CONTEXT` without dequeuing that candidate. The interrupt path now restores the interrupted PCB from COM to CUR, reverses its ready-count/queue insertion, and leaves the contextless candidate queued. This keeps PCB/current-process state consistent with the IRET frame that the architecture resumes; it neither invents a context nor consumes the incomplete PCB. Host regression covers the higher-priority missing-context candidate and saturates the diagnostic counter to verify rollback still occurs at counter maximum.

Verification after the final code edit: `make v37schedcheck` PASS; full `make -j2 all test` PASS (exit 0); `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,135 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). Two sequential `make qemu-sysboot-m217` runs PASS. The final run read 700 bytes of SYS.EXE across two IOC-selected DQA0 sectors through real IRQ14, then retained the existing incomplete-INIT stop; this boot has no activated VMS current PCB and does not exercise the new rollback path. Final capture `logs/serial/qemu-m217-run-local-20261125T193725240662Z.log`: 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. Serial manifest verification PASS (329 raw logs). QEMU exited; no runner remains. No process activation, SYSTEM login, DCL, DECnet, or SET HOST is claimed.

Local M394 implementation commit: `f16e5766a8978cfc78e54bf991068643170713e2` (first capture retained in `76aaf3e7229e1d33f6f4f054a9ab7fe85aec1c18`).
Published GitHub main M394 implementation commit: `878008714f47fffcacafeb4e15099e85f908424b`.


## M395 - Validate IA-32 same-CPL VMS interrupt frames correctly, 25 November 2026

Fixed IA-32 context validation for interrupts taken while already running at VMS access modes 1-3. Such hardware frames contain EIP/CS/EFLAGS but do not contain ESP/SS; those struct slots alias interrupted stack contents. Seeded initial contexts are marked explicitly and still require a valid cross-CPL initial stack. Once a real same-CPL interrupt vector replaces the seed marker, validation checks selectors and EFLAGS without misreading arbitrary stack words as hardware-saved ESP/SS. Host regressions cover CPL1 and CPL2 same-CPL frames with deliberately non-selector shadow words, alongside the existing initial CPL1/2/3 validation. This corrects frame validation; it does not create/activate the missing resident NULL/SWAPPER contexts, nor prove the guest scheduler runs.

Verification: `make v37ia32ctxcheck` PASS; `make -j2 all test v37schedcheck` PASS (exit 0); `python3 tools/gen_source_coverage.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,135 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). Two sequential `make qemu-sysboot-m217` runs PASS; each read 700 SYS.EXE bytes across two IOC-selected DQA0 sectors through IRQ14 then stopped at the known incomplete INIT boundary. The frame-validation change is not exercised by these boots. Captures `logs/serial/qemu-m217-run-local-20261125T194420106388Z.log` and `logs/serial/qemu-m217-run-local-20261125T194533050892Z.log`: each 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. QEMU exited; no runner remains. No public SYS$QIO execution, activated process, SYSTEM login, DCL, DECnet, or SET HOST is claimed.

Local M395 implementation commit: `dffac95d6249e45b909ad253225adb78bffb412c`.
Published GitHub main M395 implementation commit: `dffac95d6249e45b909ad253225adb78bffb412c`.

Remaining acceptance work is unchanged: INIT has no NULL/SWAPPER PHD/context activation; public SYS$QIO and process-aware AST completion, Files-11 ACP request integration, scheduler/process image activation, genuine login and DCL, DECnet Phase IV, and real SET HOST 0 remain incomplete. Source coverage is not complete (8,032 missing and 56,877 unresolved ranges remain in the indexed inventory).


## M396 - Preserve privilege-transition stack validation, 25 November 2026

Reviewed the M395 same-CPL assumption against the installed IA-32 IDT: interrupt gates target selector 0x08 (CPL0). Therefore an interrupt from VMS access modes CPL1-3 crosses to CPL0 and hardware saves the interrupted ESP/SS for IRET. Reverted the attempted relaxation that accepted arbitrary stack words for those modes. Added regressions requiring valid saved stack selectors for CPL1/CPL2 interrupt frames, preserving the required return-stack validation. M395 is superseded on this point; its prior code was invalid and is no longer present at HEAD.

Verification: focused `make v37ia32ctxcheck v37schedcheck` PASS; full `make -j2 all test v37schedcheck` PASS (exit 0); source coverage generation and `python3 tools/validate_source_ranges.py` PASS (2,135 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. One `make qemu-sysboot-m217` PASS; the guest completed the established DQA0 IRQ14 Files-11 read, then stopped at incomplete INIT. This context validation is not exercised by the boot. Capture `logs/serial/qemu-m217-run-local-20261125T195041925807Z.log`: 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. No QEMU runner remains. No process activation, SYSTEM login, DCL, DECnet, or SET HOST is claimed.

M396 correction implementation commit: `0f0962a221040a5b921ed6793300ca6ba3bc0f34`.
Published GitHub main M396 correction commit: `0f0962a221040a5b921ed6793300ca6ba3bc0f34`.


## M397 - Switch selected VMS access-mode interrupt stacks, 25 November 2026

The native resident scheduler handoff now validates a selected IA-32 frame and sets TSS ESP0 to the end of that process's kernel-stack IRET image for VMS access modes 1-3. The installed IDT gates target selector 0x08, so those privilege transitions re-enter CPL0 on the selected process stack. The CPL1/CPL2 hardware probe now selects a ring-2 frame, verifies the next interrupt frame landed at that frame's exact stack top, then restores the monitor TSS stack before ordinary boot. Per-process CR3 switching and actual NULL/SWAPPER PHD/context activation remain absent; the boot has not scheduled a resident VMS PCB.

Verification: full `make -j2 all test v37schedcheck` PASS (exit 0); coverage regeneration and validation PASS (2,135 ranges, zero errors); `git diff --check` PASS. One QEMU M217 attempt exposed that the diagnostic privilege probe left TSS ESP0 on its temporary stack; capture `logs/serial/qemu-m217-run-local-20261125T195449703160Z.log`, 1,006 bytes, SHA-256 `c13dbf231bdc3be4ccdef43e67152103098370fc26cc8a28082ad163dab94534`, records that failure. After restoring the monitor stack on probe exit, the sequential QEMU M217 run PASSed the real CPL1/CPL2 transition, DQA0 IRQ14 Files-11 read, and stopped at the existing incomplete INIT boundary. Capture `logs/serial/qemu-m217-run-local-20261125T195503943880Z.log`, 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. The scheduler-selected VMS PCB path itself remains unexercised because its contexts are absent. No QEMU runner remains.

M397 implementation commit: `54e4da94bc0e576bb9f8508e0dcf83eba365d092`.
Published GitHub main M397 implementation commit: `54e4da94bc0e576bb9f8508e0dcf83eba365d092`.


## M398 - Bound the selected IA-32 kernel-stack top, 25 November 2026

Added checked 32-bit extent arithmetic before the native scheduler writes TSS ESP0. On the IA-32 target, uintptr_t wraps at 32 bits, so comparing an already-wrapped sum against UINT32_MAX did not reject an address-space overflow. The context helper now validates the frame address plus full 76-byte restore image before publishing the stack top. Host tests cover the largest representable valid top, overflow, and addresses outside IA-32. This protects the M397 TSS handoff; no process contexts are activated yet.

Verification: `make v37ia32ctxcheck all` PASS; full `make -j2 test v37schedcheck` PASS (exit 0); coverage generation and validation PASS (2,135 ranges, zero errors); `git diff --check` PASS. One `make qemu-sysboot-m217` PASS, including the actual CPL1/CPL2 hardware stack probe and DQA0 IRQ14 Files-11 read, then the existing incomplete INIT stop. The overflow edge remains host-tested; the real ring-transition path executed. Capture `logs/serial/qemu-m217-run-local-20261125T195951193235Z.log`: 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. No QEMU runner remains.

M398 implementation commit: `c4180b15fc48fb02c7daab720f99d503080298c4`.
Published GitHub main M398 implementation commit: `c4180b15fc48fb02c7daab720f99d503080298c4`.


## M399 - Preserve the VMS PAGEDYN system parameter, 25 November 2026

Added the original 80,000-byte PAGEDYN default from SYSPARAM.MAR to the native INIT parameter set and taught SYSBOOT PARAMS.DAT merging to recognize PAGEDYN with the original 8,192-byte minimum. The host test checks the default, case-insensitive file override, and value preservation. This only makes parameter parsing faithful; paged pool placement, mapping, allocation and SWAPPER consumption remain unimplemented.

Verification: focused `make inisysparamscheck` PASS; full `make -j2 all test` completed successfully after fixing the test fixture terminator allocation exposed by its larger record; coverage generation/validation PASS (2,135 ranges; zero errors; 17,230 mapped, 30,311 partial, 56,877 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. No QEMU run: parameter decoding only changed and paged-pool settings are not consumed in the guest.

M399 implementation commit: `166ada0127757470697b5c227490d70af2130a3b`.
Published GitHub main M399 implementation commit: `166ada0127757470697b5c227490d70af2130a3b`.

No process activation, public SYS$QIO, SYSTEM login, DCL, DECnet, or SET HOST is claimed. Acceptance work remains incomplete.


## M400 - Record PAGEDYN source coverage precisely, 25 November 2026

Updated the source-coverage generator to map the original SYSPARAM.MAR PAGEDYN declaration (default 80,000 bytes, minimum 8,192) to the native default table and PARAMS.DAT decoder. The integration note now distinguishes decoder support from boot-time lookup/application, which remains absent. No new runtime behavior is claimed.

Verification: `make inisysparamscheck`, `python3 tools/gen_source_coverage.py`, `python3 tests/test_coverage_ranges.py`, and `python3 tools/validate_source_ranges.py` PASS (2,137 ranges; 451/4,566 indexed original files; zero errors; 17,230 mapped, 30,321 partial, 56,867 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. No QEMU run: coverage metadata changed only.

M400 coverage commit: `66144222eb4b938d86b1c99fd090cc6b0059cf16`.
Published GitHub main M400 coverage commit: `66144222eb4b938d86b1c99fd090cc6b0059cf16`.

PAGEDYN placement/allocation and boot parameter-file lookup/application remain incomplete. INIT still does not activate resident contexts; public SYS$QIO, process ACP/AST completion, SYSTEM login, DCL, DECnet Phase IV and SET HOST 0 remain incomplete.


## M401 - Add fixed resident paged-pool allocation boundary, 25 November 2026

Added native fixed-pool wrappers for the source EXE$ALOPAGED/EXE$DEAPAGED 16-byte rounding, allocation and free-list return behavior, using the already tested source-derived first-fit/coalescing helper. Linked the routines into resident SYS.EXE and added host regressions for alignment, rounding, exhaustion, duplicate return, coalescing and IPL restoration. This is only an initialized fixed resident pool boundary: it does not set up or place the actual paged pool, wait on SCH$LOCKW, extend backing, wake PGDYN waiters, decode original IRPs, or connect SWAPPER/INIT callers. Coverage is partial, not complete.

Verification: `make exepagedpoolcheck exenppoolcheck all` PASS; full `make -j2 all test` exited 0; `python3 tools/gen_source_coverage.py`, `python3 tests/test_coverage_ranges.py`, `python3 tools/validate_source_ranges.py`, and `git diff --check` PASS (2,140 source ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,306 partial, 56,882 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). `make qemu-sysboot-m217` PASS: actual DQA0 DDT/IRQ14/Files-11 ACP checks completed, then live IRQ0 called the scheduler and the guest stopped at the known incomplete INIT boundary because NULL/SWAPPER PHD contexts are absent. Capture `logs/serial/qemu-m217-run-local-20261125T201359381164Z.log`: 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. The full test target also produced a sequential identical M217 capture `logs/serial/qemu-m217-run-local-20261125T201525874716Z.log` (6,148 bytes, same SHA-256); both are recorded in the serial manifest. New routines are linked but are not called by the guest. Serial checks do not assert INIT completion. No process activation, SYSTEM login, DCL, DECnet, or SET HOST is claimed. QEMU exited and no runner remains.

Local M401 implementation commit: `24eac3940f0a9f95f39a13eee4233813c5413d47`.
Published GitHub main M401 implementation commit: `24eac3940f0a9f95f39a13eee4233813c5413d47`.

Remaining acceptance work is unchanged: INIT lacks NULL/SWAPPER PHD/context activation; public SYS$QIO and process-aware AST completion, Files-11 ACP request integration, scheduler/process image activation, genuine login and DCL, DECnet Phase IV, and real SET HOST 0 remain incomplete. Source coverage remains incomplete (8,032 missing and 56,882 unresolved ranges in the indexed inventory).


## M402 - Place INIT allocations inside the source NPAGEDYN interval, 25 November 2026

Corrected SYSBOOT/SYS.EXE pool geometry: INIT now passes the configured default NPAGEDYN byte count into the source packet layout, validates the contiguous resident interval below SRP/IRP/LRP, and starts all lock database, process-vector, PHV/swap, MPW, page-file-vector, logical-name and variable nonpaged-pool allocations at that interval's base. Previously those structures started at the beginning of the 96 KiB backing array and could consume bytes below the source interval. Virtual NPAGEDYN equals its resident size; page-table mapping, PAGEDYN placement, PARAMS.DAT boot invocation, pool extension/waits, and SWAPPER use remain incomplete. This fixes placement inside the native backing range, not full source INIT or process activation.

Verification: `make -j2 all` PASS; `make -j2 all test` exited 0; `python3 tools/gen_source_coverage.py`, `python3 tests/test_coverage_ranges.py`, and `python3 tools/validate_source_ranges.py` PASS (2,140 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,306 partial, 56,882 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). Two sequential `make qemu-sysboot-m217` runs passed the real IA-32 DQA0 DDT/IRQ14/Files-11 ACP boot checks, then stopped at the established INIT boundary: live IRQ0 entered scheduler code but NULL/SWAPPER contexts remain absent. Captures: `logs/serial/qemu-m217-run-local-20261125T231838592265Z.log` (6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`) and `logs/serial/qemu-m217-run-local-20261125T232028724463Z.log` (6,146 bytes, SHA-256 `d8e6c3342d8f507030236d245e1e9be430f47fd00260294bfe86bb581b0a7cba`). The QEMU runner exited; none remains. No login, DCL, DECnet, or SET HOST success is claimed.

Local M402 implementation commit: `0d0a49f35ec072eaa7f95a079d51007acdc45fce`.
Published GitHub main M402 implementation commit: `0d0a49f35ec072eaa7f95a079d51007acdc45fce`.

Remaining acceptance work: parameter-file discovery/application; mapped physical/nonpaged and paged pools; SWAPPER/NULL PHD and IA-32 contexts; public SYS$QIO with DQA0 request dispatch and process-aware AST completion; Files-11 ACP integration; scheduler and process image activation; SYSTEM login, DCL, DECnet Phase IV, and genuine SET HOST 0. Source coverage remains incomplete (8,032 missing and 56,882 unresolved ranges in the indexed inventory).


## M403 - Verify INIT dynamic records lie within NPAGEDYN, 25 November 2026

The SYS.EXE lookaside initialization now returns the exact NPAGEDYN base/size through its private boot result. The IA-32 boot validates that interval lies inside the loaded SYS.EXE image, matches the configured NPAGEDYN size, and contains the actual INI_LCKIDTBL, resource hash, process bitmap, process vector, PHV/swap map, MPW, and page-file vector arrays. Checks use each source structure's actual payload length; they would halt boot on an out-of-range table. This verifies memory placement only. It does not install physical page mappings/SPTs, initialize PAGEDYN, activate process contexts, dispatch public SYS$QIO, or complete INIT.

Verification: full `make -j2 all test` PASS (exit 0); after the final assertion/helper edit, `make -j2 all` and `make qemu-sysboot-m217` PASS. Three sequential M217 QEMU captures were recorded; final capture `logs/serial/qemu-m217-run-local-20261125T232909987054Z.log` is 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. It confirms the DQA0 DDT/IRQ14/Files-11 ACP read and scheduler IRQ, then the same honest stop because NULL/SWAPPER contexts are absent. Source coverage generator, range tests and source validator PASS (2,140 ranges; 451/4,566 original files indexed; 17,230 mapped, 30,306 partial, 56,882 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded; zero validation errors). The runner exited; no QEMU remains.

Local M403 implementation commit: `3b41871c35316aae84d8954a67a45741a4210207`.
Published GitHub main M403 implementation commit: `3b41871c35316aae84d8954a67a45741a4210207`.

Remaining acceptance work is unchanged: real page mapping and paged pool setup; SWAPPER/NULL PHD and IA-32 context activation; public SYS$QIO with process I/O and AST completion; Files-11 ACP integration; scheduler and process image activation; SYSTEM login, DCL, DECnet Phase IV, and genuine SET HOST 0. Source coverage remains incomplete (8,032 missing and 56,882 unresolved ranges in the indexed inventory).


## M404 - Initialize the resident PAGEDYN pool in SYS.EXE, 25 November 2026

Expanded the SYS.EXE resident backing range and passed SYSPARAM's default PAGEDYN size into the source SYSBOOT layout. During the original PAGDYN-before-IRP initialization order, SYS.EXE now initializes the 16-byte free-list allocator over the contiguous resident PAGEDYN interval immediately below NPAGEDYN. The boot result checks the exact rounded PAGEDYN size and adjacency to NPAGEDYN; a new host regression covers the complete default packet/NPAGEDYN/PAGEDYN geometry and is part of `make test`. This implements only the resident, non-POOLPGING storage path. PFN/SPT mapping/accounting, POOLPGING, allocation call sites, waits, extension and wakeups remain incomplete.

Verification: `make pktlayoutcheck` PASS; full `make -j2 all test` exited 0; `python3 tools/gen_source_coverage.py`, `python3 tests/test_coverage_ranges.py`, and `python3 tools/validate_source_ranges.py` PASS (2,142 ranges; 451/4,566 original files indexed; zero errors; 17,230 mapped, 30,322 partial, 56,866 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). `make qemu-sysboot-m217` PASS with the expanded actual SYS.EXE BSS and both resident pool intervals; real DQA0 DDT/IRQ14/Files-11 ACP checks and IRQ0 scheduler ran before the known stop for absent NULL/SWAPPER contexts. Capture `logs/serial/qemu-m217-run-local-20261125T233202982595Z.log`: 6,148 bytes, SHA-256 `94c3adc412fe755dcd753e37ab22fc08f7c71a3e87ce0ff9a6bd5190a5baf7e6`. `readelf -l build/sys.exe` shows the writable SYS segment at `0x00347000`, MemSiz `0x38200`, ending at `0x0037f200`, within the reserved `0x00380000` boundary (3,584 bytes of remaining aperture). QEMU exited; none remains. No login, DCL, DECnet or SET HOST is claimed.

Local M404 implementation commit: `202848e5fc976c34a0a876bd25b5cc19d38708a6`.
Published GitHub main M404 implementation commit: `202848e5fc976c34a0a876bd25b5cc19d38708a6`.

Remaining acceptance work: paged pool PFN/SPT semantics and callers; SWAPPER/NULL PHD and IA-32 context activation; public SYS$QIO and process-aware AST completion; Files-11 ACP request integration; process image activation; SYSTEM login, DCL, DECnet Phase IV, and genuine SET HOST 0. Source coverage remains incomplete (8,032 missing and 56,866 unresolved ranges in the indexed inventory).

## M405 - Exercise resident PAGEDYN allocation entries, 25 November 2026

SYS.EXE now exports the native EXE$ALOPAGED/DEAPAGED-equivalent function pointers through its resident INIT result. The IA-32 guest calls these exact entries for a 17-byte allocation, verifies the 32-byte rounded block lies within PAGEDYN, returns it, and verifies duplicate return is rejected. Added the source coverage mapping, actual serial capture, and explicit kernel header dependency. This is still a bootstrap exercise; no process/runtime consumer or wait/extension behavior is integrated.

Verification: `make -j2 all test` PASS (exit 0); `python3 tests/test_coverage_ranges.py` PASS; `python3 tools/validate_source_ranges.py` PASS (2,142 ranges; 451/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,322 partial, 56,866 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. `make qemu-sysboot-m217` PASS with the actual resident allocation/free/duplicate-free check and existing DQA0 DDT/IRQ14/Files-11 checks. It then stopped at the known INIT boundary because NULL/SWAPPER PHD contexts are absent. Capture `logs/serial/qemu-m217-run-local-20261125T233857124292Z.log`: 6,221 bytes, SHA-256 `7507346168119513dccfd3c0424616d067f739e5e03a4f3c72354a8246e08cd5`. QEMU exited; no runner remains. No login, DCL, DECnet, or SET HOST completion is claimed.

Local M405 implementation commit: `8daf3198dda7b80fd22e7b22974d9d6a687da433`.
Published GitHub main M405 implementation commit: `8daf3198dda7b80fd22e7b22974d9d6a687da433`.

Remaining acceptance work: paging PFN/SPT semantics, POOLPGING and runtime pool callers; SWAPPER/NULL PHD and IA-32 context activation; public SYS$QIO with DQA0 dispatch and process-aware AST completion; Files-11 ACP request integration; scheduler and process image activation; SYSTEM login, DCL, DECnet Phase IV, genuine SET HOST 0, second login/DCL and logout. Source coverage remains incomplete (8,032 missing and 56,866 unresolved ranges in indexed inventory).

## M406 - Decode original logical-name hash parameters, 26 November 2026

Added SYSPARAM defaults for LOGSHASHTBL (128 SYSTEM entries), LOGGHASHTBL (32 GROUP entries), and LOGPHASHTBL (128 PROCESS entries), and added all three names to the bounded SYSBOOT PARAMS.DAT decoder with the source range 1–16,383. Source coverage now maps their original SYSPARAM declarations. This establishes parameter values only; boot-time parameter-file lookup/application and SWAPINIT's allocation/linkage of the three tables remain unimplemented.

Verification: focused `make inisysparamscheck && ./build/inisysparamscheck` PASS; `make -j2 all` PASS; full `make -j2 test` exit 0; coverage generator, overlap/range regression, and source validator PASS (2,146 ranges, 451/4,566 original files indexed, zero errors; 17,230 mapped, 30,361 partial, 56,827 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded); `git diff --check` PASS. No QEMU run: this change affects SYSPARAM decoding only and is not yet consumed by boot. No DEC source or binary image was added.

Local M406 implementation commit: `5c81ea6e7e7e369b9697e98d6f29ffbfedc4ba4c`.
Published GitHub main M406 implementation commit: `5c81ea6e7e7e369b9697e98d6f29ffbfedc4ba4c`.

Remaining work includes SYSBOOT's optional USE PARAMS.DAT path; SWAPINIT allocation and activation of logical-name tables and process creation; NULL/SWAPPER PHD/native contexts; public SYS$QIO to DQA0, Files-11 ACP dispatch, process AST completion, scheduler and image activation; SYSTEM login, DCL, DECnet Phase IV and genuine SET HOST 0 with second-session LOGOUT verification. Source inventory remains incomplete (8,032 missing and 56,827 unresolved ranges among indexed files).

## M407 - Port SWAPINIT logical hash allocation, 26 November 2026

Added the resident SYS.EXE SWAPPER.MAR initial hash-table stage. It rounds SYSTEM, GROUP and PROCESS entry counts to powers of two, retains each exponent, and allocates/zeros the source-sized SYSTEM and GROUP RSHT records from the resident PAGEDYN pool. PROCESS table geometry is retained for per-process table creation. The entry is returned through SYS.EXE's INIT result; INIT verifies it is resident but does not call it because source ordering defers it until SWAPPER activation. SYS$DISK/SYS$SYSDISK insertion, TT0 creation, SWAPPER activation, and paging remain incomplete.

Verification: `make -j2 all test` PASS (exit 0); `make swapinithashcheck all` PASS (including source size boundaries, record layout, duplicate-init rejection, allocation failure rollback and coalescing); coverage generation, range regression and source validator PASS (2,152 ranges; 452/4,566 original files indexed; zero errors; 17,230 mapped, 30,392 partial, 58,479 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). `make qemu-sysboot-m217` PASS: the real guest confirms the resident SWAPPER hash entry is available, then stops at the known INIT boundary because NULL/SWAPPER PHD contexts are absent. Latest capture `logs/serial/qemu-m217-run-local-20261125T235713032875Z.log`: 6,309 bytes, SHA-256 `cb40dfb6648fba662a0bd053ead6d3274642b6b09c7cca5b38d27db7e5f0ba53`. Two preceding M217 captures are also recorded in the manifest. QEMU exited; no runner remains. Serial logs preserve raw CRLF bytes; the source/document diff check passes when those captures are excluded.

Local M407 implementation commit: `c26ed2d655c058424d71a5ca604d411fd2df54b8`.
Published GitHub main M407 implementation commit: `c26ed2d655c058424d71a5ca604d411fd2df54b8`.

Remaining work: call SWAPINIT only after native SWAPPER context activation; insert initial logical names and create TT0; complete PHD/native context creation and scheduling; public SYS$QIO with actual DQA0 dispatch, Files-11 ACP request handling and process AST completion; image activation, SYSTEM login/DCL, DECnet Phase IV, genuine SET HOST 0, second login/DCL and logout. Source coverage is incomplete (8,032 missing and 58,479 unresolved ranges in the indexed files).

## M408 - Link SWAPINIT SYSTEM boot names into RSHT storage, 26 November 2026

Extended the source-aligned SWAPINIT hash operation to link SYS$DISK and SYS$SYSDEVICE into the allocated SYSTEM RSHT using LOG$HASH bucket selection and sorted forward/back links. SYS.EXE copies the two initialized source LOG records into PAGEDYN before linking so the existing bootstrap logical-name table retains its links; owned copies and hash storage are released together on failure/teardown. Hash support accepts the source table exponent through 14. This is callable SWAPINIT preparation only: no SWAPPER process context invokes it yet, and general LOG$INSLOGN locking/supersession, TT0 creation and the SWAPPER loop remain absent.

Verification: `make swapinithashcheck logsystemcheck` PASS; full `make -j2 all test` exited 0; coverage generation, range regression, `tools/validate_source_ranges.py`, and `git diff --check` PASS (2,154 ranges; 452/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,400 partial, 58,471 unresolved, 8,032 missing, 26,459 excluded, 917 hardware-excluded). `make qemu-sysboot-m217` PASS: real DQA0 DDT/IRQ14/Files-11 ACP checks and scheduler entry completed, then INIT stopped because NULL/SWAPPER PHD contexts remain absent. Latest raw capture `logs/serial/qemu-m217-run-local-20261126T001111625987Z.log`: 6,309 bytes, SHA-256 `cb40dfb6648fba662a0bd053ead6d3274642b6b09c7cca5b38d27db7e5f0ba53`; four sequential captures are in the manifest. QEMU exited and no runner remains. Changed files were audited for configured secret patterns and original DEC source extensions; no matches/source files were found. No binary image is included. Push is pending: `git push origin main` cannot authenticate because no GitHub HTTPS username/credential is configured; verified remote `main` remains M407 `fb0b9d59d7109bad04db9f94721f9b2f8258815c`.

Local M408 implementation commit: 207db93b6a6cb967ebb6821b2a6e27a4dc55c6af.
Published GitHub main M408 implementation commit: pending push.

Remaining work: invoke SWAPINIT only after native SWAPPER activation; create TT0; complete NULL/SWAPPER PHD and native contexts; connect public SYS$QIO, DQA0 IOC and process-aware AST completion; integrate Files-11 ACP requests; scheduler and image activation; genuine SYSTEM login/DCL; DECnet Phase IV and genuine SET HOST 0, second login/DCL and LOGOUT. Source inventory remains incomplete (8,032 missing and 58,471 unresolved ranges among indexed files).

## M409 - Bind initial native frames through VMS scheduler, 26 November 2026

Added resident `port_v37_sched_start_context`: it accepts an unbound source PCB, a zeroed PHD, a validated resident IA-32 restore frame and PHD mapping parameters; builds the SYSDEFMP.MDL GENPHD fields, binds PCB$L_PHD/PHYPCB, and makes the process executable through source SCH$CHSE queueing. Invalid page-table parameters are rejected before mutation. This is a functional activation primitive, not full INIT integration: INIT still does not create mapped NULL/SWAPPER frames or call it, and no process context is activated in the guest.

Verification: `make v37schedcheck` PASS (including rejected PHD mapping, PHD/PCB binding, CHSE, selection of the exact restore frame); full `make -j2 all test` completed without compiler/test errors; coverage generation, range regression and source validator PASS (2,156 ranges; 452/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,396 partial, 58,471 unresolved, 8,032 missing, 26,463 excluded, 917 hardware-excluded). `make qemu-sysboot-m217` PASS for the unchanged real DQA0 DDT/IRQ14/Files-11 ACP path; actual boot still stops at INIT with NULL/SWAPPER PHD contexts absent. Capture `logs/serial/qemu-m217-run-local-20261126T002435041636Z.log`: 6,309 bytes, SHA-256 `cb40dfb6648fba662a0bd053ead6d3274642b6b09c7cca5b38d27db7e5f0ba53`. `readelf -l build/sys.exe` confirms the RW segment ends at `0x0037f200`, inside the reserved `0x00380000` boundary. QEMU exited; no runner remains. Changed paths were scanned for configured secret patterns and restricted DEC source extensions; no matches/source files found. No binary image was committed.

Local M409 implementation commit: `9620add557d3372b527e96c3fe8ab4abfc59c802`.
Published GitHub main M409 implementation commit: pending push; origin remains M407 `fb0b9d59d7109bad04db9f94721f9b2f8258815c` because this environment has no GitHub HTTPS credential.

Remaining work: wire initial mapped NULL/SWAPPER PHD/context creation to this activation primitive; activate SWAPPER and call SWAPINIT, create TT0; connect public SYS$QIO to DQA0 IOC with real process AST completion; integrate Files-11 ACP requests; complete process image activation, SYSTEM login/DCL, DECnet Phase IV, real SET HOST 0, second login/DCL and LOGOUT. Source inventory remains incomplete (8,032 missing and 58,471 unresolved ranges among indexed files).

## M410 - Preserve source PCB hardware address in native scheduling, 26 November 2026

Corrected the scheduler context model against INIT.MAR 1017-1026, SCHED.MAR 96, SWAPPER.MAR 1129, and SYSDEFMP.MDL: PCB names the physical hardware PCB, not the IA-32 restore frame. Initial activation now accepts the hardware PCB address separately. SCH, save/reschedule, wait, and resident event completion resolve the native IA-32 restore frame from the PHD KSP while leaving PCB unchanged. This fixes a source ABI violation in M409's activation helper; it does not create the NULL/SWAPPER hardware PCB mappings or integrate initial activation into INIT.

Verification: make v37schedcheck PASS; full make -j2 all test exited 0; coverage generation, source-range regression, source validator, and git diff --check PASS (2,156 ranges; 452/4,566 original files indexed; zero validation errors; 17,230 mapped, 30,396 partial, 58,471 unresolved, 8,032 missing, 26,463 excluded, 917 hardware-excluded). make qemu-sysboot-m217 PASS: real DQA0/IRQ14/Files-11 path runs, then actual guest stops because NULL/SWAPPER PHD contexts are absent. Capture logs/serial/qemu-m217-run-local-20261126T003243500207Z.log, 6,309 bytes, SHA-256 cb40dfb6648fba662a0bd053ead6d3274642b6b09c7cca5b38d27db7e5f0ba53; runner exited. No binary image or DEC source added.

Local M410 implementation commit: d023408c3a99d08c6c927f78e0c49bccf459d3e1.
Published GitHub main M410 implementation: d023408c3a99d08c6c927f78e0c49bccf459d3e1; resume verification commit 7a31d78 and publication checkpoint 575a45 were pushed.

Remaining work: create real source-mapped hardware PCB and per-process page/PHD contexts for NULL and SWAPPER, activate them in source INIT order, then execute SWAPINIT and build TT0. Public SYS dispatch and process-aware AST/Files-11 ACP path remain incomplete, as do image activation, SYSTEM login, DCL, DECnet Phase IV and end-to-end SET HOST 0/second-session/LOGOUT. Source inventory remains incomplete (8,032 missing and 58,471 unresolved ranges among indexed files).
