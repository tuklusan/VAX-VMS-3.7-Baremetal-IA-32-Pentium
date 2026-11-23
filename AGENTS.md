# VAX/VMS 3.7 — Native Bare-Metal IA-32 Port

## Mission

Port original VAX/VMS 3.7 to native Intel IA-32, targeting Pentium-compatible PC hardware.

This is a real operating-system port.

No VAX emulation, redesigned operating system, compatibility shell, or fabricated VMS behavior.

Preserve original VMS behavior, interfaces, privileges, executive semantics, ASTs, IPL handling, calling conventions, and boot order.

## GitHub — Mandatory

**The sole active development repository is:**

https://github.com/tuklusan/VAX-VMS-3.7-Baremetal-IA-32-Pentium.git

Default branch: main

All future engineering commits, branches, tests, CI workflows, and source coverage records belong in this repository.

Historical repository:
https://github.com/tuklusan/OpticOS

Historical branch: vms37-x86-qemu-m45-ci

The historical repository is read-only reference material. Do not commit future work there.

Never rewrite or discard useful original commit history.

## Original Source

Repository: https://github.com/calmsacibis995/vms37-src

Pinned revision:
2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752

Maintain a separate local checkout of original DEC MAR/BLISS sources.

Do not commit, upload, or redistribute original DEC source code without verified permission.

The active repository is public. Check all material for source restrictions, secrets, and redistribution rights before publishing it.

## Recovery Point

Checkpoint: M192, November 21, 2026.

The recovery package contains native IA-32 source, build files, tests, QEMU support, and an embedded resume.md.

Read resume.md before continuing.

The recovered working directory historically uses the name vms37-native-x86-m89. This does not imply the project stopped at M89.

## Current Engineering State

INCOMPLETE.

The resident SYS.EXE reaches early INIT and stops.

Private Files-11 channel operations have been tested against actual ATA reads.

These are not equivalent to completed public VMS I/O services.

The following remain incomplete:
- DQA0 IOC DDT/CRB and real interrupt completion
- Public SYS$QIO and proper process I/O
- Files-11 ACP integration
- AST semantics and process-aware completion
- Native INIT
- Scheduler and process image activation
- SYSTEM login and DCL
- DECnet Phase IV
- Genuine SET HOST 0

## Engineering Requirements

1. Map each substantive original source line to native IA-32 implementation or a justified exclusion.
2. Preserve original behavior rather than simplifying it.
3. Save source changes after each completed original source file.
4. Compile, integrate, and test changes incrementally.
5. Track complete, partial, stubbed, excluded, and missing coverage separately.
6. Do not confuse compiled symbols with functional coverage.
7. Use one QEMU runner at a time.
8. Maintain actual QEMU serial logs and regression results.
9. Never fabricate boot messages, execution results, or completed functionality.
10. Keep Git commits small, verified, and recoverable.
11. Prefer actual coding and testing over documentation or discussion.
12. Work autonomously without asking for routine approvals.

## Mandatory First Task

Before implementing new source files:

1. Verify M192 recovery integrity.
2. Inspect the existing recovered native sources.
3. Compare the recovery against historical Git commits.
4. Check for files that must not enter the public repository.
5. Rebuild the complete native image.
6. Run existing host tests.
7. Run the M192 QEMU boot.
8. Compare actual output with the checkpoint.
9. Establish a clean, verified baseline commit in the new repository.

Do not treat historical test results as proof of a successful new local build.

## Acceptance Conditions

The project is complete only when:

1. Every original source file has full functional native coverage or a justified exclusion.
2. A real IA-32 boot initializes VMS.
3. DECnet Phase IV starts.
4. SYSTEM logs in.
5. A real DCL command interpreter starts.
6. SET HOST 0 establishes an actual DECnet connection.
7. A second SYSTEM login succeeds.
8. A second DCL starts.
9. LOGOUT returns to the first DCL.

Verify the sequence end-to-end under QEMU.

## Session Continuity

At every verified checkpoint:

- Commit native source changes.
- Update resume.md.
- Record the exact Git commit SHA.
- Record original source coverage.
- Record real test results and failures.
- Identify unfinished integration work.
- Push verified, distributable changes to the new GitHub repository.

Never claim unfinished work is complete.

## Mandatory Response and Operation Rules

### Response Mode

- MINIMUM VERBOSITY.
- NO NARRATION.
- NO STATUS CHATTER unless blocked or explicitly requested.
- Prefer concrete repository actions and tool calls.
- Report only verified results, failures, and essential blockers.
- Do not explain routine engineering decisions unless asked.

### Operation Mode

- NO BUREAUCRACY.
- NO PAPERWORK.
- Work autonomously.
- Do not request approval for routine engineering tasks.
- Prioritize writing code, compiling, integrating, testing, and committing.
- Do not substitute plans, proposals, or progress reports for implementation.
- Continue with the next actionable engineering task without prompting.
- Update only the essential project records needed for recovery, source coverage, and verification.
- Never fabricate completion or skip required tests.
- Stop only for genuine blockers, safety restrictions, or tasks requiring user authorization.

