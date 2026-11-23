Warning: truncated output (original token count: 47834)
Total output lines: 2519

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
      …27834 tokens truncated…t']==40
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
 (209,230,'src/vms_ini_ucb_boot.c','port_v37_ucb_boot','Boot disk UCB fields, 30 original extension longwords; real disk DDT and CRB unresolved','partial'),
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

# M121: original SYSQIOREQ.MAR EXE$FINISHIOC only. The pending
# IOC$GL_PSBL append is real source-format resident IRP data, but public
# $QIO setup, IOC driver dispatch and IOPOST delivery have not been ported.
qio_rows=[
 (1,111,None,None,'Original declarations and pre-argument-offset service implementation unresolved','unresolved'),
 (112,123,'src/vms_sysqio_args.c','port_v37_sysqio_decode_args','Fixed SYS$QIO argument-list offsets are decoded; event flag, channel, privilege, device and I/O processing remain unimplemented','partial'),
 (124,148,None,None,'Original CCB access-pending detection and event/channel processing unresolved','unresolved'),
 (149,156,'src/vms_sysqio_args.c','port_v37_sysqio_begin','When CCB$L_WIND bit 0 marks access/deaccess pending, require a scheduler wait callback and resume only after the pending bit clears; no live SCH$WAIT/RSN$_ASTWAIT binding yet','partial'),
 (157,238,None,None,'Original spooled-device selection, channel lookup, local/global event handling and remaining service prologue unresolved','unresolved'),
 (239,243,None,None,'EXE$QIO entry declaration and service prologue unresolved','unresolved'),
 (244,247,'src/vms_sysqio_args.c','port_v37_sysqio_begin','Clear EFN before channel lookup and post failure status to the requested event flag','partial'),
 (248,252,'src/vms_v37_ccb.c','port_v37_ccb_resolve','SYSQIO begin resolves the source channel token backward from CTL$GL_CCBBASE, checks AMOD, then calls the live UCB/device resolver; no native SYS$ASSIGN allocator supplies this table yet','partial'),
 (253,283,'src/vms_sysqio_args.c','port_v37_sysqio_preflight','Native CCB access-mode comparison, DQ FDT function legality and offline exceptions with VMS status mapping; access/deaccess pending CCB waits are represented by a required callback but not bound to live SCH$WAIT','partial'),
 (284,285,None,None,'Original IOSB branch prologue unresolved','unresolved'),
 (286,292,'src/vms_sysqio_iosb.c','port_v37_sysqio_clear_iosb','Nullable IOSB, full 8-byte write probe before atomic two-longword clear; caller memory copy wiring remains unresolved','partial'),
 (293,299,None,None,'EXE$QIO resource-charge setup and AST-deletion IPL transition unresolved','unresolved'),
 (300,312,'src/vms_sysqio_args.c','port_v37_sysqio_begin','Direct/buffered per-process I/O counter charge with single-unit quota callback and rollback on denial; IRP reservation/dispatch remain unresolved','partial'),
 (313,335,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_irp','Join service begin with standard IRP/AST reservation; on reservation failure restore direct/buffered process I/O count and post the service error event; channel IOC and driver dispatch remain unresolved','partial'),
 (336,345,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_reserve_irp','Allocate a 136-byte standard IRP from the source INIT lookaside; map exhaustion to SS$_INSFMEM and return the allocated packet if later setup fails','partial'),
 (346,385,None,None,'Source window resolution, buffered user-buffer mapping and complete I/O-quota rollback unresolved','unresolved'),
 (386,429,'src/vms_sysqio_irp_build.c','port_v37_sysqio_build_irp','Build the source-layout 136-byte standard IRP through CDRP$L_LENGTH, including ARB/fork fields and conditional ACB quota flag plus ASTCNT charge, after caller packet allocation; native channel accounting and driver dispatch remain unbound','partial'),
 (430,449,None,None,'Optional I/O measurement start, sequence counter update and function dispatch setup unresolved','unresolved'),
 (450,476,'src/vms_sysqio_normalize.c','port_v37_sysqio_normalize_write','Source FOD/MNT/FOR WRITEVBLK boot-window handling, logical WRITE conversion, NOPRIV status and AST-quota branch predicate; live device-characteristics/PCB wiring remains unresolved','partial'),
 (477,479,'src/vms_sysqio_ast.c','port_v37_sysqio_check_ast_quota','Reject negative ASTCNT only when the request has an AST, posting SS$_EXQUOTA status to the caller; real PCB and abort/post integration unresolved','partial'),
 (480,514,None,None,'I/O transaction measurement sequence and transition into device access policy unresolved','unresolved'),
 (515,535,'src/vms_sysqio_device_access.c','port_v37_sysqio_device_access','Source PCB PRIV bits now populate PHY_IO/LOG_IO state (PHY_IO implies LOG_IO); device privilege and volume checks run after IRP/AST reservation. Access denial rolls back packet and PCB quotas. Live PCB/UCB binding and service dispatch remain incomplete','partial'),
 (536,536,None,None,'EXE$ABORTIO status setup unresolved','unresolved'),
 (537,540,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_reserve_irp','After allocation, reject exhausted AST quota and return the allocated IRP to its source lookaside','partial'),
 (541,544,None,None,'Remaining EXE$ABORTIO channel IOC rollback, IOSB/event completion and process post integration unresolved','unresolved'),
 (545,547,'src/vms_sysqio_irp_reserve.c','port_v37_sysqio_prepare_irp','After the source access check, set IRP$M_PHYSIO in the reserved standard IRP only when the original privilege/device policy grants physical I/O','partial'),
 (548,569,None,None,'DIAGNOSE privilege validation, driver diagnostic-buffer allocation and IRP diagnostic-buffer fields unresolved','unresolved'),
 (570,581,'src/vms_sysqio_device_access.c','port_v37_sysqio_device_access','Source PCB LOG_IO/PHY_IO bits and SPL/FOD/MNT/FOR/SHR gates select volume access callback; real live UCB security binding remains incomplete','partial'),
 (582,735,None,None,'Remaining device FDT action dispatch, physical/diagnostic behavior and request queue/completion unresolved','unresolved'),
 (736,740,'src/vms_qio_abortio.c','port_v37_qio_abortio','Clear IRP IOSB pointer, return AST quota reservation once, append IOC post queue and request IPL4; live PCB lookup and process post delivery remain unbound','partial'),
 (741,742,'src/vms_qio_finishioc.c','port_v37_qio_finishioc','EXE$FINISHIOC clears R1 before common finish path','partial'),
 (743,749,'src/vms_qio_finishio.c','port_v37_qio_finishio','UCB OPCNT, IRP media status, append IOC post FIFO and defer IPL4','partial'),
 (750,908,None,None,'Original queued device/ACP I/O, postprocessing, return and insertion routines unresolved','unresolved')]
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
    'unresolved_integration':'The native AP decoder preserves fixed argument offsets; source-ordered begin clears EFN, checks supplied channel access/FDT/offline rules, probes/clears IOSB, charges direct/buffered I/O counts and posts validation failures. The host-tested reserve helper obtains an actual source INIT IRP packet, enforces AST quota after allocation, builds the 136-byte standard IRP and returns it to the same queue on post-allocation failure. The helpers map BADPARAM, IVCHAN, NOPRIV, ILLIOFUNC, DEVOFFLINE, ACCVIO, EXQUOTA and pool exhaustion to source status values. WRITEVBLK normalization and PHY_IO/LOG_IO gates preserve source device-state decisions. However, no live user service entry, real CCB lookup, source IPL/deletion protection, channel IOC accounting, EXE$ABORTIO service post, buffered window mapping, driver FDT dispatch or process AST completion is connected. Only FINISHIOC, FINISHIO and IPL4 post request run on resident test IRPs.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M123: IOCIOPOST kernel-end-action subset executed in real SYS.EXE.
# Read/write process paths, PCB counters, buffer-release, IOSB delivery,
# ASTs, paged/swap I/O and real IPL interrupt dispatch remain unresolved.
iopost_rows=[
 (1,114,None,None,'DEC headers, VAX registers, priority table and original declarations','unresolved'),
 (115,144,'src/vms_sys_executive_entry.c','executive_iopost_irq_entry','Native supervisor IDT software INT 0x90 invokes real SYS.EXE queue worker; no process AST','partial'),
 (145,155,'src/vms_iopost_pop.c','port_v37_iopost_pop','First-in/first-out REMQUE and sentinel restoration, now called through x86 CPL0 IDT 0x90','partial'),
 (156,159,'src/vms_iopost_kernel_end.c','port_v37_iopost_kernel_end','Whitelisted native privileged end action; other VAX branch and kernel ABI unresolved','partial'),
 (160,165,'src/vms_iopost_service.c','port_v37_iopost_service','Kernel address branch for source IRP PID, IA-32 supervisor aperture replacing VAX high-bit test; PCB lookup unresolved','partial'),
 (166,1113,None,None,'Source original direct/buffered/paging postprocessing, PCB quotas, kernel AST, IOSB, wakeup, virtual segmentation, error paths, VAX interrupt frame and remainder unported','unresolved')]
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
    'unresolved_integration':'Actual x86 software INT 0x90 dispatches the resident privileged NLA end-action subset. This is not a VAX IPL implementation or a real user I/O/AST. All process postprocessing remains unported.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

# M125/M193: DQ FUNCTAB masks and native primary ATA IRQ14 transfer completion.
# The first two masks specify legal/buffered I/O and the ten remaining
# rows identify source ACP/EXE action classes. Native action addresses and
# bounded ATA transfer are partial; RB730 controller and Files-11 ACP remain.
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
 (634,639,'src/vms_dqddt_bootread.c','port_dqddt_startio','Native DQ_STARTIO dispatches physical READPBLK/WRITEPBLK through activated DDT/FDT pointers; after DQA0 publication, IOC$SEARCHDEV-selected DDB DDT START dispatches both functions through bounded private read/write FIFOs. Public process QIO, ACP, asynchronous scheduling, full RB730 error/controller semantics and process completion remain unimplemented'),
 (640,654,'src/vms_dq_bootread_decode.c','port_dq_bootread_decode','IRP MEDIA physical LBN extraction for bounded ATA LBA28; resident early INIT verifies LBN1 and LBN0 independently'),
 (655,671,'src/vms_dq_bootread_commit.c','port_dq_bootread_commit','UCB retry count, BCNT, FEX and media field update, partial DQ PREPROCESS'),
 (672,677,None,None,'Unimplemented RB730 branch/diagnostic bits and controller hardware'),
 (678,709,'src/vms_dq_bootread_preflight.c','port_dq_bootread_preflight','Source FDISPATCH: PHYSIO bypass of volume-valid, guarded read-only boot transfer'),
 (710,751,None,None,'Original RB730 pack acknowledgement, no-op commands and explicit seek hardware unresolved'),
 (752,771,'src/vms_dq_ioc_transfer.c','port_dq_ioc_transfer','Source DQ_READPBLK transfer state uses read-only IA-32 ATA LBA28 PIO with IRQ14 completion and a single outstanding private request; original seek/ERR paths remain unresolved'),
 (772,840,'src/vms_dq_ioc_run.c','port_dq_ioc_run','Resident DQ IOC FIFO now services one private READPBLK through native ATA IRQ14 and the attached CRB; source RB730 ISR/recovery, active DQA DDT dispatch and process-aware/public completion remain unresolved'),
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
    'unresolved_integration':'Native primary ATA IRQ14 is routed through a SYSDEF.MDL-shaped CRB attached to the DQA UCB; resident SYS.EXE validates staged DQ DDT/FDT and completes one private READPBLK through the native DQ IOC FIFO. DQA DDB DDT/action slots, original RB730 behavior, public process IOC/$QIO, Files-11 ACP and process AST completion remain unimplemented.'})
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
    'unresolved_integration':'Private bounded two-entry resident SYSTEM bootstrap table can resolve SYS$DISK/SYS$SYSDEVICE. Original LOG$LOCKR, SYSLOGNAM, PROCESS/GROUP tables, dynamic create/delete, access control, and disk DDT/ACP are absent.'})

