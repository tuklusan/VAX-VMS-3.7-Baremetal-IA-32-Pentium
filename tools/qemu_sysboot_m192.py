#!/usr/bin/env python3
"""Backward-compatible entry point for the current early-INIT QEMU check."""
from qemu_sysboot_m193 import main

if __name__ == '__main__':
    raise SystemExit(main())
