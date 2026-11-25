#!/usr/bin/env python3
"""Generate the current original-source to IA-32 function index."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parent.parent
REVISION = '2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
SECTIONS = [
    (1, 80, 'src/vms_mutex.h', 'port_mutex_init', 'declarations and history'),
    (81, 118, 'src/vms_mutex.c', 'port_mutex_rwait', 'resource sleep and wait flag'),
    (119, 166, 'src/vms_mutex.c', 'port_mutex_lockwnowait', 'write attempt without sleeping'),
    (167, 202, 'src/vms_mutex.c', 'port_mutex_iolockw', 'IO database exclusive entry'),
    (203, 249, 'src/vms_mutex.c', 'port_mutex_lockw', 'write claim, pending and queue/retry'),
    (250, 285, 'src/vms_mutex.c', 'port_mutex_iolockr', 'IO database shared entry'),
    (286, 352, 'src/vms_mutex.c', 'port_mutex_lockr', 'read claim and process waiting'),
    (353, 378, 'src/vms_mutex.c', 'port_mutex_ravail', 'resource wake and pending-bit clear'),
    (379, 407, 'src/vms_mutex.c', 'port_mutex_iounlock', 'IO database release'),
    (408, 473, 'src/vms_mutex.c', 'port_mutex_unlock', 'owner count, priority restore, wakes'),
    (474, 474, None, None, 'VAX assembler end directive; no run-time action'),
]

def line_for_symbol(path, symbol):
    content = (ROOT / path).read_text().splitlines()
    for i, line in enumerate(content, 1):
        if symbol == 'port_mmg_sys_end' and 'port_mmg_sys_end:' in line:
            return i
        if symbol in ('port_bug_debug','port_mmg_pfn_patch','port_mmg_pfn_patch_desc') and re.search(r'\b'+re.escape(symbol)+r'\b',line):
            return i
        if path == 'src/boot.s' and line.lstrip().startswith(symbol + ':'):
            return i
        if path == 'build_image.py' and re.search(r'^' + re.escape(symbol) + r'\s*=', line):
            return i
        if path == 'src/vms_v37_dyn_tags.h' and re.search(r'\b' + re.escape(symbol) + r'\s*=', line):
            return i
        if (symbol in ('port_exe_announce','port_exe_version_quadword','types','port_bug_debug','port_mmg_pfn_patch','port_mmg_pfn_patch_desc') or symbol.startswith('port_lib_ab_')) and re.search(r'\b' + re.escape(symbol) + r'\s*\[', line):
            return i
        if (symbol in ('port_vmb_arguments','port_boot_driver_record') and
            re.search(r'\bstruct\s+' + re.escape(symbol) + r'\s*\{', line)):
            return i
        if (symbol == 'boo_default_directory' and
            re.search(r'\bboo_default_directory\s*\[', line)):
            return i
        if re.search(r'\b' + re.escape(symbol) + r'\s*\(', line) or \
           (symbol == 'port_boo_parameters' and
            re.search(r'\bstruct\s+port_boo_parameters\s*\{', line)):
            return i
    return None

result = {'source': 'https://github.com/calmsacibis995/vms37-src',
          'revision': REVISION, 'coverage_scope': 'incremental; other source files remain unresolved',
          'files': []}
segments = []
missing = 0
for begin, end, path, symbol, purpose in SECTIONS:
    if path is None:
        segments.append({'original_lines': [begin, end], 'status': 'excluded',
                         'reason': purpose})
        continue
    line = line_for_symbol(path, symbol)
    if line is None:
        missing += 1
    segments.append({'original_lines': [begin, end], 'status': 'mapped' if line else 'missing',
                     'pc_file': path, 'pc_symbol': symbol,
                     'pc_line': line, 'function': purpose})
result['files'].append({'original_file': 'SYS/SRC/MUTEX.MAR',
                        'original_lines': 474, 'sections': segments,
                        'platform_changes': ['VAX IPL replaced with serialized IA-32 interrupt-gate calls',
                                             'VAX resume PC/PSL replaced with saved IA-32 trap frame',
                                             'VAX PCB and scheduler queue replaced with native PCB and wait state']})
dcl_if_sections = [
    (1, 88, None, None, 'source header, VAX declarations and service description'),
    (89, 92, 'src/vms_dcl_if.c', 'port_dcl_if', 'set NOSTAT and evaluate binary expression'),
    (93, 96, 'src/vms_dcl_if.c', 'port_dcl_if', 'check THEN at expression cursor and test truth'),
    (97, 103, 'src/vms_dcl_if.c', 'port_dcl_if', 'dispatch true THEN command'),
    (104, 110, 'src/vms_dcl_if.c', 'port_dcl_if', 'false clause returns normal status'),
    (111, 117, 'src/vms_dcl_if.c', 'port_dcl_if', 'mark missing THEN and return CLI syntax status'),
    (118, 120, None, None, 'VAX assembler end and blank line')
]
if_mappings=[]
for begin,end,path,symbol,why in dcl_if_sections:
    if path is None:
        if_mappings.append({'original_lines':[begin,end],'status':'excluded','reason':why})
    else:
        line=line_for_symbol(path,symbol)
        if_mappings.append({'original_lines':[begin,end],'status':'mapped' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':why})
        if line is None:missing+=1
result['files'].append({'original_file':'DCL/SRC/IF.MAR',
    'original_lines':120,'sections':if_mappings,
    'unresolved_integration':'DCL expression evaluation and command interpreter modules remain unfinished'})
dcl_status_sections = [
    (1, 61, None, None, 'source header and VAX declarations'),
    (62, 63, 'src/vms_dcl_status.c', 'port_dcl_set_status', 'default ON ERROR EXIT command'),
    (64, 84, None, None, 'source description and calling convention'),
    (85, 91, 'src/vms_dcl_status.c', 'port_dcl_set_status', 'ONEXIT guard, NOSTAT branch, AST disable'),
    (92, 106, 'src/vms_dcl_status.c', 'port_dcl_set_status', 'negative CLI status conversion and symbol strings'),
    (107, 112, 'src/vms_dcl_status.c', 'port_dcl_set_status', 'YLEVEL, last status and ON severity'),
    (113, 123, 'src/vms_dcl_status.c', 'port_dcl_set_status', 'flush, default EXIT, ON command and restart'),
    (124, 127, None, None, 'assembler ending and blank lines')
]
status_mappings=[]
for begin,end,path,symbol,why in dcl_status_sections:
    if path is None:
        status_mappings.append({'original_lines':[begin,end],'status':'excluded','reason':why})
    else:
        line=line_for_symbol(path,symbol)
        status_mappings.append({'original_lines':[begin,end],'status':'mapped' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':why})
        if line is None:missing+=1
result['files'].append({'original_file':'DCL/SRC/STATUS.MAR',
    'original_lines':127,'sections':status_mappings,
    'unresolved_integration':'Remaining DCL command parser, symbol table and interpreter need source equivalents'})
# The original LOGIN password hash service has two algorithms: the prior
# CRC method and the VMS 3.7 Purdy polynomial. All code sections are mapped.
hpwd_sections = [
    (1, 48, None, None, 'source header, ownership notice, and history'),
    (49, 81, 'src/vms_hpwd.h', 'port_hpwd', 'descriptor and calling convention'),
    (82, 91, 'src/vms_hpwd.c', 'hpwd_crc32', 'AUTODIN-II polynomial and CRC table'),
    (92, 97, 'src/vms_hpwd.c', 'hpwd_purdy', 'Purdy coefficient constants'),
    (98, 137, None, None, 'source service description'),
    (138, 162, 'src/vms_hpwd.c', 'port_hpwd', 'CRC/Purdy algorithm selection and output'),
    (163, 178, 'src/vms_hpwd.c', 'hpwd_collapse', 'eight-byte cycling and unsigned byte additions'),
    (179, 225, 'src/vms_hpwd.c', 'hpwd_purdy', 'six-term Purdy polynomial'),
    (226, 256, 'src/vms_hpwd.c', 'hpwd_power', 'modular exponentiation'),
    (257, 272, 'src/vms_hpwd.c', 'hpwd_purdy', 'modular input reduction'),
    (273, 309, 'src/vms_hpwd.c', 'hpwd_mul', 'modular multiply without VAX EMUL'),
    (310, 328, 'src/vms_hpwd.c', 'hpwd_mul', 'unsigned multiplication on IA-32'),
    (329, 345, 'src/vms_hpwd.c', 'hpwd_mul', 'scaled modular multiplication'),
    (346, 363, 'src/vms_hpwd.c', 'hpwd_add', 'modular addition and carry handling'),
    (364, 365, None, None, 'VAX assembly end directive and blank line'),
]
hpwd = []
for begin, end, path, symbol, purpose in hpwd_sections:
    if path is None:
        hpwd.append({'original_lines': [begin, end], 'status': 'excluded',
             