# M234: source MEMORYALC IRP allocation from and return to the lookaside list.
memalc_sections=[]
for first,last,path,sym,why in [
 (1,214,None,None,'Original nonpaged allocation declarations, shared pool, and general ALLOCATE machinery unresolved'),
 (215,240,'src/vms_qio_irp_pool.c','port_v37_irp_pool_alloc','IRP-sized requests dequeue a validated source INIT lookaside packet, write requested size/type, and restore synchronization IPL; nonpaged variable allocation, pool extension, resource waiting and AST-delivery return semantics remain unported'),
 (241,349,None,None,'Other ALLOC* entries, ALONPAGWAIT caller-mode wait and cleanup semantics unresolved'),
 (350,397,'src/vms_qio_irp_pool.c','port_v37_irp_pool_alloc','ALONONPAGED source IRP lookaside dequeue path with FIFO ordering and synchronized mutation, exercised by resident SYS.EXE DQA0 DDT boot reads; SRP/LRP selection and variable nonpaged allocator/extension remain unresolved'),
 (398,515,None,None,'Paged-pool allocation, block splitting, mutexes and shared memory pool unresolved'),
 (516,545,'src/vms_qio_irp_pool.c','port_v37_irp_pool_free','Return an allocated source DYN$C_IRP block to the tail of its original lookaside queue with duplicate/corruption guards and synchronization, exercised after resident SYS.EXE DQA0 DDT boot reads; SRP/LRP/general pool return paths unresolved'),
 (546,919,None,None,'Paged deallocation, allocator coalescing, shared-pool return, queue synchronization and pool extension unresolved')]:
    if path is None:memalc_sections.append({'original_lines':[first,last],'status':'unresolved','reason':why})
    else:
        ln=line_for_symbol(path,sym)
        if ln is None:missing+=1
        memalc_sections.append({'original_lines':[first,last],'status':'partial' if ln else 'missing',
            'pc_file':path,'pc_symbol':sym,'pc_line':ln,'function':why})
result['files'].append({'original_file':'SYS/SRC/MEMORYALC.MAR',
    'original_lines':919,'sections':memalc_sections,
    'unresolved_integration':'The native SYS.EXE source IRP lookaside queue allocates/releases preallocated IRP slots under a caller-provided IPL synchronization contract and is used by resident DQA0 DDT boot reads. It has no fallback nonpaged allocator, pool extension, resource wait, public user SYS$QIO integration, or source-wide SRP/LRP/allocation behavior.'})
assert len({f['original_file'] for f in result['files']})==len(result['files'])

(ROOT / 'source-coverage.json').write_text(json.dumps(result, indent=2) + '\n')
print('Source ranges:',sum(len(x.get('sections',[])) for x in result['files']),'; missing mapped functions:',missing)
raise SystemExit(1 if missing else 0)
