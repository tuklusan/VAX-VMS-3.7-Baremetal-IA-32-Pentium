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
                     'reason': purpose})
        continue
    line = line_for_symbol(path, symbol)
    if line is None:
        missing += 1
    hpwd.append({'original_lines': [begin, end],
                 'status': 'mapped' if line else 'missing',
                 'pc_file': path, 'pc_symbol': symbol, 'pc_line': line,
                 'function': purpose})
result['files'].append({'original_file': 'LOGIN/SRC/HPWD.MAR',
                        'original_lines': 365, 'sections': hpwd,
                        'platform_changes': [
                            'VAX byte and quadword operations replaced with IA-32 C operations',
                            'VAX CRC instruction replaced with the same reversed polynomial',
                            'Purdy prime arithmetic uses bounded 64-bit modular additions'
                        ]})
# LOGIN/SRC/VALIDATE.MAR maps the indexed account lookup, retry,
# password match, privilege scope and cleanup onto native storage.
validate_sections = [
    (1, 56, None, None, 'non-executable header and history'),
    (57, 131, 'src/vms_validate.h', 'port_uaf_disk_service', 'file layout, lookup statuses and buffers'),
    (132, 188, None, None, 'source service interface description'),
    (189, 193, 'src/vms_validate.c', 'port_uaf_searchuser', 'search without password'),
    (194, 228, 'src/vms_validate.c', 'port_uaf_validate', 'validation and keyed storage setup'),
    (229, 259, 'src/vms_validate.c', 'do_search', 'authorized rights, open fallback and connect'),
    (260, 298, 'src/vms_validate.c', 'do_search', 'normalize username, reject DEFAULT and use fallback'),
    (299, 313, 'src/vms_validate.c', 'do_search', 'disconnect, close and restore privilege mask'),
    (314, 326, 'src/vms_validate.c', 'locked_get', 'retry locked records with two 75-ms waits'),
    (327, 360, 'src/vms_validate.c', 'validate_password', 'upcase bounded password, call HPWD, check hash'),
    (361, 362, None, None, 'assembler end directive and blank line'),
]
validation=[]
for begin,end,path,symbol,reason in validate_sections:
    if path is None:
        validation.append({'original_lines':[begin,end],'status':'excluded','reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        validation.append({'original_lines':[begin,end],
            'status':'mapped' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':reason})
        if line is None: missing+=1
result['files'].append({'original_file':'LOGIN/SRC/VALIDATE.MAR',
    'original_lines':362,'sections':validation,
    'platform_changes':[
        'Native on-disk fixed-record keyed index replaces the VAX RMS indexed file layout',
        'BIOS-independent ATA PIO reads replace VAX RMS get calls',
        'Caller privilege bits remain unchanged after every completed request',
        'Only system-name lookup is permitted by the disk backend'
    ],
    'unresolved_integration': 'Reading an existing VAX RMS SYSUAF.DAT file is not yet supported; native indexed account records are used' })
# The two time functions use 100-ns VMS absolute time and the original
# Wednesday 1858-11-17 epoch. The native PC RTC provides current time.
login_time_sources = [
    ('LOGIN/SRC/LIBDAYWK.MAR', 138, [
        (1, 64, None, None, 'ownership, history and non-executable setup'),
        (65, 76, 'src/vms_logintime.c', 'port_login_day_of_week', 'offset for Monday-first count'),
        (77, 115, None, None, 'calling convention and service description'),
        (116, 126, 'src/vms_logintime.c', 'port_login_day_of_week', 'arguments and system time fallback'),
        (127, 136, 'src/vms_logintime.c', 'port_login_day_of_week', 'minutes, days and weekday'),
        (137, 138, None, None, 'assembler end directive and blank line')
    ]),
    ('LOGIN/SRC/LIBHOUR.MAR', 135, [
        (1, 74, None, None, 'ownership, history and non-executable setup'),
        (75, 113, None, None, 'calling convention and service description'),
        (114, 124, 'src/vms_logintime.c', 'port_login_hour_of_day', 'arguments and system time fallback'),
        (125, 133, 'src/vms_logintime.c', 'port_login_hour_of_day', 'minutes and hour modulo 24'),
        (134, 135, None, None, 'assembler end directive and blank line')
    ])
]
for original,length,ranges in login_time_sources:
    item=[]
    for first,last,path,symbol,reason in ranges:
        if path is None:
            item.append({'original_lines':[first,last],'status':'excluded','reason':reason})
        else:
            line=line_for_symbol(path,symbol)
            item.append({'original_lines':[first,last],
                'status':'mapped' if line else 'missing','pc_file':path,
                'pc_symbol':symbol,'pc_line':line,'function':reason})
            if line is None: missing+=1
    result['files'].append({'original_file':original,'original_lines':length,
        'sections':item, 'platform_changes':[
            'VAX GETTIM uses native CMOS RTC 100-ns absolute time conversion',
            'VMS epoch and Monday-first day numbering remain unchanged',
            'Kernel code avoids IA-32 compiler 64-bit divide runtime calls'
        ]})
# LOGIN/SRC/FILEIO.B32 centralizes primary streams and record I/O.
fileio_ranges = [
    (1, 204, 'src/vms_login_fileio.h', 'vms_lgi_fileio_init',
     'stream structures, callback interfaces, process local file state'),
    (205, 314, 'src/vms_login_fileio.c', 'vms_lgi_open_input',
     'separate or shared stream, system input/command names, executive channels'),
    (315, 336, 'src/vms_login_fileio.c', 'vms_lgi_close_input',
     'release input stream without stopping a shared output stream'),
    (337, 445, 'src/vms_login_fileio.c', 'vms_lgi_open_output',
     'output creation, resolved filename, device and error stream names'),
    (446, 470, 'src/vms_login_fileio.c', 'vms_lgi_close_output',
     'flush, display and close output'),
    (471, 547, 'src/vms_login_fileio.c', 'vms_lgi_write_file',
     '128-byte record buffer, sequential read, output and close'),
    (548, 585, 'src/vms_login_fileio.c', 'vms_lgi_write_fao',
     'ASCIC control and FAO result through library callback'),
    (586, 628, 'src/vms_login_fileio.c', 'vms_lgi_write_output',
     '30-second write timer, output record, cancellation and return zero'),
    (629, 657, 'src/vms_login_fileio.c', 'vms_lgi_write_timeout',
     'cancel input channel to end blocked output'),
    (658, 691, 'src/vms_login_fileio.c', 'vms_lgi_get_input',
     'prompt, uppercase, purge, timed input and terminal NUL'),
    (692, 693, None, None, 'BLISS module close directives')
]
fileio_segs=[]
for a,b,path,symbol,why in fileio_ranges:
    if path is None:
        fileio_segs.append({'original_lines':[a,b],'status':'excluded',
                            'reason':why})
    else:
        lineno=line_for_symbol(path,symbol)
        fileio_segs.append({'original_lines':[a,b],
                            'status':'mapped' if lineno else 'missing',
                            'pc_file':path,'pc_symbol':symbol,'pc_line':lineno,
                            'function':why})
        if lineno is None: missing+=1
result['files'].append({'original_file':'LOGIN/SRC/FILEIO.B32',
                        'original_lines':693,'sections':fileio_segs,
                        'pc_driver':'src/vms_login_uart.c',
                        'unresolved_integration':
                        'Listing files need RMS; FAOL requires full formatter; '
                        'native COM1 input/output works with current process stream map'})
cmd_segments=[]
for begin,end,path,symbol,reason in [
    (1,22,None,None,'original DEC text notice'),
    (23,26,'src/vms_logincmd.c','vms_login_parse',
     'LOGIN verb recognition and module entry'),
    (27,27,'src/vms_logincmd.c','vms_login_parse',
     'username required, USERNAME prompt when missing'),
    (28,29,'src/vms_logincmd.c','vms_login_parse',
     'DISK and CLI qualifiers with required values'),
    (30,30,'src/vms_logincmd.c','vms_login_parse',
     'default COMMAND qualifier with optional value'),
    (31,31,None,None,'trailing blank')
]:
    if path is None:
        cmd_segments.append({'original_lines':[begin,end],'status':'excluded',
                             'reason':reason})
    else:
        lineno=line_for_symbol(path,symbol)
        cmd_segments.append({'original_lines':[begin,end],
                             'status':'mapped' if lineno else 'missing',
                             'pc_file':path,'pc_symbol':symbol,'pc_line':lineno,
                             'function':reason})
        if lineno is None: missing+=1
result['files'].append({'original_file':'LOGIN/SRC/LOGINCMD.CLD',
                        'original_lines':31,'sections':cmd_segments,
                        'unresolved_integration':
                        'Original DCL CLI dispatcher is still absent; native parser is callable'})
interact_sections=[]
for first,last,symbol,meaning in [
    (1,196,'vms_lgi_interact_init','session fields, external service callbacks'),
    (197,254,'vms_lgi_init_interactive','open streams, set terminal and initial user, announce, authenticate'),
    (255,388,'vms_lgi_auto_login','SYSALF terminal lookup, privilege scope and UAF validation'),
    (389,522,'vms_lgi_interactive_validation','DCL LOGIN parse, flags, disk/CLI options, password'),
    (523,652,'vms_lgi_get_password','empty hash, local echo mask, quiet read and password check'),
    (653,710,'vms_lgi_write_announcement','logical SYS$ANNOUNCE and SYS$WELCOME, @file or literal'),
    (711,788,'vms_lgi_announce','welcome version, SYS$NODE and mail count'),
    (789,816,'vms_lgi_zero_password','erase password in input storage')
]:
    line=line_for_symbol('src/vms_login_interact.c',symbol)
    interact_sections.append({'original_lines':[first,last],
        'status':'mapped' if line else 'missing',
        'pc_file':'src/vms_login_interact.c','pc_symbol':symbol,
        'pc_line':line,'function':meaning})
    if line is None:missing+=1
interact_sections.append({'original_lines':[817,818],'status':'excluded',
                          'reason':'BLISS module close directives'})
result['files'].append({'original_file':'LOGIN/SRC/INTERACT.B32',
                        'original_lines':818,'sections':interact_sections,
                        'unresolved_integration':
                        'SYSALF native indexed file backend and real DCL image startup are absent; '
                        'guest COM1 terminal and native disk UAF access execute interactive path'})
# SYSDEF.MDL defines the original 284-byte fixed UAF record fields.
# Only this range is mapped; the rest of SYSDEF.MDL is unfinished.
result['files'].append({
    'original_file':'SHRLIB/SYSDEF.MDL',
    'status':'partial; DYN type numbers and UAF layout only',
    'original_lines':6969,
    'sections':[
      {'original_lines':[1,1329], 'status':'unresolved',
       'reason':'Original system-wide definitions before the DPT and DYN layouts are unfinished'},
      {'original_lines':[1330,1445], 'status':'partial',
       'pc_file':'src/vms_v37_dyn_tags.h','pc_symbol':'PORT_V37_DYN_LOG',
       'pc_line':line_for_symbol('src/vms_v37_dyn_tags.h','PORT_V37_DYN_LOG'),
       'function':'Source-ordered 63 original DYN structure type identifiers; DPT ABI and structures are incomplete'},
      {'original_lines':[1446,2826], 'status':'unresolved',
       'reason':'Other original system-wide definitions before the IRP header are unfinished'},
      {'original_lines':[2827,2890], 'status':'partial',
       'pc_file':'src/vms_qio_irp_prepare.c','pc_symbol':'port_v37_irp_prepare',
       'pc_line':line_for_symbol('src/vms_qio_irp_prepare.c','port_v37_irp_prepare'),
       'function':'Source IRP ABI type, UCB pointer, function and media status words; other IRP fields and actual $QIO unported'},
      {'original_lines':[2891,4320], 'status':'unresolved',
       'reason':'Original system-wide definitions before the PCB layout are unfinished'},
      {'original_lines':[4321,4405], 'status':'partial',
       'pc_file':'src/vms_sysqio_pcb.c',
       'pc_symbol':'port_v37_sysqio_process_bind_pcb',
       'pc_line':line_for_symbol('src/vms_sysqio_pcb.c',
                                 'port_v37_sysqio_process_bind_pcb'),
       'function':'Bind SYSQIO process PID, AST/BIO/DIO quota words and local EFCS/EFCU to exact source PCB offsets; other PCB fields, live PHD/context and active scheduler remain incomplete'},
      {'original_lines':[4406,6142], 'status':'unresolved',
       'reason':'Original system-wide definitions after the PCB and before UAF are unfinished'},
      {'original_lines':[6143,6146], 'status':'excluded',
       'reason':'structure introduction and source headings'},
      {'original_lines':[6147,6232], 'status':'mapped',
       'pc_file':'src/vms_uaf_v37.c','pc_symbol':'vms37_uaf_decode',
       'pc_line':line_for_symbol('src/vms_uaf_v37.c','vms37_uaf_decode'),
       'function':'VMS 3.7 UAF field offsets, password, flags and salt'},
      {'original_lines':[6233,6969], 'status':'unresolved',
       'reason':'Original system-wide definitions after UAF layout are not ported'},
    ],
    'unresolved_integration':'Real RMS indexed-file access and remaining original SYSDEF definitions are not yet ported'
})
# The original DCL EXIT module has two complete command entries.
dcl_exit_map=[
    (1,73,None,None,'source banner and non-executable declarations'),
    (74,104,None,None,'EXIT entry description and calling convention'),
    (105,111,'src/vms_dcl_exit.c','port_dcl_exit',
     'preserve status and evaluate optional binary expression'),
    (112,120,'src/vms_dcl_exit.c','port_dcl_exit',
     'save $STATUS and unwind one indirect command level'),
    (121,132,'src/vms_dcl_exit.c','port_dcl_exit',
     'top-level interactive image rundown or batch abort'),
    (133,150,None,None,'WAIT interface and record comments'),
    (151,169,'src/vms_dcl_exit.c','port_dcl_wait',
     'mark operand, prepend zero-day, cancel timer, BINTIM, SETIMR and WAITFR'),
    (170,171,None,None,'VAX assembler ending and blank line'),
]
new=[]
for first,last,path,symbol,reason in dcl_exit_map:
    if path:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        new.append({'original_lines':[first,last],
                    'status':'mapped' if line else 'missing',
                    'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                    'function':reason})
    else:
        new.append({'original_lines':[first,last],
                    'status':'excluded', 'reason':reason})
result['files'].append({'original_file':'DCL/SRC/EXIT.MAR',
                        'original_lines':171,'sections':new,
                        'unresolved_integration':
                        'Other DCL command, parsing and process modules are not yet ported'})
# DCL/SRC/INQUIRE.MAR: full INQUIRE command, including option scanner,
# prompt text, answer compression, symbol type and scope.
inq_sections = [
    (1, 88, None, None, 'original source headings, history and call description'),
    (89, 115, 'src/vms_dcl_inquire.c', 'port_dcl_inquire_parse',
     'option prefix, symbol name and character checks'),
    (116, 140, 'src/vms_dcl_inquire.c', 'port_dcl_inquire_parse',
     'prompt string, paired quotes, default name and colon-space'),
    (141, 153, 'src/vms_dcl_inquire.c', 'port_dcl_inquire',
     'read terminal answer, compress, disable indirect and Control-Y, set symbol'),
    (154, 158, 'src/vms_dcl_inquire.c', 'port_dcl_inquire_parse',
     'invalid input syntax result'),
    (159, 198, 'src/vms_dcl_inquire.c', 'port_dcl_inquire_parse',
     'PUNCTUATION, NOPUNCTUATION, GLOBAL and LOCAL options'),
    (199, 200, None, None, 'assembler terminator and blank line'),
]
inq=[]
for first,last,path,symbol,reason in inq_sections:
    if path:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        inq.append({'original_lines':[first,last],
                    'status':'mapped' if line else 'missing',
                    'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                    'function':reason})
    else:
        inq.append({'original_lines':[first,last], 'status':'excluded',
                    'reason':reason})
result['files'].append({'original_file':'DCL/SRC/INQUIRE.MAR',
                        'original_lines':200,'sections':inq,
                        'unresolved_integration':
                        'General DCL command parser and symbol engine not yet ported; '
                        'native COM1 session runs this source command and local/global symbol store'})
# DCL/SRC/ON.MAR: original ON level table, handlers, and reset entries.
on_sections = [
    (1, 61, None, None, 'source history, service declarations and descriptions'),
    (62, 69, 'src/vms_dcl_on.c', 'level', 'four-byte level table, warning/error/severe/control-Y'),
    (70, 99, None, None, 'calling convention and service description'),
    (100, 125, 'src/vms_dcl_on.c', 'port_dcl_on', 'read error level from source level table'),
    (126, 145, 'src/vms_dcl_on.c', 'port_dcl_on', 'require THEN and collect command text'),
    (146, 169, 'src/vms_dcl_on.c', 'port_dcl_on', 'command-level guard, distinct handlers and stored text'),
    (170, 198, 'src/vms_dcl_on.c', 'port_dcl_on', 'separator and syntax errors; fixed resident text means allocation cannot fail'),
    (199, 219, None, None, 'ON reset call descriptions'),
    (220, 223, 'src/vms_dcl_on.c', 'port_dcl_on_ctly_reset', 'clear CONTROL Y handler'),
    (224, 236, 'src/vms_dcl_on.c', 'port_dcl_on_reset', 'free prior handler and restore ERROR level'),
    (237, 238, None, None, 'assembler ending and blank line')
]
new=[]
for first,last,path,symbol,reason in on_sections:
    if not path:
        new.append({'original_lines':[first,last],'status':'excluded','reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        new.append({'original_lines':[first,last],
                    'status':'mapped' if line else 'missing',
                    'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                    'function':reason})
result['files'].append({'original_file':'DCL/SRC/ON.MAR',
                        'original_lines':238,'sections':new,
                        'unresolved_integration':
                        'Indirect command file processing and control-Y AST delivery require other DCL modules'})
image_sections=[
    (1, 51, None, None, 'source heading and macro description'),
    (52, 94, 'src/vms_dcl_intimages.c','port_dcl_image_lookup',
     'original 43 internal image names in order, starting with index 0x80'),
    (95, 97, None, None, 'VAX macro and assembler ending')
]
new=[]
for first,last,path,symbol,reason in image_sections:
    if not path:
        new.append({'original_lines':[first,last],
                    'status':'excluded','reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        new.append({'original_lines':[first,last],
                    'status':'mapped' if line else 'missing',
                    'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                    'function':reason})
result['files'].append({'original_file':'DCL/SRC/INTIMAGES.MAR',
                        'original_lines':97,'sections':new,
                        'unresolved_integration':
                        'Other internal DCL image implementations remain unfinished; this file defines their identifiers only'})
# M110: Audit both live native AST/event modules line by line. All runtime
# code remains partial: there is no original VAX REI/PSL frame, multiprocessor
# interlock, guest scheduler, or completed signal/condition delivery.
def append_source_ranges(original, count, native, spec, unresolved):
    ranges=[]
    next_line=1
    for last, state, symbol, explanation in spec:
        assert next_line <= last <= count, (original,next_line,last)
        sec={'original_lines':[next_line,last], 'status':state}
        if state in ('partial','mapped'):
            line=line_for_symbol(native,symbol)
            if line is None:
                global missing
                missing+=1
                sec['status']='missing'
            sec.update({'pc_file':native,'pc_symbol':symbol,'pc_line':line,
                'function':explanation})
        else:
            sec['reason']=explanation
        ranges.append(sec)
        next_line=last+1
    assert next_line==count+1, (original,next_line,count)
    result['files'].append({'original_file':original,'original_lines':count,
        'sections':ranges,'unresolved_integration':unresolved})

append_source_ranges('SYS/SRC/ASTDEL.MAR',671,'src/vms_astdel.c',[
  (89,'excluded',None,'Source history, declarations and VAX PSECT directives; not executable'),
  (131,'excluded',None,'ASTDEL dispatch interface description and register contract'),
  (146,'partial','port_astdel_deliver','Current PCB and AST delivery interrupt select; guest trap frame absent'),
  (177,'partial','port_astdel_deliver','Special kernel AST eligibility and queue exit'),
  (223,'partial','port_astdel_deliver','Spurious/blocked AST exit, bit clearing and level recomputation'),
  (266,'partial','port_astdel_deliver','Normal AST priority and mode selection'),
  (343,'partial','port_astdel_deliver','Kernel AST callback dispatch; no VAX CALLS/REI/PSL ABI'),
  (426,'unresolved',None,'Normal executive/supervisor/user AST user-stack frame and VAX CHMK return paths are not implemented'),
  (454,'partial','port_astdel_queue_ipast','IPAST vector and special piggyback kernel AST enqueue'),
  (496,'excluded',None,'AST queue argument and condition descriptions'),
  (586,'partial','port_astdel_queue','Queue AST requests with eligibility; VAX interlocks/multiprocessor handoffs not yet available'),
  (602,'excluded',None,'New AST level call contract and explanatory comments'),
  (626,'partial','port_astdel_new_level','Compute AST eligibility from per-mode queues'),
  (661,'excluded',None,'Swap and remove AST interface descriptions'),
  (668,'partial','port_astdel_swap','Queue replacement/removal entry; no native VAX register ABI'),
  (671,'excluded',None,'Original assembler END and file close')
], 'No live scheduler/guest process binding, VAX AST return/user frame, IPL transition, or multiprocessor interlocks.')
append_source_ranges('SYS/SRC/POSTEF.MAR',403,'src/vms_postef.c',[
  (94,'excluded',None,'Original file history, declarations and VAX PSECT directives'),
  (135,'excluded',None,'SCH$POSTEF call contract and non-executable comments'),
  (175,'partial','port_postef_post','Post private event flags, return WASSET/WASCLR and wake eligible PCB'),
  (228,'partial','port_postef_post_mask','Common EF post and waiter wake; multiprocessor replication is host-only simulation'),
  (250,'excluded',None,'CHKWAIT entry contract and event mask description'),
  (271,'partial','port_postef_wait','Event wait-all/wait-any mask and process state update'),
  (349,'unresolved',None,'Shared-memory secondary CPU event delivery, packet and synchronization setup absent'),
  (401,'unresolved',None,'EXE$SHM_CEF_REQ VAX MPC/MA$REQUEST transport and interprocessor signaling absent'),
  (403,'excluded',None,'Original assembler END directive and trailing blank')
], 'Guest event flag system service, actual PCB scheduler wakeup, and interprocessor CEF request engine remain unresolved.')
goto_sections=[
    (1, 85, None, None, 'source banner, VAX declarations and calling convention'),
    (86, 94, 'src/vms_dcl_goto.c', 'port_dcl_goto',
     'source DCL$GOTO entry, no-status flag, random access gate'),
    (95, 117, 'src/vms_dcl_goto.c', 'port_dcl_position',
     'source positioning entry and contract'),
    (118, 138, 'src/vms_dcl_goto.c', 'port_dcl_position',
     'label lookup, pending target allocation and forward scanning'),
    (139, 149, 'src/vms_dcl_goto.c', 'port_dcl_position',
     'RFA seek and sequential access restoration'),
    (150, 159, 'src/vms_dcl_goto.c', 'port_dcl_dealgoto',
     'release pending forward target control block'),
    (160, 161, None, None, 'VAX assembler ending and blank line')]

# Each source line range uses the fixed reference revision.
new=[]
for first,last,path,symbol,reason in goto_sections:
    if path:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        new.append({'original_lines':[first,last],
                    'status':'mapped' if line else 'missing',
                    'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                    'function':reason})
    else:
        new.append({'original_lines':[first,last], 'status':'excluded', 'reason':reason})
result['files'].append({'original_file':'DCL/SRC/GOTO.MAR',
                        'original_lines':161,'sections':new,
                        'unresolved_integration':
                        'RMS indirect command file and DCL command interpreter are not yet complete'})
descr_sections=[
    (1, 80, None, None, 'original VAX source description and declarations'),
    (81, 100, 'src/vms_dcl_descr.c', 'port_dcl_getdval',
     'read next token, offset, flags, terminator, type, value and advance'),
    (101, 102, None, None, 'VAX assembler ending')]
parts=[]
for first,last,path,symbol,reason in descr_sections:
    if path:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        parts.append({'original_lines':[first,last],
                     'status':'mapped' if line else 'missing',
                     'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                     'function':reason})
    else:
        parts.append({'original_lines':[first,last], 'status':'excluded','reason':reason})
result['files'].append({'original_file':'DCL/SRC/DESCRVAL.MAR',
   'original_lines':102,'sections':parts,
   'unresolved_integration':'Original packed PTRDEF and full DCL token parser are pending'})
# Original DCL READREC receives RMS records, expands quote-delimited
# symbols, handles special tokens, unwinds EOF and reports read errors.
readrec_ranges=[
    (1,76,None,None,'source description and machine declarations'),
    (77,97,None,None,'DCL$INPUT calling description'),
    (98,158,'src/vms_dcl_readrec.c','port_dcl_input',
     'select input level, prompt, obtain record, skip GOTO scan lines'),
    (159,201,'src/vms_dcl_readrec.c','port_dcl_input',
     'substitution, VERIFY, comments, trailing space and prompt'),
    (202,287,'src/vms_dcl_readrec.c','port_dcl_input',
     'RMS$_RSA retry/cancel, EOF unstack, batch end, other read errors'),
    (288,296,'src/vms_dcl_readrec.c','port_dcl_silent_logout',
     'silent process exit with completion status'),
    (297,317,None,None,'EXPAND calling description'),
    (318,359,'src/vms_dcl_readrec.c','port_dcl_expand',
     'quoted string state, paired quote and comment scan'),
    (360,430,'src/vms_dcl_readrec.c','port_dcl_expand',
     'local/global lookup, lexical fallback, recursive expansion bound'),
    (431,449,'src/vms_dcl_readrec.c','port_dcl_expand',
     'replace source line with expanded input and reset cursor'),
    (450,469,None,None,'SPECIAL calling description'),
    (470,476,'src/vms_dcl_readrec.c','port_dcl_special',
     'dispatch special ampersand and at-sign tokens'),
    (477,494,None,None,'ampersand calling description'),
    (495,507,'src/vms_dcl_readrec.c','port_dcl_special',
     'replace ampersand symbol with its value'),
    (508,523,None,None,'indirect calling description'),
    (524,534,'src/vms_dcl_readrec.c','port_dcl_special',
     'disable indirection, push frame and resume record input'),
    (535,548,None,None,'character error calling description'),
    (549,555,'src/vms_dcl_readrec.c','port_dcl_charerror',
     'report, flush input, set status and restart'),
    (556,558,None,None,'VAX assembler terminator and blank lines')]
parts=[]
for first,last,path,symbol,reason in readrec_ranges:
    if not path:
        parts.append({'original_lines':[first,last],'status':'excluded','reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        parts.append({'original_lines':[first,last],
                      'status':'mapped' if line else 'missing',
                      'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                      'function':reason})
result['files'].append({
    'original_file':'DCL/SRC/READREC.MAR','original_lines':558,'sections':parts,
    'unresolved_integration':
    'RMS file lookup and arbitrary VMS procedure files require other source modules; '
    'COM1 and lab IDE sector records currently provide the real record source.'})
# DCL startup: original INITIAL.MAR services run in native process space.
# The VAX call-frame and supervisor stack instructions become a C entry.
initial_ranges = [
    (1,101,None,None,'copyright notice, history and non-executable comments'),
    (102,115,'src/vms_dcl_initial.h','port_dcl_initial_start','PPD/PRC/RMS/indirect/spawn data declarations'),
    (116,124,'src/vms_dcl_initial.c','port_dcl_initial_start','SYS$ERROR logical and table setup'),
    (125,133,'src/vms_dcl_initial.c','port_dcl_initial_set_status','permanent $STATUS and $SEVERITY symbols'),
    (134,149,'src/vms_dcl_initial.c','port_dcl_initial_start','native entry instead of a VAX supervisor call frame'),
    (150,176,'src/vms_dcl_initial.c','port_dcl_initial_start','process data, input channel, mode, AST and table pointer'),
    (177,199,'src/vms_dcl_initial.c','port_dcl_initial_start','ON ERROR defaults and local/global/label symbol lists'),
    (200,219,'src/vms_dcl_initial.c','port_dcl_initial_start','permanent symbol allocation and pointers'),
    (220,252,'src/vms_dcl_initial.c','port_dcl_initial_start','batch P1-P8 process logical lookup and hex FF null'),
    (253,303,'src/vms_dcl_initial.c','port_dcl_initial_start','FAB/NAM/RAB structures, shared streams and prompt flags'),
    (304,332,'src/vms_dcl_initial.c','port_dcl_initial_start','nine zeroed 112-byte indirect frames and initial I/O state'),
    (333,359,'src/vms_dcl_initial.c','port_dcl_initial_start','PROC# startup files in descending order'),
    (360,365,'src/vms_dcl_initial.c','port_dcl_initial_start','LOGIN process rundown'),
    (366,400,'src/vms_dcl_initial.c','port_dcl_initial_start','parent PID, supervisor SYS$ERROR mailbox validation'),
    (401,425,'src/vms_dcl_initial.c','port_dcl_initial_start','table versions 2-5, Control-Y mask and native entry restart'),
    (426,435,'src/vms_dcl_initial.c','startup_fail','error handling and process abort'),
    (436,455,None,None,'spawn interface description'),
    (456,499,'src/vms_dcl_initial.c','port_dcl_initial_start','context mailbox read, sender verification and close'),
    (500,526,'src/vms_dcl_initial.c','port_dcl_initial_spawn_record','privilege header and context flags'),
    (527,542,'src/vms_dcl_initial.c','port_dcl_initial_spawn_record','pending command copy and flag'),
    (543,563,'src/vms_dcl_initial.c','port_dcl_initial_spawn_record','access-mode checked process logical definition'),
    (564,597,'src/vms_dcl_initial.c','port_dcl_initial_spawn_record','global/local string, permanent and binary symbols'),
    (598,617,'src/vms_dcl_initial.c','port_dcl_get_prc','CLI$GET_PRC state pointer'),
    (618,619,None,None,'assembler end')
]
initial_parts=[]
for a,b,path,symbol,reason in initial_ranges:
    if path is None:
        initial_parts.append({'original_lines':[a,b],'status':'excluded','reason':reason})
    else:
        lineno=line_for_symbol(path,symbol)
        if not lineno:missing+=1
        initial_parts.append({'original_lines':[a,b],
              'status':'mapped' if lineno else 'missing',
              'pc_file':path,'pc_symbol':symbol,'pc_line':lineno,'function':reason})
result['files'].append({'original_file':'DCL/SRC/INITIAL.MAR',
    'original_lines':619,'sections':initial_parts,
    'platform_changes':['VAX call frames replaced by direct native C calls',
                        'VAX Control-Y setup uses IA-32 AST process mask',
                        'RMS record blocks keep their source-defined lengths and streams'],
    'unresolved_integration': 'A real supervisor mailbox and SPAWN caller must supply context records. Native DCL startup is invoked at real COM1 login.'})
# DCL/SRC/COMMAND.MAR owns command setup, parsing, image dispatch and restart.
# The native interpreter calls the same source-defined services through ops.
command_ranges = [
    (1,143,None,None,'source history, ownership notice and description'),
    (144,164,'src/vms_dcl_command.h','port_dcl_cmd_start','source work fields and two command prompts'),
    (165,184,None,None,'command processing call description'),
    (185,189,'src/vms_dcl_command.c','port_dcl_command_run','restart and input cursor'),
    (190,211,None,None,'processing entry description'),
    (212,244,'src/vms_dcl_command.c','port_dcl_cmd_start','reset parse work, prompt, AST state and result array'),
    (245,273,'src/vms_dcl_command.c','port_dcl_command_run','Control-Y pending, pending command and chain selection'),
    (274,300,'src/vms_dcl_command.c','process','hangup, null lines, leading dollar, first nonblank'),
    (301,314,'src/vms_dcl_command.c','token_name','first symbol, verb or label'),
    (315,353,'src/vms_dcl_command.c','process','label tables, GOTO target and indirect level checks'),
    (354,369,'src/vms_dcl_command.c','parse_assign','string symbol equal statement'),
    (370,391,'src/vms_dcl_command.c','parse_assign','substring and arithmetic symbol assignments'),
    (392,423,'src/vms_dcl_command.c','process','command symbol expansion, comments and indirect invocation'),
    (424,459,'src/vms_dcl_command.c','procedure','foreign and indirect file command routing'),
    (460,485,'src/vms_dcl_command.c','process','native verb table lookup and image type'),
    (486,515,'src/vms_dcl_command.c','option','slash option, keyword and value routing'),
    (516,586,'src/vms_dcl_command.c','parameters','parameter delimiters, lists, joining and count limits'),
    (587,624,'src/vms_dcl_command.c','parameters','required input and extra parameter prompt handling'),
    (625,647,'src/vms_dcl_command.c','process','external and internal image selection'),
    (648,717,'src/vms_dcl_command.c','order_internal','result token ordering and native image dispatch'),
    (718,739,'src/vms_dcl_command.c','process','privileged image rundown with CONTINUE SPAWN ATTACH exceptions'),
    (740,747,'src/vms_dcl_command.c','fail','completion status and message report'),
    (748,763,'src/vms_dcl_command.c','process','internal image number routing to source image services'),
    (764,789,'src/vms_dcl_command.c','port_dcl_cmd_abort','force LOGOUT from hangup or abort'),
    (790,810,'src/vms_dcl_command.c','port_dcl_cmd_eod','EOD no-status and DECK invalid value'),
    (811,832,'src/vms_dcl_command.c','port_dcl_cmd_check_ast','Control-Y restart check'),
    (833,864,'src/vms_dcl_command.c','port_dcl_cmd_disable','nested AST disable and restore'),
    (865,893,'src/vms_dcl_command.c','port_dcl_cmd_flush','nonterminal input flush and indirect suppression'),
    (894,952,'src/vms_dcl_command.c','parameters','table prompt lookup and continuation answer'),
    (953,986,'src/vms_dcl_command.c','process','foreign image tail preservation'),
    (987,988,None,None,'VAX assembler terminator and blank line')
]
command_parts=[]
for a,b,path,symbol,reason in command_ranges:
    if path is None:
        command_parts.append({'original_lines':[a,b],'status':'excluded','reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        command_parts.append({'original_lines':[a,b],
            'status':'mapped' if line else 'missing','pc_file':path,
            'pc_symbol':symbol,'pc_line':line,'function':reason})
result['files'].append({'original_file':'DCL/SRC/COMMAND.MAR',
    'original_lines':988,'sections':command_parts,
    'platform_changes':['VAX command stack and result table mapped to native C parse state',
                        'Image vector dispatch uses native function calls',
                        'RMS input and indirect record positioning use service callbacks'],
    'unresolved_integration': 'DCLTABLES full grammar and external image execution belong to other source modules and are not yet available. Native guest currently registers only implemented verbs.'})

# REMACP remote-terminal CONFIGURE and DISCONNECT data and functions.
# Source: REM/SRC/REMPROTCL.MAR; REMDEF version and ECO in REM/SRC/REMMACROS.MAR.
rem_protocol_ranges = [
    (1,62,None,None,'ownership notice, history, source declarations'),
    (63,78,'src/vms_rem_protocol.h','port_rem_protocol_init','REM data and native terminal state'),
    (79,85,'src/vms_rem_protocol.c','port_rem_send_config','eight-byte VMS CONFIGURE template'),
    (86,93,'src/vms_rem_protocol.c','rem_disconnect','disconnect wire type and reason'),
    (94,116,None,None,'protocol service description and calling convention'),
    (117,134,'src/vms_rem_protocol.c','port_rem_protocol_receive','CONFIGURE tag, peer version, support bit'),
    (135,155,'src/vms_rem_protocol.c','port_rem_protocol_receive','common version and minimum ECO'),
    (156,165,'src/vms_rem_protocol.c','port_rem_protocol_receive','eight-byte characteristics and ECO extended longword'),
    (166,179,'src/vms_rem_protocol.c','port_rem_protocol_receive','online flag, verify channel, network window and RTT attention'),
    (180,185,'src/vms_rem_protocol.c','rem_kill','bad message kills the device'),
    (186,197,'src/vms_rem_protocol.c','rem_disconnect','write DISCONNECT with completion cleanup'),
    (198,205,'src/vms_rem_protocol.c','port_rem_send_config','send original CONFIGURE without completion callback'),
    (206,215,None,None,'description of asynchronous DISCONNECT completion'),
    (216,220,'src/vms_rem_protocol.c','port_rem_send_complete','remove channel on DISCONNECT write completion'),
    (221,223,None,None,'assembler end and blank line')
]
parts=[]
for begin,end,path,symbol,why in rem_protocol_ranges:
    if path is None:
        parts.append({'original_lines':[begin,end],'status':'excluded','reason':why})
    else:
        line=line_for_symbol(path,symbol)
        if not line:missing+=1
        parts.append({'original_lines':[begin,end],
            'status':'mapped' if line else 'missing','pc_file':path,
            'pc_symbol':symbol,'pc_line':line,'function':why})
result['files'].append({'original_file':'REM/SRC/REMPROTCL.MAR',
    'original_lines':223,'sections':parts,
    'platform_changes':['UCB data and network window stored in native PC terminal state',
                        'VAX JSB and AST calls use native channel/attention/send callbacks'],
    'unresolved_integration':'The REMACP listener, RTTDRIVER and NSP object 23 connection are not yet linked to live network sessions.'})

# SYS/SRC/BOOPARAM.MAR: exact 96-byte SYSBOOT/INIT parameter cells.
booparam_ranges=[
    (1,87,None,None,'source header, history and parameter descriptions'),
    (88,93,'src/vms_booparam.h','port_boo_parameters','start of bootstrap parameter block'),
    (94,108,'src/vms_booparam.h','port_boo_parameters','driver pointers, INILOA and nonpaged pool'),
    (109,120,'src/vms_booparam.h','port_boo_parameters','IRP, LRP and SRP split/count storage'),
    (121,128,'src/vms_booparam.h','port_boo_parameters','file cache, BOOTCB, TOPSYS and SCS ID'),
    (129,136,'src/vms_booparam.h','port_boo_parameters','port driver, microcode, SCS load and size'),
    (137,138,None,None,'assembler end and blank line')
]
segments=[]
for first,last,path,symbol,reason in booparam_ranges:
    if path is None:
        segments.append({'original_lines':[first,last],'status':'excluded','reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        segments.append({'original_lines':[first,last],
                         'status':'mapped' if line else 'missing',
                         'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':reason})
result['files'].append({'original_file':'SYS/SRC/BOOPARAM.MAR',
                       'original_lines':138,'sections':segments,
                       'platform_changes':['Native 32-bit parameter fields keep the original offsets',
                                           'SYSBOOT to INIT handoff copies all original fields'],
                       'unresolved_integration':'SYSBOOT still has to load actual system images and set parameters before SYSINIT can run.'})

# Native SYSBOOT console, BOOTS/SRC/READPRMPT.MAR. Source data offsets
# and the QIO read, QIO write and RIO output entries use the same device.
boot_console_ranges = [
    (1,65,None,None,'source header, history and parameter descriptions'),
    (66,84,'src/vms_boot_readprompt.h','port_boo_console_init','QIO IOSB, _OPA0 channel and 256-byte RIO buffer'),
    (85,119,None,None,'read/write interface and calling description'),
    (120,129,'src/vms_boot_readprompt.c','port_boo_readprompt','lazy channel assignment for _OPA0'),
    (130,135,'src/vms_boot_readprompt.c','port_boo_readprompt','prompt length and counted record'),
    (136,146,'src/vms_boot_readprompt.c','port_boo_readprompt','QIO READPROMPT and IOSB byte count'),
    (147,156,'src/vms_boot_readprompt.c','port_boo_readprompt','output-only QIO WRITEVBLK and return status'),
    (157,169,None,None,'RIO output line entry description'),
    (170,185,'src/vms_boot_readprompt.c','port_rio_output_line','append CR LF and use output-only BOO readprompt'),
    (186,186,None,None,'assembler end and blank line')
]
sections=[]
for first,last,path,symbol,reason in boot_console_ranges:
    if path is None:
        sections.append({'original_lines':[first,last],'status':'excluded',
                         'reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        sections.append({'original_lines':[first,last],
                         'status':'mapped' if line else 'missing',
                         'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                         'function':reason})
result['files'].append({'original_file':'BOOTS/SRC/READPRMPT.MAR',
                       'original_lines':186,'sections':sections,
                       'platform_changes':['SYSBOOT _OPA0 QIO calls map to the native serial console',
                                           'VAX IOSB is represented by native 16-bit status and count cells'],
                       'unresolved_integration':'SYSBOOT, SYSINIT, RMS/F11 disk mounting and STARTUP.COM are not yet ready. Startup stops rather than enter LOGINOUT without them.'})

# SYS/SRC/INITVEC.MAR is the INILOA loadable-entry linkage descriptor.
initvec_ranges = [
    (1,73,None,None,'source conditions, history and vector description'),
    (74,110,'src/vms_initvec.c','port_ini_vectors_link','source LOADVEC layout and self-relative entry resolution'),
    (111,112,None,None,'VAX program-section select for loadable image'),
    (113,134,'src/vms_initvec.c','port_ini_vectors_link','validate loadable code header and image length'),
    (135,145,'src/vms_initvec.c','port_ini_vectors_init','initial unresolved SYS.EXE vector'),
    (146,152,'src/vms_initvec.c','port_ini_vectors_link','INI$IOMAP type-3 entry and end marker'),
    (153,154,None,None,'assembler end and whitespace')
]
segments=[]
for begin,end,path,symbol,why in initvec_ranges:
    if path is None:
        segments.append({'original_lines':[begin,end],'status':'excluded','reason':why})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        segments.append({'original_lines':[begin,end],
                         'status':'mapped' if line else 'missing',
                         'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                         'function':why})
result['files'].append({'original_file':'SYS/SRC/INITVEC.MAR',
                       'original_lines':154,'sections':segments,
                       'platform_changes':['VAX JMP vector stores a verified native IA-32 entry address',
                                           'INILOA self-relative code header and type-3 offset are checked'],
                       'unresolved_integration':'INILOA.EXE is loaded from Files-11 into guest supervisor RAM, but INIT has not linked/entered INI$IOMAP; no guest I/O database or RPB handoff yet.'})

# BOOTS/SRC/BOOTIO.MAR: bootstrap device I/O, file cache and image attributes.
# Parameter-file loading is in SYSBOOT.MAR, not in this module.
bootio_sections=[
    (1,68,None,None,'source ownership, history and descriptions'),
    (69,87,'src/vms_bootio.c','boo_cache_alloc','cache sizing macro (90 percent threshold, four-page alignment)'),
    (88,140,None,None,'source service description and calling parameters'),
    (141,153,'src/vms_bootio.c','boo_rdwr_lbn','RPB bootstrap driver with physical memory mode'),
    (154,190,None,None,'source cache description and arguments'),
    (191,206,'src/vms_bootio.c','boo_cache_alloc','MEM_CACHE_TABLE original nine choices including zero sentinel'),
    (207,212,None,None,'source page allocation notes'),
    (213,230,'src/vms_bootio.c','boo_cache_alloc','select first matching cache size and usable pages'),
    (231,233,None,None,'source cache open description'),
    (234,239,'src/vms_bootio.c','boo_cache_init','allocate then open cache'),
    (240,256,'src/vms_bootio.c','boo_cache_open','initialize FIL$CACHE_INIT descriptor and directory cache'),
    (257,269,None,None,'source physical-page scan description'),
    (270,286,'src/vms_bootio.c','boo_alloc_pages','scan RPB valid bits, accept half or full contiguous cache'),
    (287,309,None,None,'source image attribute description'),
    (310,332,'src/vms_bootio.c','boo_image_attributes','image debug/global symbol and patch VBN trim'),
    (333,361,None,None,'source bootstrap SYS$ASSIGN description'),
    (362,367,'src/vms_bootio.c','boo_assign','return RPB address as source bootstrap channel'),
    (368,374,None,None,'source notes for bootstrap default directories'),
    (375,379,'src/vms_bootio.c','boo_default_directory','FIL$GT_DDSTRING and empty default device'),
    (380,381,None,None,'end of source')
]
segments=[]
for first,last,path,symbol,why in bootio_sections:
    if path is None:
        segments.append({'original_lines':[first,last],'status':'excluded',
                         'reason':why})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        segments.append({'original_lines':[first,last],
                         'status':'mapped' if line else 'missing',
                         'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':why})
result['files'].append({'original_file':'BOOTS/SRC/BOOTIO.MAR',
                       'original_lines':381,'sections':segments,
                       'platform_changes':['VAX RPB I/O vector is a native PC disk service',
                                           'VAX PFN bitmap is a native byte bitmap',
                                           'Image header offsets are supplied from the image type'],
                       'unresolved_integration':'The original FIL$OPENFILE path for SYS.EXE and other system images is not complete; the current raw IDE test disk has no Files-11 home block.'})

# BOOTS/SRC/BOOTBLOCK.MAR: the entire original boot-block job has a PC
# counterpart. BIOS replaces the VAX ROM LBN reader and the VAX UNIBUS
# address conversion is inapplicable to native IA-32 physical memory.
bootblock_sections=[
    (1,55,None,None,'original DEC source history and layout explanation'),
    (56,63,'src/boot.s','stage2_magic','boot file size, first LBN and kernel destination'),
    (64,153,None,None,'VAX UNIBUS/MASSBUS inputs and explanatory comments; PC uses BIOS drive number'),
    (154,174,'src/boot.s','_start','bootstrap metadata and physical load address'),
    (175,179,'src/boot.s','read_block','BIOS CHS block read and persistent disk-error halt'),
    (180,186,'src/boot.s','next_block','advance destination and LBN; decrement remaining count'),
    (187,195,'src/boot.s','protected_entry','enter IA-32 protected mode and transfer to the loaded image'),
    (196,198,None,None,'VAX-only UNIBUS CSR masking; no UNIBUS in a native PC'),
    (199,201,None,None,'source end directive and blank lines')]
boot_sections=[]
for first,last,path,symbol,why in bootblock_sections:
    if path is None:
        boot_sections.append({'original_lines':[first,last],
                             'status':'excluded','reason':why})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        boot_sections.append({'original_lines':[first,last],
                     'status':'mapped' if line else 'missing',
                     'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                     'function':why})
result['files'].append({'original_file':'BOOTS/SRC/BOOTBLOCK.MAR',
   'original_lines':201,'sections':boot_sections,
   'platform_changes':['VAX ROM block read replaced by PC BIOS INT 13h',
                       'VAX swapped LBN fields replaced by native little-endian LBN',
                       'PC loads executable to physical 0x10000 and enters protected mode',
                       'BIOS boot sector stores block count outside its instruction entry']})

# SYSBOOT and FILEREAD are only partly ported. Do not record these
# original files as complete until their remaining source is implemented.
sysboot_read_ranges=[
    (1,656,None,None,'SYSBOOT machine setup, image loading and other entries','missing'),
    (657,674,'src/vms_sysboot_read.c','port_sysboot_getcurrent','GETCURRENT locates SYS.EXE through Files-11','mapped'),
    (675,724,'src/vms_sysboot_read.c','port_sysboot_getcurrent','SYS.EXE file header and executable region','mapped'),
    (725,750,'src/vms_sysboot_read.c','port_sysboot_getcurrent','SYSPARAM VBN mapping and disk block transfer','mapped'),
    (751,1082,None,None,'SYSBOOT parameter parsing and early memory preparation unresolved','missing'),
    (1083,1135,'src/vms_sysboot_pktmem.c','port_boo_packet_layout','16-byte aligned virtual packet-pool sizing, page-rounded IRP/LRP/SRP reservation (partial)','partial'),
    (1136,1359,None,None,'SYSBOOT PFN calculation, page tables and physical memory setup unresolved','missing'),
    (1360,1430,'src/vms_sysboot_pktmem.c','port_boo_packet_layout','Source-ordered SRP/IRP/LRP resident allocation, BOO packet parameters (partial)','partial'),
    (1431,1958,None,None,'SYSBOOT paging, pool, CPU and image driver startup unresolved','missing'),
    (1959,1992,'src/vms_sysboot_read.c','port_sysboot_image_init','IMAGE_OPEN resolves the image and its file header','mapped'),
    (1993,2008,None,None,'fatal scattered-file messages and IMAGE_INIT description','missing'),
    (2009,2056,'src/vms_sysboot_read.c','port_sysboot_image_init','IMAGE_INIT reads file image header and executable block range','mapped'),
    (2057,2160,None,None,'SYSBOOT missing image diagnostics, directory and pool setup','missing'),
    (2161,2211,'src/vms_sysboot_read.c','port_sysboot_load_code','LOAD_CODE reads executable bytes from Files-11','mapped'),
    (2212,2263,'src/vms_sysboot_map.c','port_sysboot_setmap','BOO$SETMAP normalizes a segment of the block map','mapped'),
    (2264,2280,'src/vms_sysboot_map.c','port_sysboot_mapvbn','BOO$MAPVBN resolves one-origin virtual blocks','mapped'),
    (2281,2339,None,None,'other SYSBOOT block reading and data setup','missing'),
    (2340,2399,'src/vms_sysboot_read.c','port_sysboot_read_virtual','READ_VIRTUAL reads the mapped file region','mapped'),
    (2400,2897,None,None,'QIO_RWLB queue logic, conversational parameter files and other SYSBOOT services','missing')
]
sections=[]
for first,last,path,symbol,reason,status in sysboot_read_ranges:
    if path is None:
        sections.append({'original_lines':[first,last],'status':status,'reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        sections.append({'original_lines':[first,last],
                         'status':status if line else 'missing',
                         'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                         'function':reason})
result['files'].append({'original_file':'BOOTS/SRC/SYSBOOT.MAR',
                       'original_lines':2897,'sections':sections,
                       'platform_changes':['Native IA-32 SYS.EXE is a separately linked ELF32 file installed in the original Files-11 directory; the VAX image record format is CPU-specific', 'SYSBOOT ELF32 LOAD_CODE streams real PT_LOAD bytes from the F11 VBN map into reserved supervisor RAM'],
                       'unresolved_integration':'SYSBOOT normalized BOO$L_SYS_MAP retrieval pointers now derive from live Files-11 SYS.EXE extents. WCB overlay and packet pool PFN/SPT mapping remain incomplete; SYSPARAM and process startup are unresolved.'})

fileread_sections=[
    (1,740,None,None,'other FILEREAD declarations, cache setup, full filename parser and open logic','missing'),
    (741,866,'src/vms_dq_index_read.c','port_dq_index_read','FIL$MOUNT native resident INDEXF.SYS first FH2 read/validation with real physical IRP; SYSBOOT file lookup remains separate, no ACP','partial'),
    (867,1000,None,None,'directory cache and original argument frame are unfinished','missing'),
    (1001,1215,'src/vms_boot_files11.c','port_f11_find_file','directory header and level 2 directory record scan','mapped'),
    (1216,1377,None,None,'directory cache, level 1 search and other unported paths','missing'),
    (1378,1494,'src/vms_boot_files11.c','port_f11_read_file_header','indexed file header and checksums','mapped'),
    (1495,1545,'src/vms_boot_files11.c','port_f11_collect_file_extents','follow file header FID chain, combine normalized extents, detect cycles and overflow','mapped'),
    (1546,1635,'src/vms_boot_files11.c','port_f11_read_vbn','VBN to LBN and disk block reads','mapped'),
    (1636,1760,'src/vms_boot_files11.c','read_pointer','structure level 2 retrieval pointer formats','mapped'),
    (1761,1870,'src/vms_boot_files11.c','port_f11_file_extent_stats','file allocation count and contiguous LBN','mapped'),
    (1871,1999,'src/vms_boot_files11.c','port_f11_read_file_header','file ID and checksum validation, level 2 only','mapped')
]
sections=[]
for first,last,path,symbol,reason,status in fileread_sections:
    if path is None:
        sections.append({'original_lines':[first,last],'status':status,'reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        sections.append({'original_lines':[first,last],
                         'status':status if line else 'missing',
                         'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                         'function':reason})
result['files'].append({'original_file':'SYS/SRC/FILEREAD.MAR',
                       'original_lines':1999,'sections':sections,
                       'unresolved_integration':'Structure level 1, numeric/UIC directories, directory cache, full FIL$OPENFILE and remaining error cases need work. M137 reads real INDEXF.SYS FH2 in resident supervisor memory; M138 follows its FH2 retrieval-pointer map to read MFD FID 4,4,0 in another privileged IRP. M139 resolves MFD->SYSEXE.DIR->SYS.EXE FIDs via actual resident directory reads. M140 physically reads SYS.EXE FH2 and first native ELF32 sector using separate IRPs, but does not activate an image, bind Files-11 ACP or publish DQA0 DDT. M144 also verifies real SYSLOA386/INILOA supervisor text from Files-11 mapped physical sectors. M145 adds bounded multi-VBN directory search using the same FH2 map and a caller-supplied native disk callback; early SYS.EXE uses only its already-read VBN1 cache, not a live Files-11 ACP. M146 adds byte-granular FH2-mapped source FIL$READVBN with partial-I/O accounting and a real guest ATA read of an unaligned three-sector range, still outside any live public IOC/ACP. M147-M149 adds bounded source FIL$FINDFILID -> FIL$RDCHKFILHDR -> FIL$READVBN directory/file lookup and version enumeration over real native ATA reads, with independent FH2 verification, atomic lookup output, error-injection tests and host/guest checks; all remain privileged private boot operations, not a public Files-11 ACP. M150-M152 additionally follows source FIL$RDCHKFILHDR extension FID chains using the original SYSBOOT normalized extent decoder, protects against cycles/physical overflow, and reads cross-extent bytes through real privileged ATA sector callbacks; test fixtures include disjoint second-header extents. M216 rereads 64 SYS.EXE bytes through the published DQA0 DDT selected by IOC$SEARCHDEV using a real READPBLK/IRQ14 completion, confirming the FH2 chain callback reaches the native DDT; this is still supervisor INIT integration, not ACP or public $QIO. M217 extends the verified post-publication read to 700 unaligned file bytes across two sectors, requires exactly two READPBLK IRPs through the published DDT and compares the bytes against the independent chain cache; this remains supervisor INIT integration, not ACP or public QIO. No active ACP or VAX process image activation. M185-M192 adds a limited supervisor-only owner-scoped channel assignment, seek, read, status and release layer over the private Files-11 IOC bridge; this is not original process $ASSIGN, public $QIO, or ACP.'})

# SYSINIT remains incomplete. Only its three source-defined system
# logical names and translation loop have a native implementation.
sysinit_sections = [
    (1,311,None,None,'SYSINIT other storage and service definitions are missing','missing'),
    (312,332,'src/vms_sysinit_names.c','port_sip_create_system_names','system logical name table and directory strings','mapped'),
    (333,528,None,None,'other startup process and disk descriptors remain missing','missing'),
    (529,562,None,None,'entry and service description','excluded'),
    (563,577,None,None,'process entry and lockdown are not implemented','missing'),
    (578,628,'src/vms_sysinit_names.c','port_sip_create_system_names','TRNLOG SYS$SYSDEVICE, strip underscore, append directory, CRELOG system table','mapped'),
    (629,1905,None,None,'time setup, paging/swap, RMS, F11ACP, SYSMSG and STARTUP not implemented','missing')
]
sections=[]
for first,last,path,symbol,reason,status in sysinit_sections:
    if path is None:
        sections.append({'original_lines':[first,last],'status':status,'reason':reason})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        sections.append({'original_lines':[first,last],
                         'status':'mapped' if line else 'missing','pc_file':path,
                         'pc_symbol':symbol,'pc_line':line,'function':reason})
result['files'].append({'original_file':'SYSINI/SRC/SYSINIT.MAR',
                       'original_lines':1905,'sections':sections,
                       'unresolved_integration':'No complete native SYSINIT process, no login until required services run.'})

# Original INIT disk creation modules. One full original source file per
# section group, with source line ranges mapped by actual PC function names.
init_disk_sources = [
    ('INIT/SRC/INIMFD.B32',257,[
        (1,88,None,None,'source notice, setup and comment lines'),
        (89,196,'src/vms_init_mfd.c','port_init_mfd','fixed ODS-1 RAD50 and ODS-2 MFD file IDs and records'),
        (197,255,'src/vms_init_mfd.c','port_init_mfd','initial MFD block write through disk service'),
        (256,257,None,None,'BLISS source close')]),
    ('INIT/SRC/INIBIT.B32',262,[
        (1,69,None,None,'source notice and service description'),
        (70,145,'src/vms_init_bitmap.c','port_init_bitmap','initial bitmap output and allocation-table input'),
        (146,187,'src/vms_init_bitmap.c','port_init_bitmap','SCB structure levels, CHS details and checksum'),
        (188,258,'src/vms_init_bitmap.c','port_init_bitmap','sorted cluster allocation, bitmap bits and zero-filled tail'),
        (259,262,None,None,'BLISS source close')]),
    ('INIT/SRC/ININDX.B32',650,[
        (1,90,None,None,'source notice and data descriptions'),
        (91,182,'src/vms_init_index.c','port_init_index','native BIOS non-system boot block and initial file header fields'),
        (183,315,'src/vms_init_index.c','port_init_index','INIT_INDEX allocation metadata, boot and home block construction'),
        (316,364,'src/vms_init_index.c','port_init_index','home copies and initial index bitmap'),
        (365,505,'src/vms_init_index.c','port_init_index','nine reserved headers and their original file data and maps'),
        (506,564,'src/vms_init_index.c','home_block','two home block checksums and sequential LBN updates'),
        (565,649,'src/vms_init_index.c','make_pointer','format one, two and three retrieval pointers'),
        (650,650,None,None,'BLISS source close')]),
    ('MOUNT/SRC/CHKSM2.B32',119,[
        (1,79,None,None,'source notice and interface description'),
        (80,116,'src/vms_f11_home.c','port_f11_checksum2','16-bit additive checksum and writeback'),
        (117,119,None,None,'BLISS source close')]),
    ('MOUNT/SRC/CHKHM1.B32',138,[
        (1,63,None,None,'original DEC source notice and interface description'),
        (64,108,'src/vms_f11_home.c','port_f11_check_home1','ODS-1 inputs and required nonzero fields'),
        (109,136,'src/vms_f11_home.c','port_f11_check_home1','checksums, two structure versions and zero-padded label'),
        (137,138,None,None,'BLISS source end')]),
    ('MOUNT/SRC/CHKHM2.B32',143,[
        (1,105,None,None,'source notice and interface description'),
        (106,140,'src/vms_f11_home.c','port_f11_check_home2','home block fields, two sums and padded label'),
        (141,143,None,None,'BLISS source close')])
]
for original,length,ranges in init_disk_sources:
    sections=[]
    for first,last,path,symbol,reason in ranges:
        if path is None:
            sections.append({'original_lines':[first,last], 'status':'excluded', 'reason':reason})
        else:
            line=line_for_symbol(path,symbol)
            if line is None: missing+=1
            sections.append({'original_lines':[first,last],
                'status':'mapped' if line else 'missing',
                'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':reason})
    result['files'].append({'original_file':original,'original_lines':length,
        'sections':sections,
        'platform_changes':(['PDP-11 non-system bootblock code uses the same warning and halt on IA-32']
            if original=='INIT/SRC/ININDX.B32' else []),
        'unresolved_integration':'The full INIT utility, SYS.EXE and its loadable system images remain unfinished.'})


# SYS/SRC/FILERWIO.MAR: complete callable FIL$RDWRTLBN variant.
# The bootstrap uses BOOTIO's RPB driver form; FILERWIO is the QIOW form.
filerwio_ranges=[
    (1,101,None,None,'source license, history, declarations and API description'),
    (102,103,'src/vms_filerwio.c','port_filerwio_rdwr_lbn','entry and frame setup'),
    (104,115,'src/vms_filerwio.c','port_filerwio_rdwr_lbn','eight-byte IOSB, QIOW call and IOSB/service status choice'),
    (116,118,None,None,'assembler end directive, blank line')
]
# Read the exact source line count in future updates before changing this range.
fr=[]
for first,last,path,sym,why in filerwio_ranges:
    if path is None:fr.append({'original_lines':[first,last], 'status':'excluded','reason':why})
    else:
        line=line_for_symbol(path,sym)
        if line is None:missing+=1
        fr.append({'original_lines':[first,last],'status':'mapped' if line else 'missing',
                   'pc_file':path,'pc_symbol':sym,'pc_line':line,'function':why})
result['files'].append({'original_file':'SYS/SRC/FILERWIO.MAR',
                        'original_lines':118,'sections':fr,
                        'platform_changes':['QIOW is a native PC disk service; separate queue and IOSB status are retained']})

# BOOTS/SRC/BOOTS.MAR: all non-comment macro instructions translate to
# IA-32 boot driver table entries and the VMB -> SYSBOOT argument record.
vmb_sections=[
    (1,94,None,None,'DEC license, revision notes, and driver table description'),
    (95,104,'src/vms_vmbargs.c','port_boot_driver_register','CPU and device filters, action and driver size fields'),
    (105,116,'src/vms_vmbargs.c','port_boot_driver_register','driver-relative offsets and entry point field'),
    (117,128,'src/vms_vmbargs.c','port_boot_driver_register','optional driver name, unit initialization, source psect end'),
    (129,133,None,None,'VMB argument layout description'),
    (134,140,'src/vms_vmbargs.h','port_vmb_arguments','argument count and FILEREAD cache descriptor'),
    (141,145,'src/vms_vmbargs.h','port_vmb_arguments','PFN bounds, good-page map, optional microcode, SCS node'),
    (146,149,'src/vms_vmbargs.h','port_vmb_arguments','boot flags, CI bound, flag bit zero'),
    (150,152,'src/vms_vmbargs.c','port_vmb_memory_arguments','native RPB argument count and memory descriptors'),
    (153,155,None,None,'source END directives and final blank line')]
vmb_cov=[]
for first,last,path,symbol,purpose in vmb_sections:
    if path is None:
        vmb_cov.append({'original_lines':[first,last],'status':'excluded','reason':purpose})
    else:
        at=line_for_symbol(path,symbol)
        if at is None:missing+=1
        vmb_cov.append({'original_lines':[first,last],
                        'status':'mapped' if at else 'missing',
                        'pc_file':path,'pc_symbol':symbol,'pc_line':at,
                        'function':purpose})
result['files'].append({'original_file':'BOOTS/SRC/BOOTS.MAR',
    'original_lines':155,'sections':vmb_cov,
    'platform_changes':['VAX driver PSECT symbols become relative PC service table offsets',
                        'native BIOS E820 replaces VAX memory controller discovery'],
    'unresolved_integration':'VMB.MAR and SYSBOOT.MAR machine initialization remain incomplete; SYS.EXE now exists in Files-11 and loads, but INIT/INILOA and system services do not yet run.'})

# SYSINI/SRC/SYSMOU.B32: the complete MOUNT_SYSTEM and MAIN_HANDLER
# sequencing is implemented with explicit service calls. Missing ACP
# service providers stop boot instead of claiming a successful mount.
sysmount_ranges=[
    (1,97,None,None,'source notice, history and BLISS declarations'),
    (98,142,'src/vms_sysmount.c','port_sysmount_reset','mount status and resource storage'),
    (143,180,'src/vms_sysmount.c','port_sysmount_reset','original MOUDEF system disk options, default volume label'),
    (181,238,'src/vms_sysmount.c','port_sysmount_system','system mount arguments and privileged service entry'),
    (239,270,'src/vms_sysmount.c','port_sysmount_system','GETCHN primary/secondary checks, device flags and physical name'),
    (271,296,'src/vms_sysmount.c','port_sysmount_system','home read, ODS mount, ACP and conditional rebuild'),
    (297,350,'src/vms_sysmount.c','port_sysmount_main_handler','condition handler, signal status and mechanism data'),
    (351,362,'src/vms_sysmount.c','port_sysmount_main_handler','severe condition unwind and return status'),
    (363,365,None,None,'BLISS end directives and blank line'),
]
sysmount_sections=[]
for first,last,path,sym,why in sysmount_ranges:
    if path is None:
        sysmount_sections.append({'original_lines':[first,last], 'status':'excluded', 'reason':why})
    else:
        line=line_for_symbol(path,sym)
        if line is None:missing+=1
        sysmount_sections.append({'original_lines':[first,last],
            'status':'mapped' if line else 'missing', 'pc_file':path,
            'pc_symbol':sym,'pc_line':line,'function':why})
result['files'].append({'original_file':'SYSINI/SRC/SYSMOU.B32',
    'original_lines':365,'sections':sysmount_sections,
    'unresolved_integration':'Requires real $GETCHN, mounted volume and F11ACP. Native SYS.EXE and SYSINIT remain incomplete.'})

# BOOTS/SRC/VMB.MAR: only the source-backed RPB sections below are done.
# Memory-controller, console restart and secondary image activation remain missing.
vmb_rpb_ranges=[
    (695,760,'src/vms_vmb_rpb.c','port_vmb_init_restart','boot device, restart settings and initial RPB fields'),
    (774,805,'src/vms_vmb_rpb.c','port_vmb_init_restart','RPB PFN map and early physical-page allocations'),
    (859,877,'src/vms_vmb_rpb.c','port_vmb_init_restart','PFN bounds, memory descriptors and free page counts'),
    (1685,1720,'src/vms_vmb_rpb.c','port_vmb_init_restart','good-page counting from BIOS E820 range map')]
rpb_sections=[]
for first,last,path,sym,why in vmb_rpb_ranges:
    line=line_for_symbol(path,sym)
    if line is None:missing+=1
    rpb_sections.append({'original_lines':[first,last],
       'status':'mapped' if line else 'missing','pc_file':path,
       'pc_symbol':sym,'pc_line':line,'function':why})
# Explicitly list the unimplemented ranges. A mapped RPB function does not
# make VMB.MAR as a whole complete.
current=1
vmb_full_sections=[]
for mapped in rpb_sections:
    first,last=mapped['original_lines']
    if current<first:
        vmb_full_sections.append({'original_lines':[current,first-1],
            'status':'missing','reason':'VMB source routines not yet ported'})
    vmb_full_sections.append(mapped)
    current=last+1
if current<=2402:
    vmb_full_sections.append({'original_lines':[current,2402],
        'status':'missing','reason':'VMB secondary-image transfer and hardware startup still pending'})
result['files'].append({'original_file':'BOOTS/SRC/VMB.MAR',
   'original_lines':2402,'sections':vmb_full_sections,
   'unresolved_integration':'Many VMB routines are not yet implemented, including console restart, complete memory discovery and SYSBOOT transfer.'})

# SYS/SRC/SYSGETTIM.MAR: current system time with calling-mode access
# protection and a retry after a changing quadword is copied.
gettim_ranges=[
    (1,76,None,None,'DEC notice, declarations and service interface'),
    (77,81,'src/vms_exe_gettim.c','port_exe_gettim','get destination and check write permission'),
    (82,82,'src/vms_exe_gettim.c','port_exe_gettim','write a time quadword to the caller buffer'),
    (83,86,'src/vms_exe_gettim.c','port_exe_gettim','verify stable value; retry after concurrent update'),
    (87,88,'src/vms_exe_gettim.c','port_exe_gettim','normal completion or access violation'),
    (89,91,None,None,'end directive and blank lines')]
gettim=[]
for first,last,path,symbol,purpose in gettim_ranges:
    if path is None:
        gettim.append({'original_lines':[first,last],'status':'excluded','reason':purpose})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        gettim.append({'original_lines':[first,last],'status':'mapped' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':purpose})
result['files'].append({'original_file':'SYS/SRC/SYSGETTIM.MAR',
    'original_lines':91,'sections':gettim,
    'platform_changes':['IA-32 32-bit clock stores use a sequence for consistent 64-bit reads',
                        'page-table translation replaces the VAX IFNOWRT access check'],
    'unresolved_integration':'Executive service dispatch and SYS.EXE image activation remain incomplete'})
# SYS/SRC/SYSSETSSF.MAR: all executable source operations are mapped.
ssf_ranges=[
    (1,57,None,None,'DEC notice, service interface and declaration section'),
    (58,62,'src/vms_exe_setssf.c','port_exe_setssf','caller previous-mode test and NOPRIV result'),
    (63,64,'src/vms_exe_setssf.c','port_exe_setssf','filter byte store and NORMAL result'),
    (65,67,None,None,'return/end directive and blank lines')]
ssf=[]
for first,last,path,symbol,purpose in ssf_ranges:
    if path is None:
        ssf.append({'original_lines':[first,last],'status':'excluded','reason':purpose})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        ssf.append({'original_lines':[first,last],'status':'mapped' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':purpose})
result['files'].append({'original_file':'SYS/SRC/SYSSETSSF.MAR',
    'original_lines':67,'sections':ssf,
    'platform_changes':['VAX PSL previous mode is passed from the native IA-32 service trap'],
    'unresolved_integration':'The control-region filter byte needs the native executive service vector'})

# SYS/SRC/SYSSETEXV.MAR: mode checking and exception-vector assignment.
setexv_ranges=[
    (1,81,None,None,'DEC notice and service calling description'),
    (82,86,'src/vms_exe_setexv.c','port_exe_setexv','maximize requested mode with previous access mode'),
    (87,90,'src/vms_exe_setexv.c','port_exe_setexv','primary, secondary and last-chance vector selection'),
    (91,95,'src/vms_exe_setexv.c','port_exe_setexv','optional previous vector result, access violation without a store'),
    (96,98,'src/vms_exe_setexv.c','port_exe_setexv','assign new handler and return normal status'),
    (99,101,None,None,'assembler end and blank lines')]
setexv=[]
for first,last,path,symbol,purpose in setexv_ranges:
    if path is None:
        setexv.append({'original_lines':[first,last],'status':'excluded','reason':purpose})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        setexv.append({'original_lines':[first,last],'status':'mapped' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':purpose})
result['files'].append({'original_file':'SYS/SRC/SYSSETEXV.MAR',
    'original_lines':101,'sections':setexv,
    'platform_changes':['VAX condition-handler vectors become process control-region arrays',
                        'native page-table write checks replace VAX IFNOWRT'],
    'unresolved_integration':'Native executive process control region and service trap dispatch still missing'})


# Full translated service body for SYSGETSYI, including the first and
# final descriptor probes, data conversion, event and AST completion.
def exe_mapped_source(original, total, regions, notes):
    sections=[]
    for first,last,pc_file,pc_symbol,purpose in regions:
        if pc_file is None:
            sections.append({'original_lines':[first,last], 'status':'excluded',
                             'reason':purpose})
            continue
        number=line_for_symbol(pc_file,pc_symbol)
        if number is None:
            raise RuntimeError('Missing native symbol '+str(pc_symbol))
        sections.append({'original_lines':[first,last], 'status':'mapped',
                         'pc_file':pc_file,'pc_symbol':pc_symbol,
                         'pc_line':number,'function':purpose})
    result['files'].append({'original_file':original,'original_lines':total,
                            'sections':sections,'unresolved_integration':notes})

exe_mapped_source('SYS/SRC/SYSGETSYI.MAR',392,[
    (1,87,None,None,'copyright, historical notes and calling sequence'),
    (88,104,'src/vms_exe_getsyi.c','port_exe_getsyi','structure and item limits'),
    (105,167,None,None,'service interface and declarations'),
    (168,176,'src/vms_exe_getsyi.c','port_exe_getsyi','optional IOSB write probe and clear'),
    (177,190,'src/vms_exe_getsyi.c','port_exe_getsyi','event clear and AST quota check'),
    (191,218,'src/vms_exe_getsyi.c','port_exe_getsyi','descriptor walk and write protection'),
    (219,230,'src/vms_exe_getsyi.c','port_exe_getsyi','success and error dispatch'),
    (231,249,'src/vms_exe_getsyi.c','port_exe_getsyi','post event, IOSB and completion AST'),
    (250,292,None,None,'CHECKITEM interface description'),
    (293,303,'src/vms_exe_getsyi.c','port_exe_getsyi','software/hardware item limit checks'),
    (304,346,None,None,'PUTDATA interface description'),
    (347,374,'src/vms_exe_getsyi.c','port_exe_getsyi','8-byte version, CPU type and SID'),
    (375,390,'src/vms_exe_getsyi.c','port_exe_getsyi','MOVC5 zero padding, truncated length and access checks'),
    (391,392,None,None,'assembler terminator')],
    'Actual process event flags and AST dispatch must be installed in SYS.EXE before guest use; hardware SID needs IA-32 startup binding.')

exe_mapped_source('SYS/SRC/VERSION.MAR',53,[
    (1,41,None,None,'copyright notice and declarations'),
    (42,48,'src/vms_exe_version.c','port_exe_announce','original release announcement byte string'),
    (49,50,'src/vms_exe_version.c','port_exe_version_quadword','release version value S1JY'),
    (51,53,None,None,'end directive')],
    'The announcement remains unused until SYS.EXE completes required initialization.')

exe_mapped_source('SYS/SRC/SYSLOAVEC.MAR',240,[
    (1,118,None,None,'license, notes, source switch and macro description'),
    (119,141,'src/vms_sysloavec.c','port_sysloavec_link','typed resident vectors and loadable-image relative offsets'),
    (142,146,None,None,'section labels'),
    (147,163,'src/vms_sysloavec.c','port_sysloavec_link','load image size and 12-byte descriptor validation'),
    (164,169,'src/vms_sysloavec.c','port_sysloavec_init','resident vector table base'),
    (170,176,None,None,'vector list headings'),
    (177,209,'src/vms_sysloavec.c','types','all 23 vector entries in original source order'),
    (210,213,'src/vms_sysloavec.c','port_sysloavec_link','negative vector-list terminator'),
    (214,219,None,None,'unloaded-vector note'),
    (220,223,'src/vms_sysloavec.c','port_sysloavec_unloaded','native processor halt on unresolved entry'),
    (224,228,None,None,'patch area heading'),
    (229,236,'src/vms_sysloavec.c','port_sysloavec_patch_init','fifteen-word patch area'),
    (237,240,None,None,'source terminator')],
    'SYSLOA386 is loaded from Files-11 and LINK_VEC binds all 23 descriptors in resident SYS.EXE; only EXE$INIBOOTADP and EXE$INIPROCREG implement native hardware functions. Other entries trap if invoked, and cannot be counted as completed services.')
# Vector table presence is not CPU service implementation. 21 of 23
# original vector targets still trap if called; mark that range partial.
result['files'][-1]['sections'][6]['status']='partial'


exe_mapped_source('SYS/SRC/SYSEVTSRV.MAR',314,[
    (1,117,None,None,'DEC notice and system service calling description'),
    (118,124,'src/vms_exe_event.c','port_exe_clref','clear event system service'),
    (125,155,None,None,'cluster resolution calling interface'),
    (156,180,'src/vms_exe_event.c','port_exe_getefc','local and common cluster selection with slave-to-master path'),
    (181,208,None,None,'internal clear service calling interface'),
    (209,224,'src/vms_exe_event.c','port_exe_clref','interlocked flag clear, old-bit state and errors'),
    (225,254,None,None,'read event service interface'),
    (255,268,'src/vms_exe_event.c','port_exe_readef','entire 32-bit flag mask, destination checks, old-bit status'),
    (269,298,None,None,'set event service interface'),
    (299,310,'src/vms_exe_event.c','port_exe_setef','scheduler post and error-status propagation'),
    (311,314,None,None,'end directive')],
    'SCH$POSTEF must be bound to the live process scheduler for wakeups; SYS.EXE is not executable yet.')


exe_mapped_source('SYS/SRC/SYSSCHEVT.MAR',300,[
    (1,67,None,None,'copyright, history and assembler declarations'),
    (68,80,'src/vms_exe_schevt.c','port_exe_schdwk','source-defined wake and timer argument order'),
    (81,132,None,None,'calling conventions and service descriptions'),
    (133,138,'src/vms_exe_schevt.c','port_exe_schdwk','resolve target PID with caller access checks'),
    (139,154,'src/vms_exe_schevt.c','port_exe_schdwk','repeat delta, sign checks and 10-ms lower bound'),
    (155,163,'src/vms_exe_schevt.c','port_exe_schdwk','time probe, entry allocation and AST quota debit'),
    (164,204,None,None,'SETIMR service description and assembler sections'),
    (205,212,'src/vms_exe_schevt.c','port_exe_setimr','clear and validate selected event flag'),
    (213,234,'src/vms_exe_schevt.c','port_exe_setimr','AST and JIB timer quota, mode, request ID and EF number'),
    (235,245,'src/vms_exe_schevt.c','port_exe_setimr','quota failure and real scheduler resource wait callback'),
    (246,252,None,None,'BUILD TQE description'),
    (253,266,'src/vms_exe_schevt.c','fetch_quad','page-safe eight-byte read and bounded nonpaged entry allocation'),
    (267,287,'src/vms_exe_schevt.c','expiration','absolute/delta time translation and repeated wake validation'),
    (288,297,'src/vms_exe_schevt.c','port_exe_timer_advance','sorted time queue, expiry dispatch, quota return and failures'),
    (298,300,None,None,'assembler ending and blank lines')],
    'Full native timer services are implemented and host tested. The guest must bind '
    'event posting, wakeups, AST delivery and resource waiting to live PCB queues '
    'after SYS.EXE starts; no timer service is reported live before that.')

exe_mapped_source('SYS/SRC/SYSSETMOD.MAR',163,[
    (1,80,None,None,'DEC notice, macros and service calling description'),
    (81,83,'src/vms_exe_setmod.c','port_exe_setrwm','set caller resource wait flag'),
    (84,111,None,None,'failure-mode service description'),
    (112,116,'src/vms_exe_setmod.c','port_exe_setsfm','select previous-mode failure bit'),
    (117,150,None,None,'swap-mode service description'),
    (151,154,'src/vms_exe_setmod.c','port_exe_setswm','check PSWAPM privilege'),
    (155,159,'src/vms_exe_setmod.c','set_mode','old-state return and low-bit insertion'),
    (160,163,None,None,'source ending')],
    'Live process PCB status bits need to bind to this native mode view during '
    'SYS.EXE process creation and service dispatch.')

exe_mapped_source('SYS/SRC/SYSCANEVT.MAR',140,[
    (1,59,None,None,'DEC notice, macros and argument definitions'),
    (60,80,None,None,'cancel-timer service calling description'),
    (81,88,'src/vms_exe_canevt.c','port_exe_cantim',
     'maximum access mode, caller PID, request ID and timer type'),
    (89,126,None,None,'cancel-wakeup service calling description'),
    (127,131,'src/vms_exe_canevt.c','port_exe_canwak',
     'resolve target PID with access and privilege checks'),
    (132,136,'src/vms_exe_canevt.c','remove_matching',
     'remove matching TQE entries, restore quota and report normal'),
    (137,140,None,None,'source ending')],
    'Scheduler timer-IPL exclusion is provided by the caller lock. Guest '
    'process resolution and the executive dispatch still need live bindings.')

exe_mapped_source('SYS/SRC/TIMESCHDL.MAR',423,[
    (1,115,None,None,'source setup and clock interrupt interface'),
    (116,120,'src/vms_exe_gettim.c','port_exe_systime_tick',
     'actual i386 PIT IRQ0 increments the VMS 100-ns quadword by 100000'),
    (121,125,None,None,'clock statistics header')],
    'Only hardware time update from EXE$HWCLKINT is ported so far; process '
    'quantum, timer soft interrupts, accounting, and other ranges 126-423 '
    'are still required. This original source file is not complete.')
# Mark substantive unported TIMESCHDL ranges as missing rather than excluded.
result['files'][-1]['sections'].append({
    'original_lines':[126,423], 'status':'missing',
    'reason':'CPU accounting, quantum expiration, timer software dispatch and related routines are not ported'})

# M83: eleven original executive modules, including scheduler and mode dispatch.
for original,length,ranges,note in [
    ('SYSSETSTK.MAR',114,[(1,89,None,None,'DEC source and stack service interface'),
       (90,112,'vms_exe_setstk','port_exe_setstk','mode selection, old limits and access probes'),
       (113,114,None,None,'assembler end')],
       'Requires live per-process stack limit arrays and caller-mode memory access.'),
    ('SYSSETPRV.MAR',195,[(1,108,None,None,'copyright, history and service interface'),
       (109,136,'vms_exe_setprv','port_exe_setprv','validate flags and return old privilege mask'),
       (137,146,'vms_exe_setprv','port_exe_setprv','service exit statuses'),
       (147,178,'vms_exe_setprv','port_exe_setprv','authorized privilege, image, inner-mode and partial grant'),
       (179,193,'vms_exe_setprv','port_exe_setprv','current, PHD and permanent disable'),
       (194,195,None,None,'assembler end')],
       'Requires PCB/PHD privilege fields and live service dispatch.'),
    ('SYSSETPRI.MAR',177,[(1,129,None,None,'source history and process priority interface'),
       (130,145,'vms_exe_setpri','port_exe_setpri','previous priority access and target process lookup'),
       (146,158,'vms_exe_setpri','port_exe_setpri','authorized priority cap and saved fields'),
       (159,175,'vms_exe_setpri','port_exe_setpri','mutex, ready queue notification and old priority return'),
       (176,177,None,None,'source ending')],
       'Requires live PCB process resolution and scheduler-ready-queue callback.'),
    ('SYSSETPRA.MAR',176,[(1,103,None,None,'source header and power AST interface'),
       (104,125,'vms_exe_setpra','port_exe_setpra','register AST, access mode, quota and pending flag'),
       (126,134,None,None,'power recovery routine description'),
       (135,154,'vms_exe_setpra','port_exe_powerast','visit PCB vector, allocate kernel recovery AST and clear duration'),
       (155,165,None,None,'special AST description'),
       (166,174,'vms_exe_setpra','port_exe_powerast_requeue','convert kernel AST to normal AST'),
       (175,176,None,None,'assembler end')],
       'Physical power-fail handler and actual AST allocator must bind to these routines.'),
    ('SYSADJSTK.MAR',128,[(1,89,None,None,'source header and outer stack interface'),
       (90,100,'vms_exe_adjstk','port_exe_adjstk','outer mode privilege check and new pointer'),
       (101,113,'vms_exe_adjstk','port_exe_adjstk','signed adjustment and 512-byte growth protection'),
       (114,126,'vms_exe_adjstk','port_exe_adjstk','user stack expansion, errors and saved-mode register'),
       (127,128,None,None,'assembler end')],
       'Requires live saved stack pointers and real kernel-mode stack expansion.'),
    ('SYSDERLMB.MAR',136,[(1,113,None,None,'copyright and error-log service interface'),
       (114,134,'vms_exe_derlmb','port_exe_derlmb','DIAGNOSE privilege, synchronized owner and unit release'),
       (135,136,None,None,'assembler end')],
       'Error-log I/O forwarding must consume the declared mailbox descriptor.'),
    ('SYSGETPTI.MAR',332,[(1,131,None,None,'copyright, declarations and memory-service interface'),
       (132,144,'vms_exe_getpti','port_exe_getpti','initialize 512-byte virtual page search and minimum pages'),
       (145,204,None,None,'per-page routine interface and page-class description'),
       (205,281,'vms_exe_getpti','port_exe_getpti','PTE lookup, page ownership, matching run and minimum size'),
       (282,330,'vms_exe_getpti','demand_zero','transition, global and section demand-zero detection'),
       (331,332,None,None,'assembler end')],
       'VMS PTE provider must be bound to the native IA-32 page ownership map.'),
    ('SYSCHGMOD.MAR',132,[(1,103,None,None,'source history and mode service interface'),
       (104,108,'vms_exe_chgmod','port_exe_cmexec','CMEXEC privilege and fallback'),
       (109,130,'vms_exe_chgmod','port_exe_cmkrnl','CMKRNL privilege and native protected CALLG'),
       (131,132,None,None,'assembler end')],
       'Mode executor must perform actual IA-32 privilege and stack change.'),
    ('SYSDCLCMH.MAR',114,[(1,91,None,None,'source history and change-handler interface'),
       (92,103,'vms_exe_dclcmh','port_exe_dclcmh','select supervisor, user or compatibility vector'),
       (104,112,'vms_exe_dclcmh','port_exe_dclcmh','old-vector write, invalid service bugcheck and access checks'),
       (113,114,None,None,'assembler end')],
       'Live exception and compatibility-mode trap dispatch not yet installed.'),
    ('SCHED.MAR',108,[(1,57,None,None,'copyright, source setup and scheduling interface'),
       (58,61,None,None,'native SCH$RESCHED interrupt entry, IPL transition and SVPCTX save remain unimplemented'),
       (62,67,'vms_v37_sched','port_v37_sched_resched_current','Requeue current resident PCB at the COM queue tail and set queue summary; caller context save and hardware interrupt entry remain unimplemented'),
       (68,74,None,None,'source comments and SCH$SCHED entry separator'),
       (75,96,'vms_v37_sched','port_v37_sched_select','Highest resident COM queue selection, CUR state, current PCB/priority and floating priority; native context restore/PCBB programming remain unimplemented'),
       (97,98,'vms_v37_ia32_context','port_v37_ia32_context_init','construct initial IA-32 IRET frame with matching VMS CPL; QEMU dispatches a selected CPL2 frame from a CPL1 trap, but VMS PCB context save/restore and scheduler integration remain unresolved'),
       (99,106,None,None,'source idle wait and fatal empty-queue path lack native scheduler context restoration'),
       (107,108,None,None,'assembler end')],
       'The two-task diagnostic adapter is linked in the kernel; resident VMS PCB layout, executive wait/wake queues and normal reschedule-interrupt entry remain unbound.'),
    ('NULLPROC.MAR',103,[(1,97,None,None,'copyright and idle process description'),
       (98,101,'vms_exe_nullproc','port_exe_nullproc','native unconditional idle branch matching source BRB loop'),
       (102,103,None,None,'assembler end')],
       'Idle task must be selected by the live process scheduler.')
]:
    mapped=[]
    for first,last,basename,symbol,reason in ranges:
        if basename is None:
            mapped.append({'original_lines':[first,last],'status':'excluded','reason':reason})
        else:
            path='src/'+basename+'.c'
            line=line_for_symbol(path,symbol)
            mapped.append({'original_lines':[first,last],
                           'status':('partial' if original=='SCHED.MAR' and line else
                                     'mapped' if line else 'missing'),
                           'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                           'function':reason})
            if line is None:missing+=1
    result['files'].append({'original_file':'SYS/SRC/'+original,
                            'original_lines':length,'sections':mapped,
                            'unresolved_integration':note})


# M84: twenty source-file mappings. Keep blank and declaration ranges out of functional counts.
for original,total,parts,note in [('SYS/SRC/SYSASTCON.MAR', 188, [(117, 127, 'vms_exe_astcon', 'port_exe_setast'), (166, 186, 'vms_exe_astcon', 'port_exe_dclast')], 'Live AST queue and process quota must be bound before dispatch.'), ('SYS/SRC/SYSWAIT.MAR', 353, [(123, 160, 'vms_exe_wait', 'port_exe_wfland'), (161, 200, 'vms_exe_wait', 'port_exe_wflor'), (201, 254, 'vms_exe_wait', 'port_exe_waitfr'), (255, 352, 'vms_exe_wait', 'wait_common')], 'Actual context switch and wait queue wakeup need the resident scheduler.'), ('SYS/SRC/SYSDCLEXH.MAR', 159, [(81, 97, 'vms_exe_dclexh', 'port_exe_canexh'), (125, 136, 'vms_exe_dclexh', 'port_exe_dclexh'), (144, 155, 'vms_exe_dclexh', 'choose')], 'Per-mode termination lists must be bound to each PCB.'), ('SYS/SRC/SYSEXIT.MAR', 149, [(101, 145, 'vms_exe_exit', 'port_exe_exit')], 'Rundown and actual process deletion need native process manager binding.'), ('SYS/SRC/SYSFORCEX.MAR', 140, [(107, 124, 'vms_exe_forcex', 'port_exe_forcex'), (133, 139, 'vms_exe_forcex', 'port_exe_forcex_deliver')], 'Pending forced-exit AST requires native delivery.'), ('SYS/SRC/SYSSETIME.MAR', 511, [(124, 148, 'vms_exe_setime', 'port_exe_setime'), (188, 223, 'vms_exe_setime', 'port_exe_setime'), (224, 303, 'vms_exe_setime', 'year_for'), (304, 403, 'vms_exe_setime', 'port_exe_setime'), (404, 451, 'vms_exe_setime', 'port_exe_setime'), (469, 509, 'vms_exe_setime', 'port_exe_setime')], 'RTC, clock ownership, timer queue and SYS.EXE SYSPARAM persistent writes need live callbacks.'), ('SYS/SRC/SYSCANCEL.MAR', 242, [(127, 138, 'vms_exe_cancel', 'port_exe_canceln'), (139, 241, 'vms_exe_cancel', 'port_exe_cancel')], 'Actual driver cancel and ACP request queues remain unbound.'), ('SYS/SRC/SYSDASSGN.MAR', 204, [(109, 203, 'vms_exe_dassgn', 'port_exe_dassgn')], 'Driver cancellation, channel and mount state require native I/O database binding.'), ('SYS/SRC/SYSDEVALC.MAR', 384, [(171, 248, 'vms_exe_devalc', 'port_exe_alloc'), (298, 338, 'vms_exe_devalc', 'port_exe_dalloc'), (339, 382, 'vms_exe_devalc', 'release_one')], 'Native DDB/UCB and driver callbacks must bind to executive I/O database.'), ('SYS/SRC/SYSPURGWS.MAR', 354, [(122, 192, 'vms_exe_purgws', 'port_exe_purgws'), (193, 262, 'vms_exe_purgws', 'port_exe_purgws_scan'), (263, 353, 'vms_exe_purgws', 'port_exe_purgws_scan')], 'Live page-table/WSL ownership, pagefile and unmap callbacks needed.'), ('SYS/SRC/SYSADJWSL.MAR', 422, [(151, 245, 'vms_exe_adjwsl', 'port_exe_adjwsl'), (246, 306, 'vms_exe_adjwsl', 'port_exe_wspchk'), (307, 400, 'vms_exe_adjwsl', 'port_exe_adjwsl'), (401, 421, 'vms_exe_adjwsl', 'port_exe_extra_dyn_ws')], 'Native process working-set, PHD growth and page reclaim need memory manager integration.'), ('SYS/SRC/SYSSETPRT.MAR', 409, [(151, 243, 'vms_exe_setprt', 'port_exe_setprt'), (244, 408, 'vms_exe_setprt', 'adjust_one')], 'Real system PTE and copy-on-reference pagefile ownership must be wired.'), ('SYS/SRC/CONSOLIO.MAR', 280, [(75, 90, 'vms_exe_consolio', 'port_exe_outhex'), (112, 135, 'vms_exe_consolio', 'port_exe_outhex'), (136, 158, 'vms_exe_consolio', 'port_exe_outblank'), (159, 222, 'vms_exe_consolio', 'port_exe_outchar'), (223, 248, 'vms_exe_consolio', 'port_exe_outcrlf'), (249, 269, 'vms_exe_consolio', 'port_exe_outcstring'), (270, 279, 'vms_exe_consolio', 'port_exe_outzstring')], 'Native serial register implementation must bind output hooks to hardware.'), ('SYS/SRC/SCBVECTOR.MAR', 164, [(69, 71, 'vms_exe_scbvector', 'port_exe_scb_init'), (73, 161, 'vms_exe_scbvector', 'port_exe_scb_init')], 'Original vector order and stack designation mapped; PC IDT targets not all installed. Unmapped VAX-only nexus hardware vectors have no PC counterpart.'), ('BOOTS/SRC/ACTIMAGE.MAR', 139, [(105, 138, 'vms_boo_actimage', 'port_boo_actimage')], 'Native image activator and process rundown must bind to actual image loader.'), ('BOOTS/SRC/READLBN.MAR', 179, [(105, 114, 'vms_rtf_readlbn', 'port_rtf_target_dev'), (115, 156, 'vms_rtf_readlbn', 'port_rtf_target_dev'), (157, 178, 'vms_rtf_readlbn', 'port_rtf_read_lbn')], 'VMS QIOW device channel and RT-11 filesystem mount binding pending.'), ('BOOTS/SRC/READDRIV.MAR', 176, [(79, 94, 'vms_iogen_readdriv', 'port_iogen_readdriv'), (139, 175, 'vms_iogen_readdriv', 'port_iogen_readdriv')], 'Real RMS OPEN and section mapping must bind to native image loader.'), ('BOOTS/SRC/CONIO.MAR', 202, [(115, 201, 'vms_boo_conio', 'port_boo_conio_prompt')], 'Real serial I/O hooks not yet connected to standalone SYSBOOT.'), ('BOOTS/SRC/READPRMPT.MAR', 186, [(68, 85, 'vms_boo_readprompt', 'port_boo_readprompt'), (120, 169, 'vms_boo_readprompt', 'port_boo_readprompt'), (170, 185, 'vms_boo_readprompt', 'port_boo_output_line')], 'System terminal assignment and QIOW must bind to resident I/O stack.'), ('BOOTS/SRC/SYSGENMN.MAR', 137, [(96, 112, 'vms_boo_sysgenmn', 'port_boo_sysgen_start'), (123, 136, 'vms_boo_sysgenmn', 'port_boo_sysgen_puterror')], 'Actual SYSGEN utility CLI, system parameters and message writer still need native binding.')]:
    if original=='BOOTS/SRC/READPRMPT.MAR':
        continue  # Existing SYSBOOT entry maps this complete source file.
    regions=[]
    next_line=1
    for first,last,name,symbol in parts:
        if first>next_line:
            regions.append((next_line,first-1,None,None,'DEC header, descriptions or assembler declarations'))
        regions.append((first,last,'src/'+name+'.c',symbol,'native functional counterpart to original section'))
        next_line=last+1
    if next_line<=total:
        regions.append((next_line,total,None,None,'ending directives and nonfunctional text'))
    exe_mapped_source(original,total,regions,note)


# Native TU58 media access and SYSGEN device database display.
exe_mapped_source('BOOTS/SRC/T58BOOTIO.MAR',99,[
    (1,79,None,None,'copyright, VAX ROM interface and declarations'),
    (80,98,'src/vms_boo_t58bootio.c','port_t58_read_lbn',
     'original TU58 unit-zero logical block read through native physical driver'),
    (99,99,None,None,'assembler end')],
    'A physical TU58 serial drive must exist for its native block driver to be bound; VAX ROM entry is excluded.')
exe_mapped_source('BOOTS/SRC/SHODEV.MAR',255,[
    (1,59,None,None,'copyright and revision history'),
    (60,254,'src/vms_boo_shodev.c','port_boo_shodev',
     'SYSGEN SHOW/DRIVER and SHOW/DEVICE database walk and formatting'),
    (255,255,None,None,'assembler end')],
    'Real device database pointers and RIO$OUTPUT_LINE need native SYSGEN binding.')


exe_mapped_source('LOGIN/SRC/PROTCLI.MAR',151,[
    (1,75,None,None,'original notice and CLI protection interface'),
    (76,120,'src/vms_lgi_protcli.c','port_lgi_protect_cli',
     'supervisor ownership, write limits and page-table invalidation'),
    (121,141,None,None,'image activation service calling description'),
    (142,149,'src/vms_lgi_protcli.c','port_lgi_execute_image',
     'rundown user process and transfer to the selected image'),
    (150,151,None,None,'assembler end')],
    'Live LOGINOUT mappings, TLB, and EXE$PROCIMGACT transfer still require guest linkage.')

# M85: original runtime library services and SYS file-name, I/O and fork modules.
for original,total,first,last,path,symbol,note in [
    ('RTL/SRC/LIBANASTR.MAR',224,127,222,'src/vms_rtl85.c','port_lib_analyze_sdesc','Supported VMS descriptors; guest pointer conversion remains to bind.'),
    ('RTL/SRC/LIBEXTZV.MAR',123,115,121,'src/vms_rtl85.c','port_lib_extzv','Reserved operand maps to error until guest exception delivery is attached.'),
    ('RTL/SRC/LIBEXTV.MAR',123,117,121,'src/vms_rtl85.c','port_lib_extv','Reserved operand maps to error until guest exception delivery is attached.'),
    ('RTL/SRC/LIBINSV.MAR',123,117,121,'src/vms_rtl85.c','port_lib_insv','Guest reserved-operand exception is not attached.'),
    ('RTL/SRC/LIBMOVC3.MAR',132,122,130,'src/vms_rtl85.c','port_lib_movc3','Native byte copy.'),
    ('RTL/SRC/LIBMOVC5.MAR',143,131,141,'src/vms_rtl85.c','port_lib_movc5','Native byte copy and fill.'),
    ('RTL/SRC/LIBFFS.MAR',143,133,141,'src/vms_rtl85.c','port_lib_ffs','VMS bit-scan and not-found status.'),
    ('RTL/SRC/LIBFFC.MAR',143,133,141,'src/vms_rtl85.c','port_lib_ffc','VMS bit-scan and not-found status.'),
    ('RTL/SRC/LIBICHAR.MAR',158,137,156,'src/vms_rtl85.c','port_lib_ichar','First character through decoded string descriptor.'),
    ('RTL/SRC/LIBLEN.MAR',147,136,145,'src/vms_rtl85.c','port_lib_len','String descriptor length.'),
    ('RTL/SRC/LIBSKPC.MAR',164,129,162,'src/vms_rtl85.c','port_lib_skpc','First unequal character position.'),
    ('RTL/SRC/LIBLOCC.MAR',165,128,163,'src/vms_rtl85.c','port_lib_locc','First equal character position.'),
    ('RTL/SRC/LIBSPANC.MAR',164,135,162,'src/vms_rtl85.c','port_lib_spanc','Table and mask scan until zero.'),
    ('RTL/SRC/LIBSCANC.MAR',162,135,160,'src/vms_rtl85.c','port_lib_scanc','Table and mask scan until nonzero.'),
    ('RTL/SRC/LIBINDEX.MAR',204,144,201,'src/vms_rtl85.c','port_lib_index','Find substring position.'),
    ('RTL/SRC/LIBMATCHC.MAR',191,137,189,'src/vms_rtl85.c','port_lib_matchc','Find substring position with swapped args.'),
    ('RTL/SRC/LIBABUPCA.MAR',108,90,105,'src/vms_rtl85_tables.c','port_lib_ab_upcase','All 256 ASCII uppercase table bytes.'),
    ('RTL/SRC/LIBCVTUO.MAR',106,101,104,'src/vms_rtl85_tables.c','port_lib_ab_cvt_u_o','20 overpunch characters.'),
    ('RTL/SRC/LIBCVTOU.MAR',135,100,132,'src/vms_rtl85_tables.c','port_lib_ab_cvt_o_u','256-element overpunch-to-digit conversion table.'),
    ('RTL/SRC/LIBCVTPTU.MAR',132,97,129,'src/vms_rtl85_tables.c','port_lib_ab_cvtpt_u','Original 256-byte decimal conversion table.'),
    ('RTL/SRC/LIBCVTTPU.MAR',133,98,130,'src/vms_rtl85_tables.c','port_lib_ab_cvttp_u','Original 256-byte decimal conversion table.'),
    ('RTL/SRC/LIBCVTPTO.MAR',132,97,129,'src/vms_rtl85_tables.c','port_lib_ab_cvtpt_o','Original 256-byte decimal conversion table.'),
    ('RTL/SRC/LIBCVTTPO.MAR',133,98,130,'src/vms_rtl85_tables.c','port_lib_ab_cvttp_o','Original 256-byte decimal conversion table.'),
    ('RTL/SRC/LIBCVTPTZ.MAR',138,102,136,'src/vms_rtl85_tables.c','port_lib_ab_cvtpt_z','Original 256-byte decimal conversion table.'),
    ('RTL/SRC/LIBCVTTPZ.MAR',139,103,137,'src/vms_rtl85_tables.c','port_lib_ab_cvttp_z','Original 256-byte decimal conversion table.'),
    ('SYS/SRC/CVTATB.MAR',176,114,169,'src/vms_rtl85.c','port_sys_cvt_dtb','The three numeric conversion entry points are port_sys_cvt_dtb, port_sys_cvt_otb and port_sys_cvt_htb.'),
    ('SYS/SRC/RMSRESET.MAR',155,121,151,'src/vms_sys85.c','port_rm_reset','RMS impure data reset requires per-process RMS data to be bound.'),
    ('SYS/SRC/MTFDT.MAR',93,81,90,'src/vms_sys85.c','port_mt_check_access','Device and access-rights callbacks need native I/O database binding.'),
    ('SYS/SRC/CVTFILNAM.MAR',319,137,317,'src/vms_sys85.c','port_fil_cvtfilnam','Files-11 RAD50 word format and version conversion.'),
    ('SYS/SRC/FORKCNTRL.MAR',208,86,205,'src/vms_sys85.c','port_exe_fork','Software interrupt dispatcher and UCB callbacks require kernel interrupt binding.')
]:
    exe_mapped_source(original,total,[
        (1,first-1,None,None,'Original license notice, declarations, and interface text'),
        (first,last,path,symbol,'Native functional equivalent'),
        (last+1,total,None,None,'Assembler termination or blank lines')
    ],note)

# M117: source-exact 56-byte MB/NL/OP DPTAB headers, not active drivers.
# The size field is intentionally zero, and original MB/OP dispatch is absent.
for original,total,first,last,path,sym in [
    ('SYS/SRC/MBDRIVER.MAR',840,107,119,'src/vms_ini_dpt_mb.c','port_v37_dpt_stage_mb'),
    ('SYS/SRC/CONINTDSP.MAR',556,83,97,'src/vms_ini_dpt_op.c','port_v37_dpt_stage_op')]:
    result['files'].append({'original_file':original,'original_lines':total,
        'sections':[
          {'original_lines':[1,first-1],'status':'unresolved',
           'reason':'Original driver prelude, register details and interfaces require full translation'},
          {'original_lines':[first,last],'status':'partial','pc_file':path,'pc_symbol':sym,
           'pc_line':line_for_symbol(path,sym),
           'function':'Source prologue values only; no linked driver END/dispatch, IOC or full original driver'},
          {'original_lines':[last+1,total],'status':'unresolved',
           'reason':'Original driver dispatch, hardware and full I/O behavior remain unimplemented'}],
        'unresolved_integration':'These original drivers are not yet linked as working VMS device images.'})
result['files'].append({'original_file':'SYS/SRC/NLDRIVER.MAR',
    'original_lines':136,
    'sections':[
      {'original_lines':[1,48],'status':'excluded',
       'reason':'Copyright, edit history and VAX source declarations'},
      {'original_lines':[49,58],'status':'partial',
       'pc_file':'src/vms_ini_dpt_nl.c','pc_symbol':'port_v37_dpt_stage_nl',
       'pc_line':line_for_symbol('src/vms_ini_dpt_nl.c','port_v37_dpt_stage_nl'),
       'function':'Original 56-byte NL DPT prefix; source driver END/IOC DPT list still absent'},
      {'original_lines':[59,62],'status':'unresolved',
       'reason':'Source VAX assembler section/page directives'},
      {'original_lines':[63,70],'status':'partial',
       'pc_file':'src/vms_nl_ddt_build.c','pc_symbol':'port_v37_nl_ddt_build',
       'pc_line':line_for_symbol('src/vms_nl_ddt_build.c','port_v37_nl_ddt_build'),
       'function':'Original NL DDTAB function table and source relative-address fields; MNTVER still absent'},
      {'original_lines':[71,76],'status':'excluded',
       'reason':'Page markers and function table title/comments'},
      {'original_lines':[77,91],'status':'partial',
       'pc_file':'src/vms_nl_ddt_masks.c','pc_symbol':'port_v37_nl_ddt_masks',
       'pc_line':line_for_symbol('src/vms_nl_ddt_masks.c','port_v37_nl_ddt_masks'),
       'function':'Original source FUNCTAB legal/buffered/read/write masks and relative handler offsets'},
      {'original_lines':[92,111],'status':'unresolved',
       'reason':'Driver procedure prologue, VAX FDT calling registers and comments not fully translated'},
      {'original_lines':[112,133],'status':'partial',
       'pc_file':'src/vms_nl_ddt_dispatch.c','pc_symbol':'port_v37_nl_ddt_dispatch',
       'pc_line':line_for_symbol('src/vms_nl_ddt_dispatch.c','port_v37_nl_ddt_dispatch'),
       'function':'Native source decision, actual NL read EOF/write normal action through EXE$FINISHIOC and resident IRP FIFO'},
      {'original_lines':[134,136],'status':'excluded',
       'reason':'Source END labels and assembler directives'}],
    'unresolved_integration':'Only NLA0 now has linked native software DDT/FDT; IOC DPT registration, user $QIO, all other devices and IOPOST AST dispatch remain incomplete.'})

exe_mapped_source('SYS/SRC/SVAPTE.MAR' ,247,[
    (1,122,None,None,'DEC license, page-table layout and interface text'),
    (123,198,'src/vms_exe_memory85.c','port_mmg_svaptechk','native IA-32 system PTE reference and bounds'),
    (199,220,'src/vms_exe_memory85.c','port_mmg_pteadrchk','native validated PTE address'),
    (221,243,'src/vms_exe_memory85.c','port_mmg_pteindex','process and system region boundaries'),
    (244,247,None,None,'assembler end')],
    'Guest page-fault handler must implement nonresident page-table recovery and fatal callback.')
exe_mapped_source('SYS/SRC/BUFFERCTL.MAR',245,[
    (1,80,None,None,'DEC license, declarations and IO buffer interface'),
    (81,114,'src/vms_exe_memory85.c','port_ioc_getbyte','single byte with mapped-page crossing'),
    (115,139,'src/vms_exe_memory85.c','port_ioc_putbyte','single byte write with mapped-page crossing'),
    (140,163,'src/vms_exe_memory85.c','port_ioc_initbufwind','window initialization'),
    (164,196,'src/vms_exe_memory85.c','port_ioc_movfruser','copy from checked user memory'),
    (197,228,'src/vms_exe_memory85.c','port_ioc_movtouser','copy into checked user memory'),
    (229,242,'src/vms_exe_memory85.c','port_ioc_filspt','native IA-32 PTE lookup and page window'),
    (243,245,None,None,'assembler end')],
    'Device UCB and physical memory callbacks must be bound to native I/O driver.')

exe_mapped_source('SYS/SRC/DEBUGDATA.MAR',52,[
    (1,34,None,None,'DEC license and processor register data description'),
    (35,49,'src/vms_exe_image_end85.c','port_bug_debug','512-byte bugcheck register store and IA-32 alignment'),
    (50,52,None,None,'assembler end')],
    'Native bugcheck exception handler must save the IA-32 trap frame to these slots.')
exe_mapped_source('SYS/SRC/MDATEND.MAR',102,[
    (1,84,None,None,'DEC license, history and memory manager data layout'),
    (85,90,'src/vms_exe_image_end85.c','port_mmg_pfn_patch','16 six-byte PFN opcode patch slots and zero termination'),
    (91,96,'src/vms_exe_image_end85.c','port_mmg_pfn_patch_desc','PFN fixup patch descriptor'),
    (97,100,'src/vms_exe_image_end85.c','port_mmg_sys_end','page-aligned system image end marker'),
    (101,102,None,None,'assembler end')],
    'The final SYS.EXE link script must place .vms_sys_end after every resident section.')


# M86: entire ODS-1 and ODS-2 file-control routines. Shared code is mapped
# to each original implementation; each different structure-level path is kept.
f11_86_files = [
    ('F11A/SRC/CHKSUM.B32',125,60,121,'port_f11a_checksum'),
    ('F11B/SRC/CHKSUM.B32',124,59,120,'port_f11b_checksum'),
    ('F11A/SRC/CPYNAM.B32',120,62,116,'port_f11a_copy_name'),
    ('F11B/SRC/CPYNAM.B32',119,61,115,'port_f11b_copy_name'),
    ('F11B/SRC/FILESIZE.B32',129,58,125,'port_f11b_file_size'),
    ('F11A/SRC/GETTIM.B32',174,66,170,'port_f11a_get_time'),
    ('F11B/SRC/GETTIM.B32',153,60,149,'port_f11b_get_time'),
    ('F11B/SRC/GETPTR.B32',152,67,148,'port_f11b_get_map_pointer'),
    ('F11B/SRC/GETRVN.B32',186,66,182,'port_f11b_apply_rvn'),
    ('F11B/SRC/MATCHNAME.MAR',145,95,142,'port_f11b_match_name'),
    ('F11A/SRC/BADSCN.B32',99,62,95,'port_f11a_scan_badlog'),
    ('F11A/SRC/RETDIR.B32',126,62,122,'port_f11a_return_dir'),
    ('F11B/SRC/RETDIR.B32',151,65,147,'port_f11b_return_dir'),
    ('F11A/SRC/LOCKDB.MAR',132,84,127,'port_f11a_lock_iodb'),
    ('F11B/SRC/LOCKDB.MAR',134,86,129,'port_f11b_lock_iodb'),
    ('F11A/SRC/GETACC.MAR',108,92,103,'port_f11a_get_access'),
    ('F11B/SRC/GETACC.MAR',108,92,103,'port_f11b_get_access'),
    ('F11A/SRC/REQUEU.MAR',115,98,110,'port_f11a_requeue'),
    ('F11B/SRC/REQUEU.MAR',121,103,116,'port_f11b_requeue'),
    ('F11A/SRC/MPWIND.MAR',130,105,125,'port_f11a_map_window'),
    ('F11B/SRC/MPWIND.MAR',139,113,134,'port_f11b_map_window'),
    ('F11A/SRC/NXTHDR.B32',157,61,153,'port_f11a_next_header'),
    ('F11B/SRC/NXTHDR.B32',160,60,156,'port_f11b_next_header'),
]
for orig,total,first,last,sym in f11_86_files:
    integration = ('Native Files-11 ACP process, file-control and disk driver links '
                   'are not yet active in the guest. The original source is '
                   'ported; callbacks must be attached to the live executive.')
    exe_mapped_source(orig,total,[
        (1,first-1,None,None,'Original notices, type definitions and routine description'),
        (first,last,'src/vms_f11_acp86.c',sym,'Native ODS-1/ODS-2 routine'),
        (last+1,total,None,None,'Original module terminator or blank source')
    ],integration)

# M87: original ODS-1 and ODS-2 ACP file-header, FCB, and I/O paths.
# The structures and callbacks are in the IA-32 Files-11 ACP source.
f11_87_files = [
    ('F11A/SRC/MOUNT.B32',167,65,163,'port_f11a_mount'),
    ('F11B/SRC/MOUNT.B32',166,64,162,'port_f11b_mount'),
    ('F11A/SRC/SCHFCB.B32',145,63,141,'port_f11a_search_fcb'),
    ('F11B/SRC/SCHFCB.B32',146,63,142,'port_f11b_search_fcb'),
    ('F11A/SRC/MAKSTR.B32',174,63,170,'port_f11a_make_string'),
    ('F11B/SRC/MAKSTR.B32',174,63,170,'port_f11b_make_string'),
    ('F11A/SRC/CHKHDR.B32',173,63,169,'port_f11a_check_header'),
    ('F11B/SRC/CHKHD2.B32',184,70,180,'port_f11b_check_header2'),
    ('F11A/SRC/INIFCB.B32',158,63,154,'port_f11a_init_fcb'),
    ('F11B/SRC/INIFC2.B32',260,71,256,'port_f11b_init_fcb2'),
    ('F11A/SRC/CREFCB.B32',217,68,213,'port_f11a_create_fcb'),
    ('F11B/SRC/CREFCB.B32',182,70,178,'port_f11b_create_fcb'),
    ('F11B/SRC/MAKPTR.B32',274,80,270,'port_f11b_make_pointer'),
    ('F11A/SRC/RDHEDR.B32',177,66,173,'port_f11a_read_header'),
    ('F11B/SRC/RDHEDR.B32',200,72,196,'port_f11b_read_header'),
    ('F11A/SRC/CREWIN.B32',168,66,164,'port_f11a_create_window'),
    ('F11B/SRC/CREWIN.B32',169,65,165,'port_f11b_create_window'),
    ('F11A/SRC/GETFIB.B32',188,60,184,'port_f11a_get_fib'),
    ('F11B/SRC/GETFIB.B32',242,66,238,'port_f11b_get_fib'),
    ('F11A/SRC/MAKACC.B32',198,64,194,'port_f11a_make_access'),
    ('F11B/SRC/MAKACC.B32',194,64,190,'port_f11b_make_access'),
    ('F11B/SRC/DEACCS.B32',312,71,309,'port_f11b_deaccess'),
    ('F11A/SRC/IODONE.MAR',149,100,144,'port_f11a_io_done'),
    ('F11B/SRC/IODONE.MAR',150,100,145,'port_f11b_io_done'),
]
for orig,total,first,last,symbol in f11_87_files:
    regions=[(1,first-1,None,None,'Original notices, definitions and procedure description')]
    if orig == 'F11B/SRC/INIFC2.B32':
        regions.extend([
            (71,142,'src/vms_f11_acp87.c','port_f11b_init_fcb2','File-size and FCB setup'),
            (143,256,'src/vms_f11_acp87.c','port_f11b_fill_fcb','FCB attribute refresh')])
    elif orig == 'F11A/SRC/CREFCB.B32':
        regions.extend([
            (68,123,'src/vms_f11_acp87.c','port_f11a_create_fcb','New FCB'),
            (124,213,'src/vms_f11_acp87.c','port_f11a_update_fcb','FCB attribute refresh')])
    elif orig == 'F11B/SRC/CREFCB.B32':
        regions.extend([
            (70,125,'src/vms_f11_acp87.c','port_f11b_create_fcb','Allocate and materialize FCB; on failed FH2 initialization return the block through source-addressed EXE$DEANONPAGED, verified in QEMU to restore its SRP lookaside entry'),
            (126,178,'src/vms_f11_acp87.c','port_f11b_update_fcb','FCB attribute refresh')])
    elif orig == 'F11B/SRC/DEACCS.B32':
        regions.append((first,last,'src/vms_f11_acp87.c',symbol,
            'Read-only non-expiring DEACCESS unlinks the WCB from its FCB, releases window counters and returns the WCB allocation; this exact MAKACC/DEACCS path is exercised by the live SYS.EXE INIT probe against a real SYS.EXE FCB. Write attributes, expiration, truncate, delete, spool and cleanup branches remain explicitly incomplete.'))
    elif orig == 'F11B/SRC/RDHEDR.B32':
        regions.append((first,last,'src/vms_f11_acp87.c',symbol,
            'Native ACP FID-to-INDEXF lookup and FH2 validation execute during real IA-32 boot; SYS.EXE header is read through published DQA0 DDT/CRB IRQ14 and materialized as a native FCB. Full ACP request queues and process callback context remain unbound.'))
    elif orig == 'F11B/SRC/GETFIB.B32':
        regions.append((first,last,'src/vms_f11_acp87.c',symbol,
            'Native GETFIB returns and guest-checks the real SYS.EXE FID from its allocated WCB during QEMU boot. User FIB/AP mapping, request dispatch and process completion remain unbound.'))
    else:
        regions.append((first,last,'src/vms_f11_acp87.c',symbol,
                        'Native Files-11 ACP routine'))
    regions.append((last+1,total,None,None,'Original module end and blank lines'))
    exe_mapped_source(orig,total,regions,
        'Guest ACP process requests and scheduler must attach live process memory, user arguments, and I/O completion callbacks; the boot probe exercises read-only MAKACC/DEACCS only.')

# M90: INITADP has a native PC CPU nexus replacement and a guest caller.
# VAX-only hardware setup is excluded; ordinary VMS integration is not.
ini_ranges=[
    (1,450,None,None,'VAX CPU-specific SBI nexus descriptors, history and declarations', 'hardware-excluded'),
    (451,605,'src/vms_iniloa_iomap.c','port_ini_iomap','Guest-executed IA-32 ATA/serial I/O registration, CONFREG/SBICONF and boot RPB outputs','partial'),
    (606,639,None,None,'VAX 512-byte SPTE mapping and VAX console halt','hardware-excluded'),
    (640,909,None,None,'VAX UNIBUS adapter I/O address-space mapping, vector and data setup','hardware-excluded'),
    (910,1053,None,None,'VAX MPM, CI, DR32, MBA adapter hardware initialization','hardware-excluded')
]
segs=[]
for first,last,path,symbol,why,status in ini_ranges:
    if not path:
        segs.append({'original_lines':[first,last], 'status':status, 'reason':why})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        segs.append({'original_lines':[first,last],
            'status':status if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':why})
result['files'].append({'original_file':'INILOA/SRC/INITADP.MAR',
    'original_lines':1053,'sections':segs,
    'platform_changes':['VAX SBI nexus and UNIBUS space have no IA-32 hardware counterpart',
      'IA-32 I/O-port mapper performs explicit discovered device probing',
      'Original boot RPB fields and adapter-type array outputs are retained'],
    'unresolved_integration':'IA-32 INI$IOMAP guest entry executes from INILOA.EXE and probes real boot ATA; remaining original adapter types and executive INIT linkages are unfinished.'})

# M90: Track INIT.MAR separately. Only the specific guest-executed
# counterpart ranges are partial: the rest must remain unresolved until
# they are implemented. Do not treat line inventory as completed coverage.
init_parts=[
    (1,427,None,None,'Source header, executive storage and boot tables still need line-by-line work','unresolved'),
    (428,572,'src/kernel.c','kmain','Physical-memory/bootstrap setup and source-dependent resident entry','partial'),
    (573,663,None,None,'Paging, PFN and nonpaged pool setup unresolved','unresolved'),
    (664,730,'src/vms_ini_packet_queues.c','port_ini_packet_init','Live resident IRP/LRP/SRP queue links, sizes and partial-page fragments; IOC$ globals and dynamic pool integration pending','partial'),
    (731,755,None,None,'IPL transition, FIL$ cache handoff and late INIT requirements unresolved','unresolved'),
    (756,837,'src/kernel.c','kmain','Source-ordered SYS.EXE, SYSLOA386, INILOA Files-11 loads and live SYSLOAVEC link; SCSLOA and remaining runtime initialization not complete','partial'),
    (838,859,None,None,'INILOA code deallocation requires full nonpaged pool','unresolved'),
    (860,992,'src/vms_ini_lockdb.c','port_ini_lockdb_init','Realtime SPT bitmap, lock ID chain, power-of-two resource hash and optional process bitmap structures (DYNDEF and consumers unresolved)','partial'),
    (993,1031,'src/vms_ini_procvec.c','port_ini_procvec_init','Resident source PCB pointer/sequence arrays; SYSTEM and NULL entries now point at SYSDEF-layout PCB templates, but their PHD/native contexts are absent','partial'),
    (1032,1068,'src/vms_ini_phv_swap.c','port_ini_phvswap_init','Native PHV/system slot and swapper PTE map DYN record construction; pager and swapper process are not running','partial'),
    (1069,1089,'src/vms_ini_mpw.c','port_ini_mpw_init','Modified-page-writer PTE and PHV index record allocation, no pager consumer','partial'),
    (1090,1141,'src/vms_ini_pflvec.c','port_ini_pflvec_init','Source-derived global page-file count and null PFL shell VBN/window arithmetic; host-only, no operational shell WCB yet','partial'),
    (1142,1165,'src/vms_ini_pflvec.c','port_ini_pflvec_init','Page and swap-file PTR record in loaded SYS.EXE, anchored to resident uninitialized NULLPFL storage; guest-checked, no actual pagefile service, SYSGEN values or shell WCB','partial'),
    (1166,1183,'src/vms_ini_pflvec.c','port_ini_interrupt_stack_limits','Source-defined 512-byte interrupt-stack bounds and bounded lock resource depth executed in loaded guest SYS.EXE using live IA-32 TSS.esp0; native IPL stack and lock manager remain incomplete','partial'),
    (1184,1188,'src/vms_v37_pcb.c','port_v37_pcb_finish_syspcb','INIT_SYSPCB low-word PID update preserving the GENPCB sequence and native PHD/PHYPCB fields; boot currently has no system PHD and cannot activate it','partial'),
    (1189,1212,'src/vms_ini_syswcb.c','port_ini_syswcb_from_boot_map','Actual guest-executed 8-to-6 byte extent compaction in resident SYS.EXE from real Files-11 BOO$L_SYS_MAP, with resident partial native WCB header; original WCB constants and ACP consumers still unresolved','partial'),
    (1213,1231,None,None,'Original VAX PR$_NICR/ICCS/ICR clock-counter timing loop needs real IA-32 hardware equivalent','hardware-excluded'),
    (1232,1239,'src/vms_ini_dptlist.c','port_ini_dpt_attach_three','Source-verified DPTAB metadata bytes are staged in loaded SYS.EXE; actual linked driver images and IOC$GL_DPTLIST remain unresolved','partial'),
    (1240,1255,'src/vms_ini_syslog.c','port_ini_syslog_init','Original DYN$C_LOG=11 typed 48-byte records copied into guest SYS.EXE nonpaged addresses; DPT, boot UCB, SWAPINIT and live logical-name service pending','partial'),
    (1256,1288,'src/vms_ini_ttydrv.c','port_ini_tty_class_relocate','Source DPTAB prologue type/UCB size checks and class vector self-relative relocation; host tested; terminal service and live IOC/DDB/UCB are pending','partial'),
    (1289,1304,'src/vms_ini_ttydrv.c','port_ini_tty_bind_console','Original DEVICEDAT/TTYDEF DDB and OPA0 UCB class/port dispatch fields; host-tested but actual TTDRIVER/OP$DPT images and I/O service not running','partial'),
    (1305,1310,None,None,'Full class-driver and port-driver console registration remain unresolved','unresolved'),
    (1311,1330,'src/vms_ini_bootdev_scan.c','port_ini_bootdev_scan','Original per-adapter-kind count and boot adapter selection executes in resident SYS.EXE using actual INILOA probed IA-32 port entries; original DDB/UCB allocation and driver dispatch not implemented','partial'),
    (1331,1499,None,None,'Legacy boot driver CRB/IDB vector wiring and adapter-specific hardware remain unresolved','unresolved'),
    (1500,1537,'src/vms_ini_fin_commit.c','port_ini_fin_database','Source FIN_DATABASE device unit decimal, counted DQA0 SYS$DISK and concealed SYS$SYSDEVICE records in resident SYS.EXE; LOG$TRANSLATE is not attached','partial'),
    (1538,1589,None,None,'Original IOC$INITDRV, dynamic DDT, CRB/IDB and port-driver binding remain unresolved','unresolved'),
    (1590,1602,'src/vms_ini_fin_ddb_name.c','port_ini_fin_ddb_name','Original FIX_DRV_NAME initializes boot DQA/DQDRIVER counted DDB names after a genuine ATA home sector read; DDT/ACP still unbound','partial'),
    (1603,1906,None,None,'Legacy 386 port requires original driver controller setup and INIT teardown / scheduler / pool integration','unresolved')]
parts=[]
for first,last,path,symbol,purpose,status in init_parts:
    if not path:
        parts.append({'original_lines':[first,last],'status':status,'reason':purpose})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        parts.append({'original_lines':[first,last],
            'status':status if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':purpose})
result['files'].append({'original_file':'SYS/SRC/INIT.MAR',
    'original_lines':1906,'sections':parts,
    'unresolved_integration':'Source FIN_DATABASE names are staged only after a verified real ATA Files-11 read. SYS$DISK/SYS$SYSDEVICE translation is still confined to the bootstrap path; the later native DQA DDT binds only physical PBLK actions, with public channels and ACP absent. Other INIT operations, SYSTEM login, DCL and DECnet remain missing.'})

# M238: PDAT.MAR GENPCB source layout and the static system PCB template.
pdat_rows=[
 (1,90,None,None,'Original PCB/PHD declarations and VAX PHD generation dependencies unresolved','unresolved'),
 (91,170,'src/vms_v37_pcb.c','port_v37_pcb_init_genpcb',"Emit SYSDEF.MDL's 156-byte PCB layout and source GENPCB queue heads, DYN type, state, privileges, PID/PHD and lock queue fields; native PHD creation and process activation remain missing",'partial'),
 (171,220,'src/vms_v37_phd.c','port_v37_phd_init_genphd',"Encode the GENPHD fields in the 376-byte SYSDEFMP.MDL fixed PHD record; native page-table allocation, P0/P1 mapping and process activation remain missing",'partial'),
 (221,254,None,None,'NULL/SWAPPER stack and PHD source construction unresolved','unresolved'),
 (255,256,'src/vms_v37_pcb.c','port_v37_pcb_init_genpcb','Create the source-layout NULL GENPCB record and PID; VMS PHD and native context remain absent','partial'),
 (257,268,None,None,'SWAPPER PHD and stack initialization unresolved','unresolved'),
 (269,272,'src/vms_v37_pcb.c','port_v37_pcb_init_genpcb','Create the source-layout SWAPPER GENPCB record and PID; VMS PHD and native context remain absent','partial'),
 (273,278,None,None,'Remaining SWAPPER source PHD/context setup unresolved','unresolved'),
 (279,281,'src/vms_v37_pcb.c','port_v37_pcb_init_syspcb_template','Construct the unactivated MMG$AL_SYSPCB data template; SYS PHD and native PHYPCB have not been attached','partial'),
 (282,307,'src/vms_ini_procvec.c','port_ini_procvec_init','Initialize source PCB/sequence vectors; native port now installs NULL, SWAPPER and SYSTEM PCB pointers and queues resident COM entries, but process contexts/activation remain unresolved','partial'),
 (308,308,None,None,'Terminal MACRO END directive','excluded')]
pdat_sections=[]
for first,last,path,symbol,description,status in pdat_rows:
    if path:
        ln=line_for_symbol(path,symbol)
        if ln is None:missing+=1
        pdat_sections.append({'original_lines':[first,last],
            'status':status if ln else 'missing','pc_file':path,
            'pc_symbol':symbol,'pc_line':ln,'function':description})
    else:pdat_sections.append({'original_lines':[first,last],
        'status':status,'reason':description})
result['files'].append({'original_file':'SYS/SRC/PDAT.MAR',
    'original_lines':308,'sections':pdat_sections,
    'unresolved_integration':'Native SYS.EXE has source-layout SYSTEM and NULL PCB records and an INI_PSTATE vector pointing to those records. Their PHD/native hardware contexts remain absent, and the separate scheduler records are not bound to VMS processes.'})

# M241: source RSE.MAR executable queue insertion and priority selection.
rse_lines=735 # pinned original SYS/SRC/RSE.MAR line count at REVISION
rse_sections=[]
for first,last,path,symbol,description,status in [
 (1,332,None,None,'RSE wait-state, AST, swapping and synchronization routines remain unresolved except separately mapped source entries','unresolved'),
 (333,342,'src/vms_v37_sched.c','port_v37_sched_chse','Encode source B_PINC and executable-state queue summary behavior; callback side effects and complete source entry conditions remain partial','partial'),
 (343,394,None,None,'SCH$CHSE IPL checks and preceding/following RSE entry paths remain unresolved','unresolved'),
 (395,440,'src/vms_v37_sched.c','port_v37_sched_chse','Implement resident COM/nonresident COMO queue movement, inverted-priority calculation, COMQS summary bits and reschedule/swapper callback points','partial'),
 (441,rse_lines,None,None,'Remaining RSE process state, wait, swap and scheduler routines unresolved','unresolved')]:
    if path:
        ln=line_for_symbol(path,symbol)
        if ln is None:missing+=1
        rse_sections.append({'original_lines':[first,last],
            'status':status if ln else 'missing','pc_file':path,
            'pc_symbol':symbol,'pc_line':ln,'function':description})
    else:rse_sections.append({'original_lines':[first,last],
        'status':status,'reason':description})
result['files'].append({'original_file':'SYS/SRC/RSE.MAR',
    'original_lines':rse_lines,'sections':rse_sections,
    'unresolved_integration':'SCH$CHSE model is now integrated during INIT for resident NULL/SWAPPER queue insertion; actual process context activation, full IPL semantics, swapper execution and the remaining scheduler routines are unresolved.'})

# M107: per-source native assembly-free RTL routines, each saved as its own
# source file with one callable symbol. These are host-tested and built into
# an IA-32 relocatable RTL library, *not* an operational guest RTL or complete
# original-file behavior. Original lines are from the pinned read-only tree.
m107_modules = [
('MTHIIDNNT',130,123),('MTHIIGNNT',127,120),('MTHIIHNNT',125,118),
('MTHJIDNNT',126,120),('MTHJIGNNT',126,120),('MTHJIHNNT',124,118),
('MTHININT',126,119),('MTHJNINT',135,130),
('MTHIISIGN',135,113),('MTHJISIGN',136,114),
('MTHIMAX0',142,116),('MTHIMIN0',138,116),
('MTHJMAX0',142,116),('MTHJMIN0',155,129),
('MTHSIGN',147,124),('MTHDSIGN',152,129),
('MTHGSIGN',139,129),('MTHHSIGN',141,131),
('MTHMAX1',165,134),('MTHMIN1',164,133),
('MTHDMAX1',139,126),('MTHDMIN1',141,128),
('MTHGMAX1',137,124),('MTHGMIN1',139,126),
('MTHHMAX1',141,127),('MTHHMIN1',143,128),
('MTHCONJG',136,128),('MTHDCONJG',136,127),
('MTHGCONJG',136,127),('MTHSGN',196,125),
('COMR50WD',239,132),('COMRAD50',181,139),
('COMIRAD50',186,151),('COMR50ASC',269,205)]
for original,total,entry in m107_modules:
    path='src/native_rtl_m107/'+original.lower()+'.c'
    symbol='port_m107_'+original.lower()
    line=line_for_symbol(path,symbol)
    if line is None:missing+=1
    regions=[]
    if original=='COMR50ASC':
        regions=[
          {'original_lines':[1,96],'status':'excluded','reason':'History, calling conventions, declarations'},
          {'original_lines':[97,137],'status':'partial','pc_file':'src/vms_rtl_m107.c',
           'pc_symbol':'port_m107_r50asc','pc_line':line_for_symbol('src/vms_rtl_m107.c','port_m107_r50asc'),
           'function':'VAX RADIX-50 40-symbol translation table rewritten as index ranges (including unused code 29)'},
          {'original_lines':[138,204],'status':'excluded','reason':'Explanations and calling declarations; executable dispatch begins at R50ASC entry'}]
    elif original in ('MTHMAX1','MTHMIN1'):
        regions=[{'original_lines':[1,entry-1],'status':'excluded','reason':'Source notices, interfaces and declarations'},
                 {'original_lines':[entry,entry+9],'status':'unresolved',
                  'reason':'Original INTEGER*2 and INTEGER*4 return variants with VAX floating conversion not yet mapped'},]
        entry+=10
    else:
        regions=[{'original_lines':[1,entry-1],'status':'excluded','reason':'Source notices, history, calling convention and register declarations'}]
    if original=='MTHSGN':
        regions.append({'original_lines':[entry,180],'status':'partial','pc_file':path,'pc_symbol':symbol,
                        'pc_line':line,'function':'VAX F-float sign comparison; unintegrated VAX entry frame'})
        regions.append({'original_lines':[181,194],'status':'unresolved',
                        'reason':'Original MTH$SGN_R1 assembly register-entry variant not provided by native wrapper'})
    else:
        regions.append({'original_lines':[entry,total-2],'status':'partial' if line else 'missing',
                        'pc_file':path,'pc_symbol':symbol,'pc_line':line,
                        'function':'Native source-specific RTL implementation through exact-format typed helper, tested on host; original VAX calls, exceptions, extra entry variants unresolved'})
    regions.append({'original_lines':[total-1,total],'status':'excluded','reason':'Original assembler END statement and trailing empty line'})
    result['files'].append({'original_file':'RTL/SRC/'+original+'.MAR','original_lines':total,
       'sections':regions,
       'unresolved_integration':('MTHHMIN1 source contains MOVG/CMPG and uses R1 without setup; native H comparison is intent-only, not binary-equivalent. ' if original=='MTHHMIN1' else '')+
        'All M107 functions are IA-32 relocatable objects but host tests only; not linked or invoked by the live VMS guest. Original VAX ABI and exceptional faults need implementation.'})

# M108: 38 new original RTL units, each a separate IA-32 source file.
# Source line counts/entry locations are taken from pinned original reference,
# not from generated Python wrappers. Never infer 100% original functionality.
m108_modules = [
 ('MTHABS',346,128), ('MTHAINT',194,137), ('MTHAMOD',162,137),
 ('MTHANINT',138,128), ('MTHFLOOR',196,121), ('MTHMOD',176,127),
 ('MTHDINT',183,128), ('MTHGINT',185,131), ('MTHHINT',194,139),
 ('MTHDFLOOR',204,125), ('MTHGFLOOR',192,117),('MTHHFLOOR',202,126),
 ('MTHDNINT',138,128),('MTHGNINT',135,125),('MTHHNINT',146,136),
 ('MTHDIM',357,129),('MTHDMOD',162,137),('MTHGMOD',156,131),
 ('MTHHMOD',172,145),('MTHDPROD',138,130),('MTHGPROD',136,128),
 ('MTHBITOPS',480,131),('MTHCABS',156,121),('MTHCDABS',163,127),
 ('MTHCGABS',163,127),('MTHRANDOM',204,165),('FORRANDOM',212,138),
 ('LIBINSQHI',178,153),('LIBINSQTI',178,153),
 ('LIBREMQHI',195,163),('LIBREMQTI',196,164),
 ('STRMOVQ',129,123),('LIBMOVTC',267,162),('LIBMOVTUC',337,163),
 ('LIBCRC',139,126),('LIBEMUL',132,123),('LIBEDIV',170,128),
 ('LIBGETOPC',173,145)]
assert len(m108_modules)==38
for original,total,entry in m108_modules:
    path='src/native_rtl_m108/'+original.lower()+'.c'
    symbol='port108_'+original.lower()
    if original=='MTHABS':symbol='port108_mthabs_i2'
    if original=='MTHDIM':symbol='port108_mthdim_i2'
    if original=='MTHMOD':symbol='port108_mthmod_i2'
    if original=='MTHBITOPS':symbol='port108_mthbitops_i2'
    line=line_for_symbol(path,symbol)
    if line is None:missing+=1
    spans=[{'original_lines':[1,entry-1], 'status':'excluded',
            'reason':'Source notices, history, declarations and VAX argument-frame documentation'}]
    if original=='FORRANDOM':
        spans.append({'original_lines':[entry,184], 'status':'unresolved',
            'reason':'Original FOR$RANDU variants and exact first call state need translation'})
        entry=185
    spans.append({'original_lines':[entry,total-2],
          'status':'partial' if line else 'missing',
          'pc_file':path,'pc_symbol':symbol,'pc_line':line,
          'function':'Original-specific native IA-32 operation. Tested bounded inputs; original complete VAX calling frame, condition signaling, interrupt and guest runtime integration unimplemented.'})
    spans.append({'original_lines':[total-1,total], 'status':'excluded',
          'reason':'Original assembly end directive and trailing blank line'})
    caveat=('Source exact integer numerical subdomain only; arbitrary fractional VAX math unimplemented. '
            if original in ('MTHAMOD','MTHDMOD','MTHGMOD','MTHHMOD','MTHCABS','MTHCDABS','MTHCGABS','MTHDIM') else '')
    caveat+=('MTH$HMIN1 is not part of this batch. Source-defined queue hardware secondary interlocks absent. '
             if original.startswith('LIBINSQ') or original.startswith('LIBREMQ') else '')
    if original=='LIBCRC':caveat+='Reflected polynomial CRC subset only; VAX CRC instruction table is not yet implemented. '
    if original=='FORRANDOM':caveat+='32-bit seed transition tested; VAX F rounding and first-call exactness unresolved. '
    result['files'].append({'original_file':'RTL/SRC/'+original+'.MAR','original_lines':total,
      'sections':spans,'unresolved_integration':caveat+
      'Native ELF32 relocatable RTL only. Not linked into SYS.EXE or guest DCL. Partial source mapping; no VAX CPU emulation.'})

# M109: 36 source-specific native x87 math translations. This is a
# partial domain/representation adapter. The original polynomial tables,
# VAX condition signaling, H-floating 113-bit accuracy, register-entry ABI,
# and real guest DCL/RTL link are NOT supplied. The full source-entry index
# records exactly which routines have corresponding native symbols.
m109_manifest = json.loads((ROOT/'tools/m109_originals.json').read_text())
assert m109_manifest['revision']==REVISION
assert m109_manifest['file_count']==36
m109_seen=set()
for item in m109_manifest['modules']:
    original=item['source']
    assert original not in m109_seen
    m109_seen.add(original)
    total=item['lines']
    entries=item['entries']
    assert entries and all(0<ent['line']<total-1 for ent in entries)
    path='src/native_rtl_m109/'+original.split('/')[-1].lower().replace('.mar','.c')
    assert (ROOT/path).is_file()
    sections=[]
    # Pre-entry source includes real coefficient tables and processing logic,
    # NOT just declarations. Leave those unresolved instead of falsely
    # excluding them from full functional coverage.
    if entries[0]['line']>1:
        sections.append({'original_lines':[1,entries[0]['line']-1],
           'status':'unresolved','reason':'VAX polynomial coefficients, shared assembly helper logic, declarations, and instruction-specific routines not ported'})
    for index,entry in enumerate(entries):
        start=entry['line']
        stop=(entries[index+1]['line']-1 if index+1<len(entries) else total-2)
        assert start<=stop
        symbol=entry['symbol']
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        sections.append({'original_lines':[start,stop],
            'status':'partial' if line else 'missing',
            'pc_file':path,'pc_symbol':symbol,'pc_line':line,
            'function':('Original '+entry['label']+' mathematical operation; '+
              'native IA-32 long double/x87 approximation and strict VAX F/D/G/H word adapter. '+
              'VAX error signaling, coefficient rounding, exact H semantics and full calling frame remain unresolved.')})
    sections.append({'original_lines':[total-1,total],
                     'status':'excluded',
                     'reason':'Original assembler END directive and final blank line; not a runtime instruction'})
    result['files'].append({'original_file':original,'original_lines':total,
        'sections':sections,
        'unresolved_integration':(
          'M109 source-specific wrappers compile to IA-32 ELF32 relocatable code and run '+
          'only in host tests. APPROX deliberately marks non-bit-identical x87 output. '+
          'H-float inputs using precision below 64-bit are rejected; H result low bits '+
          'are approximate. No guest executable links this library. No VAX CPU emulation.')})
assert len(m109_seen)==36
assert len({x['original_file'] for x in result['files']})==len(result['files'])

# M110: 34 additional source-specific, freestanding IA-32 RTL modules.
# All are *partial* numerical adapters or strict table/pool subsets. Their
# VAX register conventions, arithmetic fault dispatch and guest link remain
# unresolved. Earlier M110 ASTDEL/POSTEF line mapping is also partial.
m110_manifest=json.loads((ROOT/'tools/m110_originals.json').read_text())
assert m110_manifest['revision']==REVISION
assert m110_manifest['file_count']==34
m110_seen=set()
for mod in m110_manifest['modules']:
    original=mod['source'];total=mod['lines'];entries=mod['entries']
    assert original not in m110_seen
    m110_seen.add(original)
    assert entries and all(0<z['line']<total-1 for z in entries)
    assert [z['line'] for z in entries]==sorted(z['line'] for z in entries)
    filename=original.rsplit('/',1)[-1].lower().replace('.mar','.c')
    path='src/native_rtl_m110/'+filename
    assert (ROOT/path).is_file()
    modname=filename[:-2]
    prefix='port110_'+modname
    parts=[]
    if entries[0]['line']>1:
        parts.append({'original_lines':[1,entries[0]['line']-1],
            'status':'unresolved','reason':'Original declarations, coefficient tables, VAX hardware data and non-entry code not migrated line-by-line'})
    def entry_symbol(index):
        if mod['kind']=='complex' and modname.endswith(('sinco','sinc')):
            return prefix+('_sin' if index==0 else '_cos')
        if mod['kind']=='sincos':
            return [prefix,prefix+'_sin',prefix+'_cos',prefix+'d',prefix+'_sind',prefix+'_cosd'][index]
        if modname=='mthcvtdg':
            return [None,None,prefix+'_d_g',prefix+'_g_d'][index]
        if modname=='mthconver':
            # Only these source-defined conversion entry points are native;
            # the remaining source variants remain explicitly unresolved.
            mapped={0:prefix+'_dfloti',1:prefix+'_dflotj',2:prefix+'_iint',
                    3:prefix+'_iint',4:prefix+'_jint',5:prefix+'_jint',
                    11:prefix+'_dble',12:prefix+'_sngl'}
            return mapped.get(index)
        if modname=='libef':return prefix+'_'+['get','free','reserve'][index]
        if modname=='liblun':return prefix+'_'+['get','free'][index]
        return prefix
    for index,entry in enumerate(entries):
        first=entry['line']
        last=entries[index+1]['line']-1 if index+1<len(entries) else total-2
        assert first<=last,(original,first,last)
        sym=entry_symbol(index)
        if sym is None:
            parts.append({'original_lines':[first,last],'status':'unresolved',
               'reason':'Source entry '+entry['label']+' not translated; explicit VAX ABI/array operation pending'})
            continue
        line=line_for_symbol(path,sym)
        if line is None:missing+=1
        parts.append({'original_lines':[first,last],
            'status':'partial' if line else 'missing',
            'pc_file':path,'pc_symbol':sym,'pc_line':line,
            'function':'Source entry '+entry['label']+' native IA-32 bounded subset; original machine-code rounding, signal handling, descriptor calling frame and live guest binding absent'})
    parts.append({'original_lines':[total-1,total],
        'status':'excluded','reason':'Original assembler END directive and non-executable final line'})
    result['files'].append({'original_file':original,'original_lines':total,
        'sections':parts,
        'unresolved_integration':('This host-tested source-specific IA-32 RTL module is not called by real guest VMS. '
           'Shared x87 numerical operations are approximations, not DEC polynomial equivalence. '
           'Original VAX format H cannot retain 113-bit precision on an 80-bit x87; '
           'condition signaling, all argument and calling conventions, or full table/EF-LUN system services are not complete.')})
assert len(m110_seen)==34
assert len({x['original_file'] for x in result['files']})==len(result['files'])

# M111: 40 distinct pinned VMS 3.7 RTL sources with explicit partial areas.
# Every entry has one source-specific native function. Claimed partial only:
# VAX condition delivery, runtime descriptors, exception frames, and live guest
# image integration are still absent. This is NOT full functional coverage.
m111=json.loads((ROOT/'tools/m111_originals.json').read_text())
assert m111['revision']==REVISION and m111['file_count']==40
special={
    'OTSPOWDD':['otspowdr','otspowrd','otspowdd'],
    'OTSLUN':['otslun_get','otslun_free','otslun_take'],
    'OTSCVTTLL':['otscvttll_compat','otscvttll'],
    'OTSCVTTIL':['otscvttil_compat','otscvttil'],
    'OTSMOVE':['otsmove','otsmove_r5'],
    'BASMULDDI':['basmulddi_mul','basmulddi_div'],
    'BASCOPYFD':['bascopyfd_f','bascopyfd_d'],
    'BASDET':['basdet_f','basdet_d','basdet_store']
}
m111_seen=set()
for item in m111['modules']:
    name=item['name']; original=item['source']; total=item['lines']
    assert original.startswith('RTL/SRC/') and original.endswith('.MAR')
    assert original not in m111_seen and total>4
    m111_seen.add(original)
    native='src/native_rtl_m111/'+name.lower()+'.c'
    assert (ROOT/native).is_file()
    entries=item['entries']
    assert entries and all(0<e['line']<total-1 for e in entries)
    assert [e['line'] for e in entries]==sorted(e['line'] for e in entries)
    symbols=special.get(name,[name.lower()])
    assert len(symbols)==len(entries),(name,symbols,entries)
    sections=[]
    if entries[0]['line']>1:
        sections.append({'original_lines':[1,entries[0]['line']-1],
            'status':'unresolved',
            'reason':'Original declarations, VAX ABI, condition handlers and source-specific pre-entry control or constants not yet translated'})
    for i,e in enumerate(entries):
        first=e['line'];last=(entries[i+1]['line']-1 if i+1<len(entries) else total-2)
        assert first<=last,(name,first,last)
        symbol='port111_'+symbols[i]
        line=line_for_symbol(native,symbol)
        if line is None:missing+=1
        sections.append({'original_lines':[first,last],
            'status':'partial' if line else 'missing',
            'pc_file':native,'pc_symbol':symbol,'pc_line':line,
            'function':'Native IA-32 bounded subset for original '+e['label']+
                       '; VAX exceptional condition/accuracy, full argument ABI, DEC exactness and live guest dispatch unresolved'})
    sections.append({'original_lines':[total-1,total],
                     'status':'excluded',
                     'reason':'VAX assembler .END directive and final non-executable blank line'})
    result['files'].append({'original_file':original,
        'original_lines':total,'sections':sections,
        'unresolved_integration':('Standalone guest-architecture ELF32 object and native host tests only. '
              'Source-derived functional subset; source pre-entry code unresolved. Exact VAX RTL '
              'fault/condition semantics, FP rounding and descriptor conventions incomplete. '
              'Guest SYS.EXE/DCL/DECnet integration and QEMU execution of these routines absent.')})
assert len(m111_seen)==40
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M112: forty additional original VMS 3.7 FORTRAN RTL programs.
# 30 source-specific MAR entrypoint selectors plus 10 B32 control/dispatcher
# modules, each with a compiled native IA-32 ELF32 callable translation.
# Source-level status remains PARTIAL: original VAX frames, RMS callbacks,
# full FORTRAN error handler, nested LUB stack, guest binding are unfinished.
m112=json.loads((ROOT/'tools/m112_originals.json').read_text())
assert m112['revision']==REVISION and m112['file_count']==40
assert len(m112['modules'])==40
m112_seen=set()
for item in m112['modules']:
    original=item['source'];total=item['lines'];entries=item['entries']
    assert original.startswith('RTL/SRC/FOR') and original not in m112_seen
    m112_seen.add(original)
    native='src/native_rtl_m112/'+item['name'].lower()+'.c'
    assert (ROOT/native).is_file()
    assert entries and all(0<e['line']<total for e in entries)
    sections=[]
    first=entries[0]['line']
    if first>1:
        sections.append({'original_lines':[1,first-1],'status':'unresolved',
          'reason':'Original VAX module declarations, procedure masks, error handlers, VAX RMS LUB and ABI not yet implemented'})
    for i,e in enumerate(entries):
        start=e['line']
        end=entries[i+1]['line']-1 if i+1<len(entries) else total-1
        sym=e['symbol']; lineno=line_for_symbol(native,sym)
        if lineno is None:missing+=1
        sections.append({'original_lines':[start,end],
          'status':'partial' if lineno else 'missing',
          'pc_file':native,'pc_symbol':sym,'pc_line':lineno,
          'function':('Source-specific native statement ordinal and object-format flag dispatch' if item['kind']=='statement-dispatch' else
            'Bounded native FORTRAN LUB/RMS operation via explicit checked backend'),
          'unresolved':'No live RMS/ACP, runtime AST/unwind frames, exact IOSTAT and signal semantics or guest linkage'})
    sections.append({'original_lines':[total,total],'status':'excluded',
       'reason':'Non-executable final module END directive or terminal blank line'})
    result['files'].append({'original_file':original,'original_lines':total,
      'sections':sections,'unresolved_integration':
        'Source selector/host-verified native subset only; no VAX execution. Requires native FORTRAN backend attached to real VMS disk/file system and condition handling; not yet run in QEMU.'})
assert len(m112_seen)==40
assert len({x['original_file'] for x in result['files']})==len(result['files'])

# M113: 40 further pinned original RTL/COBOL/BASIC sources; explicitly
# track full source line extent, with partial native IA-32 subsets only.
# A callable symbol does not equal a complete/booted VAX service.
m113=json.loads((ROOT/'tools/m113_originals.json').read_text())
assert m113['revision']==REVISION and m113['file_count']==40
assert len(m113['modules'])==40
m113_seen=set()
for item in m113['modules']:
    original=item['source'];total=item['lines'];name=item['name']
    assert original.startswith('RTL/SRC/') and original not in m113_seen
    m113_seen.add(original)
    native='src/native_rtl_m113/'+name.lower()+'.c'
    assert (ROOT/native).is_file()
    entries=item['entries']
    assert entries and [e['line'] for e in entries]==sorted(e['line'] for e in entries)
    sections=[]
    if entries[0]['line']>1:
        sections.append({'original_lines':[1,entries[0]['line']-1],
            'status':'unresolved',
            'reason':'Original declarations, VAX calling conventions, condition frames, coefficients or static data not independently line-mapped'})
    for i,e in enumerate(entries):
        first=e['line']
        last=entries[i+1]['line']-1 if i+1<len(entries) else total-1
        assert 1<=first<=last<total,(original,first,last,total)
        sym=e['symbol'];lineno=line_for_symbol(native,sym)
        if lineno is None:missing+=1
        status='missing' if lineno is None else e['status']
        assert status in ('partial','unresolved','missing')
        sections.append({'original_lines':[first,last],
          'status':status,'pc_file':native,'pc_symbol':sym,'pc_line':lineno,
          **({'reason':'Original VAX handler stack/register scan and condition delivery not translated; source-specific IA-32 entry explicitly returns UNSUPPORTED'} if status=='unresolved' else {}),
          'function':'Source-specific bounded native IA-32 subset for VMS RTL source '+name,
          'unresolved':'Exact VAX ABI, condition signals, object formatting, RMS/VMS guest integration or rounding still missing'})
    sections.append({'original_lines':[total,total],'status':'excluded',
                     'reason':'Non-executable VAX MACRO-32 final END directive or terminal blank line'})
    result['files'].append({'original_file':original,'original_lines':total,
        'sections':sections,'unresolved_integration':
        'Source-derived partial RTL functionality compiled as standalone IA-32 objects. '
        'Some entries deliberately return PORT113_UNSUPPORTED. Original VAX ABI, exact DEC '
        'error/condition and rounding semantics and real SYS.EXE/DCL guest linkage absent.'})
assert len(m113_seen)==40
assert len({x['original_file'] for x in result['files']})==len(result['files'])


# M118: DEVICEDAT.MAR source-built record topology. Structural validation
# is guest executed, but fields requiring CRB, DDT, class/port vector and
# live I/O remain expressly unresolved. Do not call this a live device DB.
devicedat_ranges=[
 (1,128,None,None,'Copyright, history, declarations and unported symbol values','unresolved'),
 (129,154,'src/vms_ini_devicedb.c','port_v37_ucb_base','Original fixed UCB header and self-referential I/O queue; other UCB flags depend on original macros','partial'),
 (155,175,'src/vms_ini_devicedb.c','port_v37_ddb_base','Original counted DDB name, linked-list fields and DYN type; no live DDT','partial'),
 (176,205,'src/vms_ini_devicedb.c','port_v37_devices_prelink','IOC$GL_DEVLIST head and DDB source order; ADP and DPT lists not yet registered','partial'),
 (206,208,'src/vms_ini_ddb_boot.c','port_v37_ddb_boot','Boot DDB with F11 ACP identifier and NULL DDT before device driver link','partial'),
 (209,230,'src/vms_ini_ucb_boot.c','port_v37_ucb_boot','Boot disk UCB fields and 30 original extension longwords; DQA DDT/CRB are bound later for physical PBLK only, while full driver behavior and ACP remain unresolved','partial'),
 (231,234,'src/vms_ini_ddb_opa.c','port_v37_ddb_opa','OPA generic DDB, source counted OPERATOR name; no class DDT','partial'),
 (235,263,'src/vms_ini_ucb_opa.c','port_v37_ucb_opa','OPA0 UCB layout, 320-byte source size, online template and line size; class and port vectors missing','partial'),
 (264,353,None,None,'Original console CRB, IDB, VAX interrupts and PC UART adaptation still need original-value derivation','unresolved'),
 (354,356,'src/vms_ini_ddb_mb.c','port_v37_ddb_mb','MBA DDB name and next link; MB driver dispatch not linked','partial'),
 (357,391,None,None,'Mailbox clone template and all original mailbox characteristics unresolved','unresolved'),
 (392,423,'src/vms_ini_ucb_mb1.c','port_v37_ucb_mb1','Source job-controller MBA1 template fields and next unit pointer; no mailbox service','partial'),
 (424,453,'src/vms_ini_ucb_mb2.c','port_v37_ucb_mb2','Source operator MBA2 template fields; no mailbox service','partial'),
 (454,460,'src/vms_ini_ddb_nl.c','port_v37_ddb_nl','NLA DDB tail node, no DDT until IOC live','partial'),
 (461,478,'src/vms_ini_ucb_nl.c','port_v37_ucb_nl','NLA0 template, I/O queue, source status and owner; not connected to $QIO','partial'),
 (479,572,None,None,'Network WCB and later source tables unresolved','unresolved')]
rows=[]
for lo,hi,pc,sym,why,status in devicedat_ranges:
    if pc:
        line=line_for_symbol(pc,sym)
        if line is None:missing+=1
        rows.append({'original_lines':[lo,hi], 'status':status if line else 'missing',
                     'pc_file':pc,'pc_symbol':sym,'pc_line':line,
                     'function':why})
    else: rows.append({'original_lines':[lo,hi],'status':status,'reason':why})
result['files'].append({'original_file':'SYS/SRC/DEVICEDAT.MAR','original_lines':572,
    'sections':rows,'unresolved_integration':'Original source DDB/UCB records are in resident SYS.EXE; native NL DDT/FDT now bound to NLA0. BOOT/OPA/MBA DDTS, CRBs, $QIO and IOPOST AST dispatch absent.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])


# M122: source VAX MACRO-32 DDTAB and FUNCTAB are software driver
# metadata, stored as actual native-endian record bytes in loaded SYS.EXE.
result['files'].append({'original_file':'SHRLIB/SYSMAR.MAR','original_lines':1573,
    'sections':[
      {'original_lines':[1,215],'status':'unresolved',
       'reason':'Original VAX macros and declarations require full native translation'},
      {'original_lines':[216,241],'status':'partial',
       'pc_file':'src/vms_nl_ddt_build.c','pc_symbol':'port_v37_nl_ddt_build',
       'pc_line':line_for_symbol('src/vms_nl_ddt_build.c','port_v37_nl_ddt_build'),
       'function':'DDTAB expansion for software NL driver only; missing MNTVER and other drivers'},
      {'original_lines':[242,502],'status':'unresolved',
       'reason':'Other original source macros and VAX hardware translation unresolved'},
      {'original_lines':[503,546],'status':'partial',
       'pc_file':'src/vms_nl_ddt_masks.c','pc_symbol':'port_v37_nl_ddt_masks',
       'pc_line':line_for_symbol('src/vms_nl_ddt_masks.c','port_v37_nl_ddt_masks'),
       'function':'FUNCTAB / GENRADDR software null FDT legal/buffered/read/write relative masks'},
      {'original_lines':[547,1573],'status':'unresolved',
       'reason':'Remaining original VAX macro service runtime and restrictions unresolved'}],
    'unresolved_integration':'Only source NL FDT/DDT table bytes implemented; original macro assembler library and boot services far from complete.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M119: original CONINTDSP.MAR console-port action routines. The six
# source routines are directly callable IA-32, but TERMINALIO and the
# class-driver GETNXT/PUTNXT remain absent, so there is NO live tty output.
con_original=next(f for f in result['files'] if f['original_file']=='SYS/SRC/CONINTDSP.MAR')
con_rows=[
 (1,82,None,None,'Original VAX declarations and console hardware setup unresolved','unresolved'),
 (83,97,'src/vms_ini_dpt_op.c','port_v37_dpt_stage_op','Original OP source DPT prologue metadata only, no linked driver','partial'),
 (98,483,None,None,'Original VAX console ISR, TERMINALIO, PDR and hardware output dispatch still unresolved','unresolved'),
 (484,487,'src/vms_con_xoff.c','port_v37_con_xoff','Schedule source XOFF hold, defer TERMINALIO until class ready','partial'),
 (488,500,'src/vms_con_xon.c','port_v37_con_xon','Schedule source XON hold, defer physical output until class ready','partial'),
 (501,508,'src/vms_con_stop.c','port_v37_con_stop','Original Ctrl-S state and STOP hold transition','partial'),
 (509,518,'src/vms_con_stop2.c','port_v37_con_stop2','Original broadcast STOP2, TIM status and two-tick due time','partial'),
 (519,526,'src/vms_con_abort.c','port_v37_con_abort','Clear BURST hold and set 2 tick timeout','partial'),
 (527,550,'src/vms_con_resume.c','port_v37_con_resume','Resume Ctrl-S, clear STOP flags and compute burst timeout; no live terminal class dispatch','partial'),
 (551,556,None,None,'Original CON_END, VAX driver finishing and assembler end','unresolved')]
con_sections=[]
for lo,hi,pc,symbol,why,status in con_rows:
    if pc:
        line=line_for_symbol(pc,symbol)
        if line is None:missing+=1
        con_sections.append({'original_lines':[lo,hi], 'status':status if line else 'missing',
            'pc_file':pc,'pc_symbol':symbol,'pc_line':line,'function':why})
    else:con_sections.append({'original_lines':[lo,hi],'status':status,'reason':why})
con_original['sections']=con_sections
con_original['unresolved_integration']='Source port-action state changes are linked into real SYS.EXE and guest-tested on a diagnostic OPA0 copy only. Original class driver, TERMINALIO, output interrupt service and IOC are absent.'
# SHRLIB/TTYDEF.MAR literal source UCB fields, not the entire terminal class.
tty_rows=[
 (1,244,None,None,'Original terminal-driver macro and constant definitions not ported in full','unresolved'),
 (245,267,'src/vms_con_port.c','port_v37_con_valid','Source 132-byte UCB base and terminal state/extension validity','partial'),
 (268,298,None,None,'Other terminal AST and typeahead fields not ported','unresolved'),
 (299,311,'src/vms_con_xon.c','port_v37_con_xon','Original HOLD tank XON/XOFF/STOP/STOP2/BURST bit values','partial'),
 (312,348,None,None,'Cursor, defaults and queued terminal output still missing','unresolved'),
 (349,351,'src/vms_con_resume.c','port_v37_con_resume','Original terminal output length used to time burst restart','partial'),
 (352,447,None,None,'Remaining modem speed and char defaults not implemented','unresolved'),
 (448,449,'src/vms_con_port.c','port_v37_con_valid','Source terminal UCB length of 256 bytes','partial'),
 (450,781,None,None,'Terminal class vectors and remaining macros require original service translation','unresolved')]
sec=[]
for lo,hi,pc,sym,why,status in tty_rows:
    if pc:
        line=line_for_symbol(pc,sym)
        if line is None:missing+=1
        sec.append({'original_lines':[lo,hi],'status':status if line else 'missing',
            'pc_file':pc,'pc_symbol':sym,'pc_line':line,'function':why})
    else:sec.append({'original_lines':[lo,hi],'status':status,'reason':why})
result['files'].append({'original_file':'SHRLIB/TTYDEF.MAR','original_lines':781,
    'sections':sec,'unresolved_integration':'Only source UCB extension offsets and tank flags are mapped; no complete terminal-driver class or port runtime.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M120/M255: source IOC$SEARCHDEV/GEN/ALC plus IOC$FFCHAN free CCB scan.
# This resolves real source-format DEVICEDAT DDB and UCB pointers in SYS.EXE,
# NOT a $QIO service. LOG$TRANSLATE, mailbox allocation, device mutex, actual
# DDTs and process ownership are still unresolved and fail closed.
ioc_rows=[
 (1,215,None,None,'Original IOC queues, channel tables and declarations unresolved','unresolved'),
 (216,248,'src/vms_v37_ccb.c','port_v37_ccb_find_free','IOC$FFCHAN source scan of CCB$B_AMOD from CTL$GL_CCBBASE, first free 16-byte channel token and CHINDX high-water update','partial'),
 (249,295,None,None,'Original IOC lock and device search entry context unresolved','unresolved'),
 (296,298,'src/vms_ioc_searchgen.c','port_v37_ioc_searchgen','IOC$SEARCHGEN generic mode; real device availability waits for full ownership and mount privileges','partial'),
 (299,301,'src/vms_ioc_searchalc.c','port_v37_ioc_searchalc','IOC$SEARCHALC generic allocation mode; no process ownership or ACL checks','partial'),
 (302,325,'src/vms_ioc_searchdev.c','port_v37_ioc_searchdev','IOC$SEARCHDEV original physical entry, bounded read-only native source record lookup','partial'),
 (326,348,'src/vms_devlog_input.c','port_devlog_input','Source descriptor length and case-folded owned scratch before LOG$TRANSLATE, SYSTEM-only','partial'),
 (349,361,'src/vms_devlog_normalize.c','port_devlog_normalize','Source bounded ten-iteration LOG$TRANSLATE loop on two original SYSTEM boot entries','partial'),
 (362,384,'src/vms_devlog_underscore.c','port_devlog_underscore','Leading underscore bypasses further translation; double underscore records CONCEALED, RMS PPF path absent','partial'),
 (385,410,'src/vms_devlog_colon.c','port_devlog_colon','Colon-limited lookup and nonlocal device rejection before original IOC DDB walk; RMS file syntax unavailable','partial'),
 (411,427,'src/vms_ioc_parse.c','port_v37_ioc_parse','Parse source uppercase controller letters before digits with strict bounds','partial'),
 (428,444,'src/vms_ioc_unit.c','port_v37_ioc_parse_unit','Original 16-bit decimal unit name with overflow check','partial'),
 (445,460,'src/vms_ioc_parse.c','port_v37_ioc_parse','Reject nonlocal and malformed device names; original VMS status constants pending','partial'),
 (461,490,'src/vms_devlog_search.c','port_devlog_search','Source LOG$TRANSLATE SYSTEM bootstrap followed by prelinked DDB/UCB result; no $ASSIGN, IOC mutex or ACP','partial'),
 (491,503,'src/vms_ioc_ddb.c','port_v37_ioc_ddb','Source DDB$L_LINK counted-name search guarded against corrupt pointers','partial'),
 (504,531,'src/vms_ioc_ucb.c','port_v37_ioc_ucb','Source UCB$L_LINK unit matching and fail-closed unallocated generic mode','partial'),
 (532,604,None,None,'Original device I/O rest of source not yet ported','unresolved')]
rows=[]
for lo,hi,pc,sym,desc,status in ioc_rows:
    if pc:
        line=line_for_symbol(pc,sym)
        if line is None:missing+=1
        rows.append({'original_lines':[lo,hi],'status':status if line else 'missing',
            'pc_file':pc,'pc_symbol':sym,'pc_line':line,'function':desc})
    else:rows.append({'original_lines':[lo,hi],'status':status,'reason':desc})
result['files'].append({'original_file':'SYS/SRC/IOSUBPAGD.MAR','original_lines':604,
    'sections':rows,'unresolved_integration':'Original IOC$SEARCHDEV calls resident bounded SYSTEM-only LOG$TRANSLATE followed by typed DDB/UCB scan. SYS$DISK/SYS$SYSDEVICE reach unbound DQA0; the DQ DDT, IOC mutex, $ASSIGN/$QIO, process/group logical tables, full RMS and ACP remain unported.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M277-M278: SYSASSIGN.MAR output probe/clear, descriptor read/MAXACMODE and
# local device binding. Mailbox, network and remote paths remain incomplete;
# no public service entry/process-memory binding is wired.
assign_rows=[
 (1,180,None,None,'System-service argument and mailbox-descriptor handling remain unresolved','unresolved'),
 (181,182,'src/vms_sysassign.c','port_v37_sysassign_descriptor_service','Probes and clears channel output before descriptor reads and publishes the assigned channel only after success; tested through translated IA-32 pages, but no live process map is bound','partial'),
 (183,187,None,None,'Mailbox descriptor copy and TEST_MAILBOX remain unresolved','unresolved'),
 (188,194,'src/vms_sysassign.c','port_v37_sysassign_descriptor','Returns source IVLOGNAM/ACCVIO for invalid length/pointer, maps stage-qualified IOC/CCB errors to IVDEVNAM/NOSUCHDEV/NOIOCHAN/NOPRIV/DEVALLOC, copies descriptor/name through checked caller-mode page translations, applies MAXACMODE=max(requested,current), and uses source-PCB-bound process CCB state when supplied','partial'),
 (195,195,'src/vms_v37_ccb.c','port_v37_ccb_find_free','IOC$FFCHAN scans the source byte-offset channel table and advances CHINDX after descriptor access-mode maximization','partial'),
 (196,210,None,None,'Mailbox TEST_MAILBOX, spooled/template and error routing unresolved','unresolved'),
 (211,212,'src/vms_sysassign.c','port_v37_sysassign_local','Local physical IOC$SEARCHDEV is composed with the source CCB assignment helper; public service-vector binding remains unresolved','partial'),
 (213,228,None,None,'DEVICE FOUND branch routing for spooled/network/template UCBs and caller privilege checks unresolved','unresolved'),
 (229,265,'src/vms_v37_ccb.c','port_v37_ccb_assign_local','Local UCB owner ancestry, share/spool allocation policy, access callback and implicit PID allocation under I/O database lock','partial'),
 (266,299,'src/vms_v37_ccb.c','port_v37_ccb_assign_local','Local CCB UCB/AMOD/STS binding and UCB reference increment; mailbox association and FOD/share mailbox handling remain unsupported','partial'),
 (300,346,None,None,'NETMBX privilege, network UCB creation, byte quota, unit initialization, mailbox validation and rollback unresolved','unresolved'),
 (347,415,None,None,'Remote assignment, SYS$QIO connection, completion wait and TEST_MAILBOX are not implemented','unresolved')]
assign_sections=[]
for lo,hi,pc,sym,description,status in assign_rows:
    if pc:
        lineno=line_for_symbol(pc,sym)
        if lineno is None:missing+=1
        assign_sections.append({'original_lines':[lo,hi],'status':status if lineno else 'missing',
            'pc_file':pc,'pc_symbol':sym,'pc_line':lineno,'function':description})
    else:assign_sections.append({'original_lines':[lo,hi],'status':status,'reason':description})
result['files'].append({'original_file':'SYS/SRC/SYSASSIGN.MAR','original_lines':415,
    'sections':assign_sections,'unresolved_integration':'Native helpers probe/clear/write channel output through the translated caller-memory adapter, parse a VMS string descriptor, maximize requested access mode, search DQA0, and perform local UCB ownership/refcount/CCB mutation. The resident path can bind the process PID/ARB from a source PCB and its CCB table/high-water state from a process CTL allocation; a host integration test exercises that binding and mapped user pointers. No running scheduler process supplies its page map or CTL allocation to a public SYS$ASSIGN service entry. Mailbox handling, network UCB creation and remote assignment remain unresolved.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M121: original SYSQIOREQ.MAR EXE$FINISHIOC only. The pending
# IOC$GL_PSBL append is real source-format resident IRP data, but public
# $QIO setup, IOC driver dispatch and IOPOST delivery have not been ported.
qio_rows=[
 (1,111,None,None,'Original declarations and pre-argument-offset service implementation unresolved','unresolved'),
 (112,123,'src/vms_sysqio_args.c','port_v37_sysqio_decode_args','Fixed SYS$QIO argument-list offsets are decoded; event flag, channel, privilege, device and I/O processing remain unimplemented','partial'),
 (124,148,None,None,'Original CCB access-pending detection and event/channel processing unresolved','unresolved'),
 (149,156,'src/vms_sysqio_args.c','port_v37_sysqio_begin','When CCB$L_WIND bit 0 marks access/deaccess pending, require a scheduler wait callback and resume only after the pending bit clears; no live SCH$WAIT/RSN$_ASTWAIT binding yet','partial'),
 (157,238,None,None,'Original spooled-device selection, channel lookup, local/global event handling and remaining service prologue unresolved','unresolved'),
 (239,243,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_user_irp','Validates/copies the fixed VMS AP list through checked caller-mode page translations, then checks READ/WRITE transfer access after IRP reservation and rolls back packet/I/O/AST charges on ACCVIO; no live system-service trap dispatch or driver submission yet','partial'),
 (244,247,'src/vms_sysqio_args.c','port_v37_sysqio_begin','Clear EFN before channel lookup and post failure status to the requested event flag','partial'),
 (248,252,'src/vms_sysassign.c','port_v37_sysassign_lookup_qio_channel','Resolve the source channel token from a PID-bound CCB table, verify the assigned UCB and bind its native DQ FDT/device view; local DQA0 SYS$ASSIGN helper composes IOC$SEARCHDEV with CCB ownership/refcount. Public service-vector dispatch remains absent','partial'),
 (253,283,'src/vms_sysqio_args.c','port_v37_sysqio_preflight','Host integration now passes an IOC$SEARCHDEV-assigned DQA0 CCB through channel lookup into CCB AMOD/FDT checks. Native access-mode comparison, DQ FDT legality and offline exceptions are implemented; wait callback is not bound to live SCH$WAIT','partial'),
 (284,285,None,None,'Original IOSB branch prologue unresolved','unresolved'),
 (286,292,'src/vms_sysqio_iosb.c','port_v37_sysqio_clear_iosb','Nullable IOSB, full 8-byte write probe before atomic two-longword clear; caller memory copy wiring remains unresolved','partial'),
 (293,299,None,None,'EXE$QIO resource-charge setup and AST-deletion IPL transition unresolved','unresolved'),
 (300,312,'src/vms_sysqio_args.c','port_v37_sysqio_begin','Direct/buffered per-process I/O counter charge with single-unit quota callback and rollback on denial; IRP reservation/dispatch remain unresolved','partial'),
 (313,335,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_irp','Join service begin with standard IRP/AST reservation; on reservation failure restore direct/buffered process I/O count and post the service error event; channel IOC and driver dispatch remain unresolved','partial'),
 (336,345,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_user_irp','Allocate/build the standard IRP, pin the caller IOSB and transfer pages, and on ACCVIO return the packet, release acquired pins, restore I/O/AST charges and post the event; the tested pending registry can take ownership keyed by PID/IRP before dispatch, but live DDT submission remains absent','partial'),
 (346,385,None,None,'Source window resolution, buffered user-buffer mapping and complete I/O-quota rollback unresolved','unresolved'),
 (386,429,'src/vms_sysqio_irp_build.c','port_v37_sysqio_build_irp','Build source-layout 136-byte IRP with negative CCB offset, ARB/fork fields and conditional ACB quota charge; IRP$B_PRI comes from the bound PCB$B_PRIB as in SUCCES, and stale/malformed PCB identity is rejected. Native channel accounting and driver dispatch remain unbound','partial'),
 (430,449,None,None,'Optional I/O measurement start, sequence counter update and function dispatch setup unresolved','unresolved'),
 (450,476,'src/vms_sysqio_normalize.c','port_v37_sysqio_normalize_write','Source FOD/MNT/FOR WRITEVBLK boot-window handling, logical WRITE conversion, NOPRIV status and AST-quota branch predicate; live device-characteristics/PCB wiring remains unresolved','partial'),
 (477,479,'src/vms_sysqio_ast.c','port_v37_sysqio_check_ast_quota','Reject negative ASTCNT only when the request has an AST, posting SS$_EXQUOTA status to the caller; real PCB and abort/post integration unresolved','partial'),
 (480,514,None,None,'I/O transaction measurement sequence and transition into device access policy unresolved','unresolved'),
 (515,535,'src/vms_sysqio_pcb.c','port_v37_sysqio_process_bind_pcb','SHRLIB IFPRIV tests the current process PHD PRIVMSK, not PCB$Q_PRIV; native binding derives PHY_IO/LOG_IO from the bound PHD mask and makes PHY_IO imply LOG_IO. SYS$QIO service dispatch remains incomplete','partial'),
 (536,536,None,None,'EXE$ABORTIO status setup unresolved','unresolved'),
 (537,540,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_reserve_irp','After allocation, reject exhausted AST quota and return the allocated IRP to its source lookaside','partial'),
 (541,544,None,None,'Remaining EXE$ABORTIO channel IOC rollback, IOSB/event completion and process post integration unresolved','unresolved'),
 (545,547,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_irp','After the source access check, set IRP$M_PHYSIO in the reserved standard IRP only when the original privilege/device policy grants physical I/O','partial'),
 (548,569,None,None,'DIAGNOSE privilege validation, driver diagnostic-buffer allocation and IRP diagnostic-buffer fields unresolved','unresolved'),
 (570,581,'src/vms_sysqio_device_access.c','port_v37_sysqio_device_access','PHD PRIVMSK derived LOG_IO/PHY_IO flags and SPL/FOD/MNT/FOR/SHR gates select volume access callback; real live UCB security binding remains incomplete','partial'),
 (582,735,None,None,'Remaining device FDT action dispatch, physical/diagnostic behavior and request queue/completion unresolved','unresolved'),
 (736,740,'src/vms_qio_abortio.c','port_v37_qio_abortio','Clear IRP IOSB pointer, return AST quota reservation once, append IOC post queue and request IPL4; live PCB lookup and process post delivery remain unbound','partial'),
 (741,742,'src/vms_qio_finishioc.c','port_v37_qio_finishioc','EXE$FINISHIOC clears R1 before common finish path','partial'),
 (743,749,'src/vms_qio_finishio.c','port_v37_qio_finishio','UCB OPCNT, IRP media status, append IOC post FIFO and defer IPL4','partial'),
 (750,770,None,None,'Source documentation between finish and device-queue action','unresolved'),
 (771,773,'src/vms_sysqio_pending.c','port_v37_sysqio_submit','Transfer prepared process page-pin ownership to the pending registry before invoking the driver callback; rejected callbacks release the transferred pins. The callback still lacks a live DQA0 priority queue/IOC$INITIATE binding','partial'),
 (774,802,None,None,'ALTSTART action dispatch and routine documentation unresolved','unresolved'),
 (803,831,None,None,'Alternate driver start path and IPL handling unresolved','unresolved'),
 (832,859,None,None,'ACP queue and ACP process wake path unresolved','unresolved'),
 (860,868,'src/vms_dq_ioc_unit.c','port_dq_ioc_unit_submit','Native unit admission serializes retained private IRPs with SYSDEF UCB$W_STS.BSY at source offset 88, raises/restores interrupt exclusion, defers packets while active, and dispatches next after IOC post; public process SYS$QIO/live pointer IRPs remain unbound','partial'),
 (869,896,None,None,'EXE$INSERTIRP source description and calling convention','excluded'),
 (897,905,'src/vms_irp_queue.c','port_v37_irp_queue_insert','Implement SYSQIOREQ EXE$INSERTIRP with source 32-bit intrusive IRP/UCB forward/back links, unsigned byte-priority comparison and FIFO ordering for equal priorities; live EXE$QIODRVPKT/DQA submission remains unbound','partial'),
 (906,908,None,None,'VAX assembler end directive and blank lines','excluded')]
qio_sec=[]
for lo,hi,pc,sym,description,status in qio_rows:
    if pc:
        lineno=line_for_symbol(pc,sym)
        if lineno is None:missing+=1
        qio_sec.append({'original_lines':[lo,hi],'status':status if lineno else 'missing',
                        'pc_file':pc,'pc_symbol':sym,'pc_line':lineno,'function':description})
    else:qio_sec.append({'original_lines':[lo,hi],'status':status,'reason':description})
result['files'].append({'original_file':'SYS/SRC/SYSQIOREQ.MAR',
    'original_lines':908,'sections':qio_sec,
    'unresolved_integration':'The native AP decoder preserves fixed argument offsets, and the caller-memory entry helper validates/copies the full argument list through IA-32 page translations before invoking SYSQIOREQ preparation; the host regression also verifies an inaccessible AP fails with ACCVIO before charging I/O. Source-ordered begin clears EFN, checks channel access/FDT/offline rules, probes/clears IOSB, charges direct/buffered I/O counts and posts validation failures. The reserve helper obtains a source INIT IRP packet, enforces AST quota, builds the 136-byte standard IRP and returns it on post-allocation failure. WRITEVBLK normalization and PHY_IO/LOG_IO gates preserve source device-state decisions. A bounded PID/IRP pending table can take ownership of the prepared IOSB and pinned transfer pages and is exercised through the DIRPOST callback, but no live system-service trap dispatch or DQA0 queue submission is connected; source IPL/deletion protection, EXE$ABORTIO service post, buffered window mapping and process AST scheduling remain incomplete.'})
result['files'].append({'original_file':'SYS/SRC/SYSQIOFDT.MAR',
    'original_lines':868,
    'sections':[
      {'original_lines':[1,176],'status':'unresolved','reason':'SYSQIOFDT setup, FDT dispatch and routines before the one-parameter action remain unported'},
      {'original_lines':[177,179],'status':'partial','pc_file':'src/vms_sysqio_irp_build.c','pc_symbol':'port_v37_sysqio_build_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_build.c','port_v37_sysqio_build_irp'),'function':'EXE$ONEPARM copies P1 to IRP$L_MEDIA when the device FDT selects the one-parameter action; host test verifies the exact packet field'},
      {'original_lines':[180,248],'status':'unresolved','reason':'Zero-parameter action and pre-read/write SYSQIOFDT routines remain unported'},
      {'original_lines':[249,250],'status':'partial','pc_file':'src/vms_sysqio_irp_reserve.c','pc_symbol':'port_v37_sysqio_prepare_user_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_user_irp'),'function':'Pins all translated IA-32 pages for a READ destination after IRP reservation and rolls back on failure; VMS SVAPTE/BOFF mapping, IOLOCK restart/fault semantics and live submission remain unresolved'},
      {'original_lines':[251,251],'status':'partial','pc_file':'src/vms_sysqio_irp_build.c','pc_symbol':'port_v37_sysqio_build_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_build.c','port_v37_sysqio_build_irp'),'function':'Sets IRP$V_FUNC for the classified READ FDT action; following source lock/access checks remain unresolved'},
      {'original_lines':[252,253],'status':'partial','pc_file':'src/vms_sysqio_irp_reserve.c','pc_symbol':'port_v37_sysqio_prepare_user_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_user_irp'),'function':'Pins all translated IA-32 pages for a WRITE source after IRP reservation and rolls back on failure; VMS SVAPTE/BOFF mapping, IOLOCK restart/fault semantics and live submission remain unresolved'},
      {'original_lines':[254,254],'status':'partial','pc_file':'src/vms_sysqio_irp_build.c','pc_symbol':'port_v37_sysqio_build_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_build.c','port_v37_sysqio_build_irp'),'function':'Copies low byte of P4 to IRP$B_CARCON for classified read/write FDT actions'},
      {'original_lines':[255,259],'status':'unresolved','reason':'Physical function normalization in the common FDT path remains incomplete'},
      {'original_lines':[260,261],'status':'partial','pc_file':'src/vms_sysqio_irp_build.c','pc_symbol':'port_v37_sysqio_build_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_build.c','port_v37_sysqio_build_irp'),'function':'Zero-extends low word P2 into IRP$L_BCNT for classified read/write actions; zero count leaves BCNT clear'},
      {'original_lines':[262,264],'status':'unresolved','reason':'P1 buffer accessibility, page locking and EXE$QIODRVPKT submission remain unported'},
      {'original_lines':[265,364],'status':'unresolved','reason':'Remaining SYSQIOFDT routines and buffer-check setup unresolved'},
      {'original_lines':[365,380],'status':'unresolved','reason':'READLOCK/WRITELOCK address probing, page locking, BOFF and SVAPTE population unresolved'},
      {'original_lines':[381,527],'status':'unresolved','reason':'Page locking and source buffer-check setup remain unported'},
      {'original_lines':[528,545],'status':'unresolved','reason':'Additional EXE$READCHKR/WRITECHKR source documentation and surrounding code remain unported'},
      {'original_lines':[546,553],'status':'partial','pc_file':'src/vms_sysqio_irp_reserve.c','pc_symbol':'port_v37_sysqio_prepare_user_irp','pc_line':line_for_symbol('src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_user_irp'),'function':'Runs the helper after IRP reservation and returns packet/quota state on inaccessible READ/WRITE pages; DQA ACP MODIFY remains a separate FDT action'},
      {'original_lines':[554,554],'status':'unresolved','reason':'Source page boundary directive'},
      {'original_lines':[555,578],'status':'partial','pc_file':'src/vms_userptr.c','pc_symbol':'port_user_io_range_valid','pc_line':line_for_symbol('src/vms_userptr.c','port_user_io_range_valid'),'function':'Validates address overflow and every IA-32 user PDE/PTE across up to 65,535 bytes; native page size replaces VAX page stepping'},
      {'original_lines':[579,584],'status':'partial','pc_file':'src/vms_sysqio_args.c','pc_symbol':'port_v37_sysqio_check_buffer','pc_line':line_for_symbol('src/vms_sysqio_args.c','port_v37_sysqio_check_buffer'),'function':'Returns SS$_ACCVIO without changing the output buffer plan when any translated user page lacks required access'},
      {'original_lines':[585,618],'status':'unresolved','reason':'Remaining source documentation and WRITECHKR setup unresolved'},
      {'original_lines':[619,625],'status':'partial','pc_file':'src/vms_sysqio_args.c','pc_symbol':'port_v37_sysqio_check_buffer','pc_line':line_for_symbol('src/vms_sysqio_args.c','port_v37_sysqio_check_buffer'),'function':'Selects read access for user WRITE buffers; zero count skips probing, with byte count populated in standard IRP construction'},
      {'original_lines':[626,626],'status':'unresolved','reason':'Source page boundary directive'},
      {'original_lines':[627,649],'status':'partial','pc_file':'src/vms_userptr.c','pc_symbol':'port_user_io_range_valid','pc_line':line_for_symbol('src/vms_userptr.c','port_user_io_range_valid'),'function':'Checks each page of the complete 16-bit transfer range, including source large-buffer chunk cases'},
      {'original_lines':[650,868],'status':'unresolved','reason':'Modify/window and remaining SYSQIOFDT FDT routines unresolved'}],
    'unresolved_integration':'Native IRP construction preserves ONE action P1 media, READ status bit, CARCON from P4, and BCNT from zero-extended P2 for classified read/write actions. The caller-memory preparation path checks READ writable-buffer and WRITE readable-buffer access across all native pages after reservation and rolls back the packet plus process charges on ACCVIO. DQA ACP MODIFY actions are not interpreted as SYSQIOFDT MODIFY memory actions. Page locking, IRP BOFF/SVAPTE setup, live FDT dispatcher/QIODRVPKT queue, fault/restart and asynchronous abort completion, and remaining SYSQIOFDT functions remain incomplete.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M123: IOCIOPOST kernel-end-action subset executed in real SYS.EXE.
# Process paths, PCB counters, IOSB delivery, ASTs, paged/swap I/O and real
# IPL interrupt dispatch remain unresolved.
iopost_rows=[
 (1,114,None,None,'DEC headers, VAX registers, priority table and original declarations','unresolved'),
 (115,144,'src/vms_sys_executive_entry.c','executive_iopost_irq_entry','Native supervisor IDT software INT 0x90 invokes real SYS.EXE queue worker; no process AST','partial'),
 (145,155,'src/vms_iopost_pop.c','port_v37_iopost_pop','First-in/first-out REMQUE and sentinel restoration, now called through x86 CPL0 IDT 0x90','partial'),
 (156,159,'src/vms_iopost_kernel_end.c','port_v37_iopost_kernel_end','Whitelisted native privileged end action; other VAX branch and kernel ABI unresolved','partial'),
 (160,162,'src/vms_iopost_service.c','port_v37_iopost_service','Negative PID/end-action kernel branch is limited to the checked IA-32 supervisor aperture; other kernel actions remain fail-closed','partial'),
 (163,170,'src/vms_iopost_process_direct.c','port_v37_iopost_process_direct','Resolve positive PID process index through source PCB vector, verify PCB sequence, and charge PCB$W_DIOCNT before direct completion; buffered route remains deferred','partial'),
 (171,210,'src/vms_iopost_unlock.c','port_v37_iopost_unlock_direct','DIRECT I/O path computes locked pages from byte offset/count and unlocks both buffer areas in each bounded IRPE chain; paging/swap and segmented virtual requests remain deferred','partial'),
 (211,971,None,None,'Remaining original buffered/paging postprocessing, PCB quotas, kernel AST and virtual segmentation unported','unresolved'),
 (972,975,'src/vms_iopost_account.c','port_v37_iopost_account_phd','DIRPOST selects PHD$L_DIOCNT or BIOCNT from the IRP buffered bit and increments the source 32-bit accounting counter; the caller must resolve SCH$GL_CURPCB PHD','partial'),
 (976,989,None,None,'Optional I/O measurement and diagnostic-buffer move/probe/free path remain unresolved','unresolved'),
 (990,1000,'src/vms_iopost_ccb.c','port_v37_iopost_ccb_complete','DIRPOST sign-extends the IRP negative CCB byte offset, decrements CCB$W_IOC, and on idle transfers the queued deaccess IRP to an IOC$WAKACP callback; live SYS$QIO completion is not connected','partial'),
 (1001,1015,'src/vms_iopost_dirpost.c','port_v37_iopost_dirpost','DIRPOST process I/O accounting and ordered completion; PID/IRP pending registry validates ownership before accounting, then copies IOSB through retained pages and releases pins before SCH$POSTEF; live scheduler and process binding remain absent','partial'),
 (1016,1035,'src/vms_iopost_irpe.c','port_v37_iopost_irpe_plan','Bounded source-ordered IRPE chain preflight and post-POSTEF deallocation callback; live IRPE resolver/lookaside is not bound','partial'),
 (1036,1113,None,None,'System-buffer move/PROBEW, IOSB access-fault path, and remaining paging/segmented I/O completion unported','unresolved')]
rows=[]
for lo,hi,pc,sym,desc,status in iopost_rows:
    if pc:
        line=line_for_symbol(pc,sym)
        if line is None:missing+=1
        rows.append({'original_lines':[lo,hi],'status':status if line else 'missing',
            'pc_file':pc,'pc_symbol':sym,'pc_line':line,'function':desc})
    else:rows.append({'original_lines':[lo,hi],'status':status,'reason':desc})
result['files'].append({'original_file':'SYS/SRC/IOCIOPOST.MAR',
    'original_lines':1113,'sections':rows,
    'unresolved_integration':'Actual x86 software INT 0x90 dispatches the resident privileged NLA end-action subset. Host integration tests connect DIRPOST to a bounded PID/IRP SYSQIO pending registry for IOSB copy and pinned-page release. This is not yet called by a live DQA0 user request, does not bind a running process scheduler or event/AST queues, and is not a VAX IPL implementation.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M125/M193: DQ FUNCTAB masks and native primary ATA IRQ14 transfer completion.
# The first two masks specify legal/buffered I/O and the ten remaining
# rows identify source ACP/EXE action classes. Native DDT actions are bound
# for physical READPBLK/WRITEPBLK only; RB730 and Files-11 ACP remain.
dq_fdt_rows=[
 (1,391,None,None,'Original RB730 declarations and hardware definitions unresolved'),
 (392,437,'src/vms_dqddt_stage_init.c','port_dqddt_stage_init','DPTAB source unit defaults staged only; original UBA/CRB device initialization absent'),
 (438,471,'src/vms_dqddt_activate.c','port_dqddt_activate','Native DDT/FDT activation relocates source action slots to real IA-32 pointers; unsupported actions fail closed, source regdump/MNTVER and RB730 behavior remain unresolved'),
 (472,495,None,None,'Original RB730 hardware FTAB and GENF controller actions unresolved'),
 (496,525,'src/vms_dq_fdt_legal.c','port_dq_fdt_legal','Original legal IO$_ function code bitmap (27 bits), not dispatch'),
 (526,545,'src/vms_dq_fdt_buffered.c','port_dq_fdt_buffered','Original buffered I/O bitmap (18 bits)'),
 (546,551,'src/vms_dq_fdt_read.c','port_dq_fdt_read','Source ACP$READBLK logical, physical, virtual and header read mask'),
 (552,558,'src/vms_dq_fdt_write.c','port_dq_fdt_write','Source ACP$WRITEBLK write and write-check mask'),
 (559,562,'src/vms_dq_fdt_access.c','port_dq_fdt_access','Source ACP$ACCESS file operations mask'),
 (563,565,'src/vms_dq_fdt_deaccess.c','port_dq_fdt_deaccess','Source ACP$DEACCESS mask'),
 (566,570,'src/vms_dq_fdt_modify.c','port_dq_fdt_modify','Source ACP$MODIFY deletion and metadata mask'),
 (571,573,'src/vms_dq_fdt_mount.c','port_dq_fdt_mount','Source ACP$MOUNT request mask, ACP remains unported'),
 (574,581,'src/vms_dq_fdt_zero.c','port_dq_fdt_zero','Source EXE$ZEROPARM disk no-parameter mask'),
 (582,584,'src/vms_dq_fdt_one.c','port_dq_fdt_one','Source EXE$ONEPARM SEEK mask'),
 (585,588,'src/vms_dq_fdt_sense.c','port_dq_fdt_sense','Source EXE$SENSEMODE characteristics mask'),
 (589,592,'src/vms_dq_fdt_set.c','port_dq_fdt_set','Source EXE$SETCHAR disk set-mode mask'),
 (593,633,None,None,'Original RB730 driver setup and controller dispatch entry unresolved'),
 (634,639,'src/vms_dqddt_bootread.c','port_dqddt_startio','Native DQ_STARTIO dispatches physical READPBLK/WRITEPBLK through activated DDT/FDT pointers; after DQA0 publication, IOC$SEARCHDEV-selected DDB DDT START dispatches both functions through bounded private read/write FIFOs. Native ATA separately supports one real IRQ14 asynchronous sector read, but it is not yet bound to the DQA DDT request queue or a process IRP. Public process QIO, ACP, asynchronous DQA scheduling, full RB730 error/controller semantics and process completion remain unimplemented'),
 (640,654,'src/vms_dq_bootread_decode.c','port_dq_bootread_decode','IRP MEDIA physical LBN extraction for bounded ATA LBA28; resident early INIT verifies LBN1 and LBN0 independently'),
 (655,671,'src/vms_dq_bootread_commit.c','port_dq_bootread_commit','UCB retry count, BCNT, FEX and media field update, partial DQ PREPROCESS'),
 (672,677,None,None,'Unimplemented RB730 branch/diagnostic bits and controller hardware'),
 (678,709,'src/vms_dq_bootread_preflight.c','port_dq_bootread_preflight','Source FDISPATCH: PHYSIO bypass of volume-valid, guarded read-only boot transfer'),
 (710,751,None,None,'Original RB730 pack acknowledgement, no-op commands and explicit seek hardware unresolved'),
 (752,767,'src/vms_dq_ioc_transfer.c','port_dq_ioc_transfer','Source physical READPBLK transfer uses real ATA LBA28 with IRQ14; this synchronous retryable private path does not service public DQA requests'),
 (768,771,'src/vms_dq_ioc_async.c','port_dq_ioc_start_async','Priority-selected packet now enters published DQA0 DDT async READPBLK admission after IOC$SEARCHDEV and native DDT/UCB preflight; UCB busy arbitration, public SYS$QIO ownership and user process channel remain unimplemented'),
 (772,781,'src/vms_dq_ioc_async.c','port_dq_ioc_async_complete','IRQ completion commits the retained supervisor IRP and UCB result through resident DQ source code before queue terminal state; caller later runs bounded private IOC post. Public IOSB and AST dispatch remain unconnected'),
 (782,840,None,None,'Original CHECKECC and RB730 ECC correction, continuation, and controller status analysis are unimplemented'),
 (841,851,'src/vms_dq_ioc_retry.c','port_dq_ioc_retry','Source RETRYERR bounded error retry count and return to FDISPATCH; native private ATA callback, RB730 recalibration hardware and ISR unimplemented'),
 (852,916,None,None,'Original RB730 error classification, controller-specific faults and fatal ISR paths remain unresolved'),
 (917,939,'src/vms_dq_ioc_post.c','port_dq_ioc_post','Source FUNCXT one final status/bytes transferred, private IOC post and request retirement; no hardware channel/AST/public QIO'),
 (940,2189,None,None,'RB730 CSR/interrupts, retry, init, controller/port resource and remaining disk source unresolved')]
dq_fdt_sections=[]
for first,last,path,symbol,detail in dq_fdt_rows:
    if path is None:
        dq_fdt_sections.append({'original_lines':[first,last], 'status':'unresolved','reason':detail})
    else:
        line=line_for_symbol(path,symbol)
        if line is None:missing+=1
        dq_fdt_sections.append({'original_lines':[first,last],
             'status':'partial' if line else 'missing',
             'pc_file':path,'pc_symbol':symbol,'pc_line':line,'function':detail})
result['files'].append({'original_file':'DRIVER/SRC/DQDRIVER.MAR',
    'original_lines':2189,'sections':dq_fdt_sections,
    'unresolved_integration':'Native primary ATA IRQ14 is routed through a SYSDEF.MDL-shaped CRB attached to the DQA UCB. Resident SYS.EXE relocates and publishes a native DQA DDT/FDT into DDB/UCB; IOC$SEARCHDEV selects it for physical READPBLK/WRITEPBLK through bounded private IOC FIFOs and actual IRQ14. Other DQ FDT actions, original RB730 controller behavior, public process IOC/$QIO, Files-11 ACP and process AST completion remain unimplemented.'})
# M131: original RELOCDRV.MAR LOCATE/EXECUTE and IOC$INITDRV,
# limited to bounded source DQ DPT_STORE on isolated DDB/UCB copies.
# Real CRB/IDB/port/relocation of DQ action pointers remain unresolved.
relocdrv_rows=[
 (1,102,None,None,'DEC declarations, DDT relocation address vector and original VAX ABI remain unresolved'),
 (103,113,'src/vms_relocdrv_dq_init.c','port_relocdrv_dq_init','IOC$INITDRV DQ DPT program executed on resident isolated DQA0 records, never published'),
 (114,139,None,None,'IOC$REINITDRV driver reload and actual CRB/IDB/UCB dynamic store not yet bound'),
 (140,162,'src/vms_relocdrv_walk.c','port_relocdrv_walk','Original common INIT/REINIT source structure/opcode loop with exact terminating byte, bounded cap'),
 (163,190,'src/vms_relocdrv_step.c','port_relocdrv_step','RELOCDRV EXECUTE source dispatch for B/W/D/L/V operations; absolute kernel mapping guarded'),
 (191,235,'src/vms_relocdrv_fetch.c','port_relocdrv_fetch','Little-endian source immediate and indirect, signed PC-relative DPT and VAX INSV field semantics'),
 (236,245,None,None,'Original VAX LOCATE stack/documentation not used in IA-32 code'),
 (246,278,'src/vms_relocdrv_region.c','port_relocdrv_region','Source CRB/DDB/IDB/UCB type dispatch with explicit IA-32 bounds, no unchecked pointer chase'),
 (279,315,None,None,'Original DDT/FDT driver relocation declarations and VAX high-bit address convention unresolved'),
 (316,352,'src/vms_ddtreloc_apply.c','port_ddtreloc_apply','IOC$RELOC_DDT original DDT offset vector and 12-byte FDT action relocation on bounded isolated bytes only; resident DQ driver remains unbound')]
sections=[]
for lo,hi,pc,sym,desc in relocdrv_rows:
    if pc:
        lineno=line_for_symbol(pc,sym)
        if lineno is None:missing+=1
        sections.append({'original_lines':[lo,hi],'status':'partial' if lineno else 'missing',
          'pc_file':pc,'pc_symbol':sym,'pc_line':lineno,'function':desc})
    else:sections.append({'original_lines':[lo,hi],'status':'unresolved','reason':desc})
result['files'].append({'original_file':'SYS/SRC/RELOCDRV.MAR',
    'original_lines':352,'sections':sections,
    'unresolved_integration':'Nine DQ DPT_STORE operations are interpreted on isolated resident DQA0 DDB/UCB copies using original source byte opcodes. This does NOT initialize/publish a live driver; original active DDT/CRB/IDB, MNTVER, DQ hardware/ACP and live driver relocation remain unported. A private IOC$RELOC_DDT DDT/FDT image rewrite is tested on host bytes only.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M128: LOGNAMSUB.MAR source pin, not a claim of a full logical-name service.
# Only the two FIN_DATABASE bootstrap SYSTEM entries have bound native list
# records. PROCESS/GROUP tables, LOG$LOCKR, SYSLOGNAM and supercession remain
# unresolved; original LOG$HASH and bounded SYSTEM search/translate are partial.
lognam_sections=[]
for first,last,path,sym,why in [
 (1,128,None,None,'Original module prologue, LOG$DELETE, supersession and deallocation unresolved'),
 (129,162,'src/vms_log_system_insert.c','port_log_system_insert','Original LOG$INSLOGN_LCK link sorting for two boot SYSTEM LOG records only; no supersede or runtime insertion'),
 (163,243,None,None,'Original LOG$LOCKR/LOCKW/UNLOCK mutex and GROUP/PROCESS table synchronization unimplemented'),
 (244,276,'src/vms_log_system_lookup.c','port_log_system_lookup','Source LOG$SEARCHLOG bucket search limited to two resident SYSTEM boot names; no PCB access mode'),
 (277,295,None,None,'Remaining SEARCHLOG status and hash calling convention require later full service binding'),
 (296,320,'src/vms_log_hash.c','port_log_hash','Original little-endian XOR, ROTL and MULL hash high bits with caller-selected short table exponent'),
 (321,372,None,None,'Original descriptor probing, access-mode and caller output protocol not fully implemented'),
 (373,411,'src/vms_log_translate.c','port_log_translate','Source leading underscore escape, NOTRAN fallback and bounded two-record SYSTEM resolution; no PROCESS/GROUP tables'),
 (412,424,None,None,'Original LOG$UNLOCK, measurement and native return convention remain unresolved')]:
    if path is None:lognam_sections.append({'original_lines':[first,last],'status':'unresolved','reason':why})
    else:
        ln=line_for_symbol(path,sym)
        if ln is None:missing+=1
        lognam_sections.append({'original_lines':[first,last],'status':'partial' if ln else 'missing',
           'pc_file':path,'pc_symbol':sym,'pc_line':ln,'function':why})
result['files'].append({'original_file':'SYS/SRC/LOGNAMSUB.MAR',
    'original_lines':424,'sections':lognam_sections,
    'unresolved_integration':'Private bounded two-entry resident SYSTEM bootstrap table can resolve SYS$DISK/SYS$SYSDEVICE. Original LOG$LOCKR, SYSLOGNAM, PROCESS/GROUP tables, dynamic create/delete, access control, public channel I/O and Files-11 ACP are absent; published DQA DDT supports physical PBLK only.'})

# M234: source MEMORYALC IRP allocation from and return to the lookaside list.
memalc_sections=[]
for first,last,path,sym,why in [
 (1,214,None,None,'Original nonpaged allocation declarations, shared pool, and general ALLOCATE machinery unresolved'),
 (215,240,'src/vms_qio_irp_pool.c','port_v37_irp_pool_alloc','IRP-sized requests dequeue a validated source INIT lookaside packet, write requested size/type, and restore synchronization IPL; nonpaged variable allocation, pool extension, resource waiting and AST-delivery return semantics remain unported'),
 (241,349,None,None,'Other ALLOC* entries, ALONPAGWAIT caller-mode wait and cleanup semantics unresolved'),
 (350,397,'src/vms_exe_np_pool.c','port_exe_np_allocate','Source IRP/LRP/SRP class selection, DYN tag preservation, any-fitting-slot lookaside dequeue and variable-pool fallback; resident SYS.EXE ACP FCB and WCB allocation now use this path, and QEMU verifies FCB creation plus a read-only MAKACC/DEACCS WCB cycle returning both allocations; general users, pool extension, resource waiting and AST-delivery semantics remain unresolved'),
 (398,461,None,None,'Paged-pool allocation, block splitting, mutexes and shared memory pool unresolved'),
 (462,515,'src/vms_exe_np_pool.c','port_exe_np_pool_alloc','EXE$ALLOCATE source first-fit search, aligned block selection and low-address split in the live SYS.EXE nonpaged pool; failure resource accounting and pool extension remain unresolved'),
 (516,545,'src/vms_qio_irp_pool.c','port_v37_irp_pool_free','Return an allocated source DYN$C_IRP block to the tail of its original lookaside queue with duplicate/corruption guards and synchronization, exercised after resident SYS.EXE DQA0 DDT boot reads; SRP/LRP/general pool return paths unresolved'),
 (546,617,'src/vms_exe_np_pool.c','port_exe_np_deallocate','Source-addressed nonpaged IRP/LRP/SRP lookaside return or variable-block release by DYN tag; QEMU verifies failed Files-11 ACP FCB initialization returns the SRP packet and successful read-only WCB deaccess returns its WCB packet, while paged-pool return remains unresolved'),
 (618,641,'src/vms_exe_np_pool.c','port_exe_np_pool_free','EXE$DEALLOCATE maintains the sorted free list and coalesces adjacent blocks under restored IPL synchronization; fatal bugcheck delivery and source resource wakeup remain unresolved'),
 (642,919,None,None,'Paged/shared pool return, queue synchronization and pool extension unresolved')]:
    if path is None:memalc_sections.append({'original_lines':[first,last],'status':'unresolved','reason':why})
    else:
        ln=line_for_symbol(path,sym)
        if ln is None:missing+=1
        memalc_sections.append({'original_lines':[first,last],'status':'partial' if ln else 'missing',
            'pc_file':path,'pc_symbol':sym,'pc_line':ln,'function':why})
result['files'].append({'original_file':'SYS/SRC/MEMORYALC.MAR',
    'original_lines':919,'sections':memalc_sections,
    'unresolved_integration':'The native SYS.EXE source IRP lookaside queue allocates/releases preallocated IRP slots under a caller-provided IPL synchronization contract and is used by resident DQA0 DDT boot reads. Host-tested allocation also selects exact-size IRP/LRP/SRP queues and falls back to the variable pool; this broader allocator is not yet wired to ACP because the first live wiring attempt regressed DQA0 DDT publication. Pool extension, resource waits, public user SYS$QIO integration, and paged pool remain unresolved.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

(ROOT / 'source-coverage.json').write_text(json.dumps(result, indent=2) + '\n')
print('Source ranges:',sum(len(x.get('sections',[])) for x in result['files']),'; missing mapped functions:',missing)
raise SystemExit(1 if missing else 0)
