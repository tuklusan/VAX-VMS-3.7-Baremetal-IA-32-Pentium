from pathlib import Path

root = Path(__file__).resolve().parents[1]
asm = (root / 'src/arch.s').read_text()
arch = (root / 'src/arch.c').read_text()
ata = (root / 'src/vms_ata_x86.c').read_text()

assert 'vector_no_error 46' in asm
assert '144,46' in asm
assert 'gate_set(46, arch_isr_table[36],0)' in arch
assert 'vector_no_error 145' in asm
assert '144,46,145' in asm
assert 'gate_set(145, arch_isr_table[37],0)' in arch
assert 'f->vector==145u' in arch and 'arch_vms_schedule_frame(f)' in arch
executive = (root / 'src/vms_sys_executive_entry.c').read_text()
assert 'executive_vms_sched_chse' in executive
assert 'vms_sched_chse=executive_vms_sched_chse' in executive
kernel = (root / 'src/kernel.c').read_text()
assert 'proc_result.vms_sched_chse(proc_result.vms_scheduler' in kernel
assert 'source SCH$CHSE reached resident IA-32 vector 0x91' in kernel
assert 'native_no_context_count<' in kernel and 'context_misses_before+1u' in kernel
assert 'outb(0x21,0xfa)' in arch and 'outb(0xA1,0xbf)' in arch
assert 'f->vector == 46' in arch and 'outb(0xa0,0x20)' in arch
assert 'port_ata_x86_irq' in ata and 'ATA_STATUS' in ata
assert 'irq_armed=1' in ata and 'irq_done=1' in ata
assert 'port_ata_x86_write28_async' in ata
assert 'if(async_write_source)' in ata
assert 'INIT: DQA0 DDT asynchronous WRITEPBLK completed by IRQ14; scratch restored.' in (root / 'src/kernel.c').read_text()
print('Primary ATA IRQ14 vector, PIC routing, handler and completion checks PASS')
