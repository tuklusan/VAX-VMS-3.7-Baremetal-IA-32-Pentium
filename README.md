# VAX/VMS 3.7 native IA-32 port — M192

This repository contains a native bare-metal IA-32 port in progress. The current
resident `SYS.EXE` reaches early `INIT` and stops because required executive,
driver, ACP, and process services are incomplete. This is not a completed VMS
system. Original DEC MAR/BLISS source is maintained separately and is not
included here.

Original source reference: `calmsacibis995/vms37-src`, revision
`2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752`.

## Build and host regression

Install GNU Make, GCC with 32-bit freestanding compilation support, GNU binutils,
NASM, and Python 3. The verified M192 build used GCC 12.3.0:

```sh
make clean
make -j2 CC=gcc-12 all
make CC=gcc-12 relocdrvcheck f11channelcheck f11ioccheck sysexecentrycheck sourcerangecheck
make CC=gcc-12 test
```

The guest test also requires `qemu-system-i386` (or `qemu-system-x86_64`):

```sh
python3 tools/qemu_sysboot_m192.py
python3 tools/checkpoint_serial_logs.py verify
```

Build outputs and disk images are generated locally and excluded from Git.
Raw COM1 test transcripts are archived under `logs/serial/` with hashes in
`manifest.json`.

## M192 boundary

The single real IA-32 QEMU run confirms the resident startup and private
owner-scoped Files-11 channel probe, including 600 bytes read from ATA. The
probe is not public `$ASSIGN`/`SYS$QIO`, a live DQA0 DDT/CRB/IRQ path, or an ACP.
The guest stops at the incomplete `INIT` boundary. SYSTEM login, DCL, DECnet
Phase IV, and genuine `SET HOST 0` remain unfinished.

See [resume.md](resume.md) and [source-coverage.json](source-coverage.json) for
the verified checkpoint, exact test results, coverage totals, and remaining
integration work.
