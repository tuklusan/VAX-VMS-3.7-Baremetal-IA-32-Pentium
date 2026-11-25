Warning: truncated output (original token count: 83826)
Total output lines: 1734

# Native VAX/VMS 3.7 IA-32 recovery, updated through M368

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

SYSQIOREQ.MAR lines 286-292 are partial coverage only. Public SYS$QIO, process page-table callback wiring, quotas, full device-access ch…53826 tokens truncated…/qemu-m217-run-local-20261125T032228272926Z.log`, 5,788 bytes, SHA-256 `fccbda36c6742e594849f10b4131c241fa8f059ee6dcb41b3af32d4bc9579232`; QEMU exited. Earlier M336 capacity-failure captures are retained in the serial manifest.

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
