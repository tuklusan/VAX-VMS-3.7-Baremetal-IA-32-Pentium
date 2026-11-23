#!/usr/bin/env python3
"""Compatibility entry point for the current M196 native boot regression."""
from qemu_sysboot_m196 import main
if __name__ == "__main__":
    raise SystemExit(main())
