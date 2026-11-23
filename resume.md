# Native VAX/VMS 3.7 IA-32 recovery, updated through M262

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
