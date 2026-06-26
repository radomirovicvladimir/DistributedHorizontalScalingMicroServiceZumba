
kernel:     file format elf64-littleriscv


Disassembly of section .text:

0000000080000000 <_entry>:
    80000000:	00006117          	auipc	sp,0x6
    80000004:	bc813103          	ld	sp,-1080(sp) # 80005bc8 <_GLOBAL_OFFSET_TABLE_+0x8>
    80000008:	00001537          	lui	a0,0x1
    8000000c:	f14025f3          	csrr	a1,mhartid
    80000010:	00158593          	addi	a1,a1,1
    80000014:	02b50533          	mul	a0,a0,a1
    80000018:	00a10133          	add	sp,sp,a0
    8000001c:	078020ef          	jal	ra,80002094 <start>

0000000080000020 <spin>:
    80000020:	0000006f          	j	80000020 <spin>
	...

0000000080001000 <trap_entry>:

.section .text
.globl trap_entry
.align 4
trap_entry:
    addi sp, sp, -128
    80001000:	f8010113          	addi	sp,sp,-128

    sd ra,    0(sp)
    80001004:	00113023          	sd	ra,0(sp)
    sd t0,    8(sp)
    80001008:	00513423          	sd	t0,8(sp)
    sd t1,   16(sp)
    8000100c:	00613823          	sd	t1,16(sp)
    sd t2,   24(sp)
    80001010:	00713c23          	sd	t2,24(sp)
    sd t3,   32(sp)
    80001014:	03c13023          	sd	t3,32(sp)
    sd t4,   40(sp)
    80001018:	03d13423          	sd	t4,40(sp)
    sd t5,   48(sp)
    8000101c:	03e13823          	sd	t5,48(sp)
    sd t6,   56(sp)
    80001020:	03f13c23          	sd	t6,56(sp)
    sd a0,   64(sp)
    80001024:	04a13023          	sd	a0,64(sp)
    sd a1,   72(sp)
    80001028:	04b13423          	sd	a1,72(sp)
    sd a2,   80(sp)
    8000102c:	04c13823          	sd	a2,80(sp)
    sd a3,   88(sp)
    80001030:	04d13c23          	sd	a3,88(sp)
    sd a4,   96(sp)
    80001034:	06e13023          	sd	a4,96(sp)
    sd a5,  104(sp)
    80001038:	06f13423          	sd	a5,104(sp)
    sd a6,  112(sp)
    8000103c:	07013823          	sd	a6,112(sp)
    sd a7,  120(sp)
    80001040:	07113c23          	sd	a7,120(sp)

    mv a0, sp                 # pass &TrapFrame to the C dispatcher
    80001044:	00010513          	mv	a0,sp
    call c_trap_handler
    80001048:	058000ef          	jal	ra,800010a0 <c_trap_handler>

    ld ra,    0(sp)
    8000104c:	00013083          	ld	ra,0(sp)
    ld t0,    8(sp)
    80001050:	00813283          	ld	t0,8(sp)
    ld t1,   16(sp)
    80001054:	01013303          	ld	t1,16(sp)
    ld t2,   24(sp)
    80001058:	01813383          	ld	t2,24(sp)
    ld t3,   32(sp)
    8000105c:	02013e03          	ld	t3,32(sp)
    ld t4,   40(sp)
    80001060:	02813e83          	ld	t4,40(sp)
    ld t5,   48(sp)
    80001064:	03013f03          	ld	t5,48(sp)
    ld t6,   56(sp)
    80001068:	03813f83          	ld	t6,56(sp)
    ld a0,   64(sp)           # return value lives here after the dispatcher writes it
    8000106c:	04013503          	ld	a0,64(sp)
    ld a1,   72(sp)
    80001070:	04813583          	ld	a1,72(sp)
    ld a2,   80(sp)
    80001074:	05013603          	ld	a2,80(sp)
    ld a3,   88(sp)
    80001078:	05813683          	ld	a3,88(sp)
    ld a4,   96(sp)
    8000107c:	06013703          	ld	a4,96(sp)
    ld a5,  104(sp)
    80001080:	06813783          	ld	a5,104(sp)
    ld a6,  112(sp)
    80001084:	07013803          	ld	a6,112(sp)
    ld a7,  120(sp)
    80001088:	07813883          	ld	a7,120(sp)

    addi sp, sp, 128
    8000108c:	08010113          	addi	sp,sp,128
    sret
    80001090:	10200073          	sret
	...

00000000800010a0 <c_trap_handler>:
    uint64 t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
};

// Dispatcher for ecall / exception / interrupt. For Task 1 only ecall is wired.
extern "C" void c_trap_handler(TrapFrame* f) {
    800010a0:	fd010113          	addi	sp,sp,-48
    800010a4:	02113423          	sd	ra,40(sp)
    800010a8:	02813023          	sd	s0,32(sp)
    800010ac:	00913c23          	sd	s1,24(sp)
    800010b0:	01213823          	sd	s2,16(sp)
    800010b4:	01313423          	sd	s3,8(sp)
    800010b8:	03010413          	addi	s0,sp,48
    uint64 cause = READ_CSR(scause);
    800010bc:	142029f3          	csrr	s3,scause
    uint64 pc    = READ_CSR(sepc);
    800010c0:	14102973          	csrr	s2,sepc

    if (cause == SCAUSE_ECALL_U || cause == SCAUSE_ECALL_S) {
    800010c4:	ff898713          	addi	a4,s3,-8
    800010c8:	00100793          	li	a5,1
    800010cc:	06e7ea63          	bltu	a5,a4,80001140 <c_trap_handler+0xa0>
    800010d0:	00050493          	mv	s1,a0
        // Syscall: dispatch by a0, result goes back in a0.
        switch (f->a0) {
    800010d4:	04053783          	ld	a5,64(a0) # 1040 <_entry-0x7fffefc0>
    800010d8:	00100713          	li	a4,1
    800010dc:	00e78c63          	beq	a5,a4,800010f4 <c_trap_handler+0x54>
    800010e0:	00200713          	li	a4,2
    800010e4:	04e78463          	beq	a5,a4,8000112c <c_trap_handler+0x8c>
                f->a0 = (uint64)MemoryAllocator::free((void*)f->a1);
                break;
            }
            default:
                // Unknown syscall: signal error. (-1 in a0, sign-extended.)
                f->a0 = (uint64)-1;
    800010e8:	fff00793          	li	a5,-1
    800010ec:	04f53023          	sd	a5,64(a0)
                break;
    800010f0:	0180006f          	j	80001108 <c_trap_handler+0x68>
                size_t bytes = (size_t)f->a1 * MEM_BLOCK_SIZE;
    800010f4:	04853503          	ld	a0,72(a0)
                f->a0 = (uint64)MemoryAllocator::alloc(bytes);
    800010f8:	00651513          	slli	a0,a0,0x6
    800010fc:	00001097          	auipc	ra,0x1
    80001100:	cac080e7          	jalr	-852(ra) # 80001da8 <_ZN15MemoryAllocator5allocEm>
    80001104:	04a4b023          	sd	a0,64(s1)
        }
        // Step past the ecall instruction so sret resumes after it.
        WRITE_CSR(sepc, pc + 4);
    80001108:	00490913          	addi	s2,s2,4
    8000110c:	14191073          	csrw	sepc,s2
    // Anything else is unexpected during Task 1 — halt loudly.
    kputs("\nunhandled trap: scause="); kputhex(cause);
    kputs(" sepc=");                    kputhex(pc);
    kputc('\n');
    kpanic("trap");
}
    80001110:	02813083          	ld	ra,40(sp)
    80001114:	02013403          	ld	s0,32(sp)
    80001118:	01813483          	ld	s1,24(sp)
    8000111c:	01013903          	ld	s2,16(sp)
    80001120:	00813983          	ld	s3,8(sp)
    80001124:	03010113          	addi	sp,sp,48
    80001128:	00008067          	ret
                f->a0 = (uint64)MemoryAllocator::free((void*)f->a1);
    8000112c:	04853503          	ld	a0,72(a0)
    80001130:	00001097          	auipc	ra,0x1
    80001134:	d08080e7          	jalr	-760(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    80001138:	04a4b023          	sd	a0,64(s1)
                break;
    8000113c:	fcdff06f          	j	80001108 <c_trap_handler+0x68>
    kputs("\nunhandled trap: scause="); kputhex(cause);
    80001140:	00004517          	auipc	a0,0x4
    80001144:	ee050513          	addi	a0,a0,-288 # 80005020 <CONSOLE_STATUS+0x10>
    80001148:	00001097          	auipc	ra,0x1
    8000114c:	a28080e7          	jalr	-1496(ra) # 80001b70 <kputs>
    80001150:	00098513          	mv	a0,s3
    80001154:	00001097          	auipc	ra,0x1
    80001158:	a60080e7          	jalr	-1440(ra) # 80001bb4 <kputhex>
    kputs(" sepc=");                    kputhex(pc);
    8000115c:	00004517          	auipc	a0,0x4
    80001160:	ee450513          	addi	a0,a0,-284 # 80005040 <CONSOLE_STATUS+0x30>
    80001164:	00001097          	auipc	ra,0x1
    80001168:	a0c080e7          	jalr	-1524(ra) # 80001b70 <kputs>
    8000116c:	00090513          	mv	a0,s2
    80001170:	00001097          	auipc	ra,0x1
    80001174:	a44080e7          	jalr	-1468(ra) # 80001bb4 <kputhex>
    kputc('\n');
    80001178:	00a00513          	li	a0,10
    8000117c:	00001097          	auipc	ra,0x1
    80001180:	9b8080e7          	jalr	-1608(ra) # 80001b34 <kputc>
    kpanic("trap");
    80001184:	00004517          	auipc	a0,0x4
    80001188:	ec450513          	addi	a0,a0,-316 # 80005048 <CONSOLE_STATUS+0x38>
    8000118c:	00001097          	auipc	ra,0x1
    80001190:	b68080e7          	jalr	-1176(ra) # 80001cf4 <kpanic>

0000000080001194 <_ZL7in_heapPv>:
    kputs(name);
    kputc('\n');
    if (!ok) tests_failed++;
}

static bool in_heap(void* p) {
    80001194:	ff010113          	addi	sp,sp,-16
    80001198:	00813423          	sd	s0,8(sp)
    8000119c:	01010413          	addi	s0,sp,16
    return p != nullptr &&
           (uint64)p >= (uint64)HEAP_START_ADDR &&
    800011a0:	02050263          	beqz	a0,800011c4 <_ZL7in_heapPv+0x30>
    800011a4:	00005797          	auipc	a5,0x5
    800011a8:	a0c7b783          	ld	a5,-1524(a5) # 80005bb0 <HEAP_START_ADDR>
    return p != nullptr &&
    800011ac:	02f56063          	bltu	a0,a5,800011cc <_ZL7in_heapPv+0x38>
           (uint64)p <  (uint64)HEAP_END_ADDR;
    800011b0:	00005797          	auipc	a5,0x5
    800011b4:	9f87b783          	ld	a5,-1544(a5) # 80005ba8 <HEAP_END_ADDR>
           (uint64)p >= (uint64)HEAP_START_ADDR &&
    800011b8:	02f56263          	bltu	a0,a5,800011dc <_ZL7in_heapPv+0x48>
    800011bc:	00000513          	li	a0,0
    800011c0:	0100006f          	j	800011d0 <_ZL7in_heapPv+0x3c>
    800011c4:	00000513          	li	a0,0
    800011c8:	0080006f          	j	800011d0 <_ZL7in_heapPv+0x3c>
    800011cc:	00000513          	li	a0,0
}
    800011d0:	00813403          	ld	s0,8(sp)
    800011d4:	01010113          	addi	sp,sp,16
    800011d8:	00008067          	ret
           (uint64)p >= (uint64)HEAP_START_ADDR &&
    800011dc:	00100513          	li	a0,1
    800011e0:	ff1ff06f          	j	800011d0 <_ZL7in_heapPv+0x3c>

00000000800011e4 <_ZZL9e2e_testsvEN3FooD1Ev>:
    mem_free(p2);
    check_bool("ecall full free restored heap",
               MemoryAllocator::total_free_bytes() == free0);

    // C++ new/delete (also goes through ecall).
    struct Foo { uint64 x[8]; virtual ~Foo() {} };
    800011e4:	ff010113          	addi	sp,sp,-16
    800011e8:	00813423          	sd	s0,8(sp)
    800011ec:	01010413          	addi	s0,sp,16
    800011f0:	00813403          	ld	s0,8(sp)
    800011f4:	01010113          	addi	sp,sp,16
    800011f8:	00008067          	ret

00000000800011fc <_ZL10check_boolPKcb>:
static void check_bool(const char* name, bool ok) {
    800011fc:	fe010113          	addi	sp,sp,-32
    80001200:	00113c23          	sd	ra,24(sp)
    80001204:	00813823          	sd	s0,16(sp)
    80001208:	00913423          	sd	s1,8(sp)
    8000120c:	01213023          	sd	s2,0(sp)
    80001210:	02010413          	addi	s0,sp,32
    80001214:	00050913          	mv	s2,a0
    80001218:	00058493          	mv	s1,a1
    tests_run++;
    8000121c:	00005717          	auipc	a4,0x5
    80001220:	9d870713          	addi	a4,a4,-1576 # 80005bf4 <_ZL9tests_run>
    80001224:	00072783          	lw	a5,0(a4)
    80001228:	0017879b          	addiw	a5,a5,1
    8000122c:	00f72023          	sw	a5,0(a4)
    kputs(ok ? "  [ OK ] " : "  [FAIL] ");
    80001230:	04058e63          	beqz	a1,8000128c <_ZL10check_boolPKcb+0x90>
    80001234:	00004517          	auipc	a0,0x4
    80001238:	e2c50513          	addi	a0,a0,-468 # 80005060 <CONSOLE_STATUS+0x50>
    8000123c:	00001097          	auipc	ra,0x1
    80001240:	934080e7          	jalr	-1740(ra) # 80001b70 <kputs>
    kputs(name);
    80001244:	00090513          	mv	a0,s2
    80001248:	00001097          	auipc	ra,0x1
    8000124c:	928080e7          	jalr	-1752(ra) # 80001b70 <kputs>
    kputc('\n');
    80001250:	00a00513          	li	a0,10
    80001254:	00001097          	auipc	ra,0x1
    80001258:	8e0080e7          	jalr	-1824(ra) # 80001b34 <kputc>
    if (!ok) tests_failed++;
    8000125c:	00049c63          	bnez	s1,80001274 <_ZL10check_boolPKcb+0x78>
    80001260:	00005717          	auipc	a4,0x5
    80001264:	99070713          	addi	a4,a4,-1648 # 80005bf0 <_ZL12tests_failed>
    80001268:	00072783          	lw	a5,0(a4)
    8000126c:	0017879b          	addiw	a5,a5,1
    80001270:	00f72023          	sw	a5,0(a4)
}
    80001274:	01813083          	ld	ra,24(sp)
    80001278:	01013403          	ld	s0,16(sp)
    8000127c:	00813483          	ld	s1,8(sp)
    80001280:	00013903          	ld	s2,0(sp)
    80001284:	02010113          	addi	sp,sp,32
    80001288:	00008067          	ret
    kputs(ok ? "  [ OK ] " : "  [FAIL] ");
    8000128c:	00004517          	auipc	a0,0x4
    80001290:	dc450513          	addi	a0,a0,-572 # 80005050 <CONSOLE_STATUS+0x40>
    80001294:	fa9ff06f          	j	8000123c <_ZL10check_boolPKcb+0x40>

0000000080001298 <_ZL12direct_testsv>:
static void direct_tests() {
    80001298:	fc010113          	addi	sp,sp,-64
    8000129c:	02113c23          	sd	ra,56(sp)
    800012a0:	02813823          	sd	s0,48(sp)
    800012a4:	02913423          	sd	s1,40(sp)
    800012a8:	03213023          	sd	s2,32(sp)
    800012ac:	01313c23          	sd	s3,24(sp)
    800012b0:	01413823          	sd	s4,16(sp)
    800012b4:	01513423          	sd	s5,8(sp)
    800012b8:	04010413          	addi	s0,sp,64
    kputs("\n-- direct (MemoryAllocator::*) --\n");
    800012bc:	00004517          	auipc	a0,0x4
    800012c0:	db450513          	addi	a0,a0,-588 # 80005070 <CONSOLE_STATUS+0x60>
    800012c4:	00001097          	auipc	ra,0x1
    800012c8:	8ac080e7          	jalr	-1876(ra) # 80001b70 <kputs>
    MemoryAllocator::check();
    800012cc:	00001097          	auipc	ra,0x1
    800012d0:	c60080e7          	jalr	-928(ra) # 80001f2c <_ZN15MemoryAllocator5checkEv>
    size_t free0 = MemoryAllocator::total_free_bytes();
    800012d4:	00001097          	auipc	ra,0x1
    800012d8:	d28080e7          	jalr	-728(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    800012dc:	00050913          	mv	s2,a0
    void* p1 = MemoryAllocator::alloc(100);
    800012e0:	06400513          	li	a0,100
    800012e4:	00001097          	auipc	ra,0x1
    800012e8:	ac4080e7          	jalr	-1340(ra) # 80001da8 <_ZN15MemoryAllocator5allocEm>
    800012ec:	00050493          	mv	s1,a0
    void* p2 = MemoryAllocator::alloc(4096);
    800012f0:	00001537          	lui	a0,0x1
    800012f4:	00001097          	auipc	ra,0x1
    800012f8:	ab4080e7          	jalr	-1356(ra) # 80001da8 <_ZN15MemoryAllocator5allocEm>
    800012fc:	00050993          	mv	s3,a0
    void* p3 = MemoryAllocator::alloc(1);
    80001300:	00100513          	li	a0,1
    80001304:	00001097          	auipc	ra,0x1
    80001308:	aa4080e7          	jalr	-1372(ra) # 80001da8 <_ZN15MemoryAllocator5allocEm>
    8000130c:	00050a13          	mv	s4,a0
    check_bool("alloc(100) in heap",  in_heap(p1));
    80001310:	00048513          	mv	a0,s1
    80001314:	00000097          	auipc	ra,0x0
    80001318:	e80080e7          	jalr	-384(ra) # 80001194 <_ZL7in_heapPv>
    8000131c:	00050593          	mv	a1,a0
    80001320:	00004517          	auipc	a0,0x4
    80001324:	d7850513          	addi	a0,a0,-648 # 80005098 <CONSOLE_STATUS+0x88>
    80001328:	00000097          	auipc	ra,0x0
    8000132c:	ed4080e7          	jalr	-300(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("alloc(4096) in heap", in_heap(p2));
    80001330:	00098513          	mv	a0,s3
    80001334:	00000097          	auipc	ra,0x0
    80001338:	e60080e7          	jalr	-416(ra) # 80001194 <_ZL7in_heapPv>
    8000133c:	00050593          	mv	a1,a0
    80001340:	00004517          	auipc	a0,0x4
    80001344:	d7050513          	addi	a0,a0,-656 # 800050b0 <CONSOLE_STATUS+0xa0>
    80001348:	00000097          	auipc	ra,0x0
    8000134c:	eb4080e7          	jalr	-332(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("alloc(1) in heap",    in_heap(p3));
    80001350:	000a0513          	mv	a0,s4
    80001354:	00000097          	auipc	ra,0x0
    80001358:	e40080e7          	jalr	-448(ra) # 80001194 <_ZL7in_heapPv>
    8000135c:	00050593          	mv	a1,a0
    80001360:	00004517          	auipc	a0,0x4
    80001364:	d6850513          	addi	a0,a0,-664 # 800050c8 <CONSOLE_STATUS+0xb8>
    80001368:	00000097          	auipc	ra,0x0
    8000136c:	e94080e7          	jalr	-364(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("alloc 16-aligned p1", ((uint64)p1 & 0xF) == 0);
    80001370:	00f4f593          	andi	a1,s1,15
    80001374:	0015b593          	seqz	a1,a1
    80001378:	00004517          	auipc	a0,0x4
    8000137c:	d6850513          	addi	a0,a0,-664 # 800050e0 <CONSOLE_STATUS+0xd0>
    80001380:	00000097          	auipc	ra,0x0
    80001384:	e7c080e7          	jalr	-388(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("alloc 16-aligned p2", ((uint64)p2 & 0xF) == 0);
    80001388:	00f9f593          	andi	a1,s3,15
    8000138c:	0015b593          	seqz	a1,a1
    80001390:	00004517          	auipc	a0,0x4
    80001394:	d6850513          	addi	a0,a0,-664 # 800050f8 <CONSOLE_STATUS+0xe8>
    80001398:	00000097          	auipc	ra,0x0
    8000139c:	e64080e7          	jalr	-412(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("alloc 16-aligned p3", ((uint64)p3 & 0xF) == 0);
    800013a0:	00fa7593          	andi	a1,s4,15
    800013a4:	0015b593          	seqz	a1,a1
    800013a8:	00004517          	auipc	a0,0x4
    800013ac:	d6850513          	addi	a0,a0,-664 # 80005110 <CONSOLE_STATUS+0x100>
    800013b0:	00000097          	auipc	ra,0x0
    800013b4:	e4c080e7          	jalr	-436(ra) # 800011fc <_ZL10check_boolPKcb>
    MemoryAllocator::check();
    800013b8:	00001097          	auipc	ra,0x1
    800013bc:	b74080e7          	jalr	-1164(ra) # 80001f2c <_ZN15MemoryAllocator5checkEv>
    MemoryAllocator::free(p1);
    800013c0:	00048513          	mv	a0,s1
    800013c4:	00001097          	auipc	ra,0x1
    800013c8:	a74080e7          	jalr	-1420(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    void* p1b = MemoryAllocator::alloc(100);
    800013cc:	06400513          	li	a0,100
    800013d0:	00001097          	auipc	ra,0x1
    800013d4:	9d8080e7          	jalr	-1576(ra) # 80001da8 <_ZN15MemoryAllocator5allocEm>
    800013d8:	00050a93          	mv	s5,a0
    check_bool("reuse after free", p1b == p1);
    800013dc:	40a485b3          	sub	a1,s1,a0
    800013e0:	0015b593          	seqz	a1,a1
    800013e4:	00004517          	auipc	a0,0x4
    800013e8:	d4450513          	addi	a0,a0,-700 # 80005128 <CONSOLE_STATUS+0x118>
    800013ec:	00000097          	auipc	ra,0x0
    800013f0:	e10080e7          	jalr	-496(ra) # 800011fc <_ZL10check_boolPKcb>
    MemoryAllocator::free(p1b);
    800013f4:	000a8513          	mv	a0,s5
    800013f8:	00001097          	auipc	ra,0x1
    800013fc:	a40080e7          	jalr	-1472(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    MemoryAllocator::free(p3);
    80001400:	000a0513          	mv	a0,s4
    80001404:	00001097          	auipc	ra,0x1
    80001408:	a34080e7          	jalr	-1484(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    MemoryAllocator::free(p2);
    8000140c:	00098513          	mv	a0,s3
    80001410:	00001097          	auipc	ra,0x1
    80001414:	a28080e7          	jalr	-1496(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    MemoryAllocator::check();
    80001418:	00001097          	auipc	ra,0x1
    8000141c:	b14080e7          	jalr	-1260(ra) # 80001f2c <_ZN15MemoryAllocator5checkEv>
    check_bool("free restored full heap", MemoryAllocator::total_free_bytes() == free0);
    80001420:	00001097          	auipc	ra,0x1
    80001424:	bdc080e7          	jalr	-1060(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    80001428:	40a905b3          	sub	a1,s2,a0
    8000142c:	0015b593          	seqz	a1,a1
    80001430:	00004517          	auipc	a0,0x4
    80001434:	da050513          	addi	a0,a0,-608 # 800051d0 <CONSOLE_STATUS+0x1c0>
    80001438:	00000097          	auipc	ra,0x0
    8000143c:	dc4080e7          	jalr	-572(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("free(NULL) == 0", MemoryAllocator::free(nullptr) == 0);
    80001440:	00000513          	li	a0,0
    80001444:	00001097          	auipc	ra,0x1
    80001448:	9f4080e7          	jalr	-1548(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    8000144c:	00153593          	seqz	a1,a0
    80001450:	00004517          	auipc	a0,0x4
    80001454:	cf050513          	addi	a0,a0,-784 # 80005140 <CONSOLE_STATUS+0x130>
    80001458:	00000097          	auipc	ra,0x0
    8000145c:	da4080e7          	jalr	-604(ra) # 800011fc <_ZL10check_boolPKcb>
               MemoryAllocator::free((void*)0xdeadbeefUL) == -1);
    80001460:	37ab7537          	lui	a0,0x37ab7
    80001464:	00251513          	slli	a0,a0,0x2
    80001468:	eef50513          	addi	a0,a0,-273 # 37ab6eef <_entry-0x48549111>
    8000146c:	00001097          	auipc	ra,0x1
    80001470:	9cc080e7          	jalr	-1588(ra) # 80001e38 <_ZN15MemoryAllocator4freeEPv>
    check_bool("free(bogus) == -1",
    80001474:	00150593          	addi	a1,a0,1
    80001478:	0015b593          	seqz	a1,a1
    8000147c:	00004517          	auipc	a0,0x4
    80001480:	cd450513          	addi	a0,a0,-812 # 80005150 <CONSOLE_STATUS+0x140>
    80001484:	00000097          	auipc	ra,0x0
    80001488:	d78080e7          	jalr	-648(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("alloc(0) == NULL", MemoryAllocator::alloc(0) == nullptr);
    8000148c:	00000513          	li	a0,0
    80001490:	00001097          	auipc	ra,0x1
    80001494:	918080e7          	jalr	-1768(ra) # 80001da8 <_ZN15MemoryAllocator5allocEm>
    80001498:	00153593          	seqz	a1,a0
    8000149c:	00004517          	auipc	a0,0x4
    800014a0:	ccc50513          	addi	a0,a0,-820 # 80005168 <CONSOLE_STATUS+0x158>
    800014a4:	00000097          	auipc	ra,0x0
    800014a8:	d58080e7          	jalr	-680(ra) # 800011fc <_ZL10check_boolPKcb>
}
    800014ac:	03813083          	ld	ra,56(sp)
    800014b0:	03013403          	ld	s0,48(sp)
    800014b4:	02813483          	ld	s1,40(sp)
    800014b8:	02013903          	ld	s2,32(sp)
    800014bc:	01813983          	ld	s3,24(sp)
    800014c0:	01013a03          	ld	s4,16(sp)
    800014c4:	00813a83          	ld	s5,8(sp)
    800014c8:	04010113          	addi	sp,sp,64
    800014cc:	00008067          	ret

00000000800014d0 <_ZL12stress_testsv>:
               MemoryAllocator::total_free_bytes() == free0);
}

// ---- stress / fragmentation ----------------------------------------------

static void stress_tests() {
    800014d0:	c9010113          	addi	sp,sp,-880
    800014d4:	36113423          	sd	ra,872(sp)
    800014d8:	36813023          	sd	s0,864(sp)
    800014dc:	34913c23          	sd	s1,856(sp)
    800014e0:	35213823          	sd	s2,848(sp)
    800014e4:	35313423          	sd	s3,840(sp)
    800014e8:	37010413          	addi	s0,sp,880
    kputs("\n-- stress --\n");
    800014ec:	00004517          	auipc	a0,0x4
    800014f0:	c9450513          	addi	a0,a0,-876 # 80005180 <CONSOLE_STATUS+0x170>
    800014f4:	00000097          	auipc	ra,0x0
    800014f8:	67c080e7          	jalr	1660(ra) # 80001b70 <kputs>

    size_t free0 = MemoryAllocator::total_free_bytes();
    800014fc:	00001097          	auipc	ra,0x1
    80001500:	b00080e7          	jalr	-1280(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    80001504:	00050913          	mv	s2,a0

    // Exhaust the heap with 4 KB allocations, then free them all.
    const int N = 64;
    void* buf[N];
    int allocated = 0;
    for (int i = 0; i < N; i++) {
    80001508:	00000493          	li	s1,0
    int allocated = 0;
    8000150c:	00000993          	li	s3,0
    for (int i = 0; i < N; i++) {
    80001510:	03f00793          	li	a5,63
    80001514:	0297c863          	blt	a5,s1,80001544 <_ZL12stress_testsv+0x74>
        buf[i] = mem_alloc(4096);
    80001518:	00001537          	lui	a0,0x1
    8000151c:	00001097          	auipc	ra,0x1
    80001520:	b1c080e7          	jalr	-1252(ra) # 80002038 <mem_alloc>
    80001524:	00349793          	slli	a5,s1,0x3
    80001528:	fd040713          	addi	a4,s0,-48
    8000152c:	00f707b3          	add	a5,a4,a5
    80001530:	e0a7b023          	sd	a0,-512(a5)
        if (!buf[i]) break;
    80001534:	00050863          	beqz	a0,80001544 <_ZL12stress_testsv+0x74>
        allocated++;
    80001538:	0019899b          	addiw	s3,s3,1
    for (int i = 0; i < N; i++) {
    8000153c:	0014849b          	addiw	s1,s1,1
    80001540:	fd1ff06f          	j	80001510 <_ZL12stress_testsv+0x40>
    }
    kputs("  allocated 4KB blocks: "); kputdec(allocated); kputc('\n');
    80001544:	00004517          	auipc	a0,0x4
    80001548:	c4c50513          	addi	a0,a0,-948 # 80005190 <CONSOLE_STATUS+0x180>
    8000154c:	00000097          	auipc	ra,0x0
    80001550:	624080e7          	jalr	1572(ra) # 80001b70 <kputs>
    80001554:	00098513          	mv	a0,s3
    80001558:	00000097          	auipc	ra,0x0
    8000155c:	6f4080e7          	jalr	1780(ra) # 80001c4c <kputdec>
    80001560:	00a00513          	li	a0,10
    80001564:	00000097          	auipc	ra,0x0
    80001568:	5d0080e7          	jalr	1488(ra) # 80001b34 <kputc>
    check_bool("at least 8 x 4KB fit", allocated >= 8);
    8000156c:	00700593          	li	a1,7
    80001570:	0135a5b3          	slt	a1,a1,s3
    80001574:	00004517          	auipc	a0,0x4
    80001578:	c3c50513          	addi	a0,a0,-964 # 800051b0 <CONSOLE_STATUS+0x1a0>
    8000157c:	00000097          	auipc	ra,0x0
    80001580:	c80080e7          	jalr	-896(ra) # 800011fc <_ZL10check_boolPKcb>

    for (int i = 0; i < allocated; i++) mem_free(buf[i]);
    80001584:	00000493          	li	s1,0
    80001588:	0334d263          	bge	s1,s3,800015ac <_ZL12stress_testsv+0xdc>
    8000158c:	00349793          	slli	a5,s1,0x3
    80001590:	fd040713          	addi	a4,s0,-48
    80001594:	00f707b3          	add	a5,a4,a5
    80001598:	e007b503          	ld	a0,-512(a5)
    8000159c:	00001097          	auipc	ra,0x1
    800015a0:	ad0080e7          	jalr	-1328(ra) # 8000206c <mem_free>
    800015a4:	0014849b          	addiw	s1,s1,1
    800015a8:	fe1ff06f          	j	80001588 <_ZL12stress_testsv+0xb8>
    MemoryAllocator::check();
    800015ac:	00001097          	auipc	ra,0x1
    800015b0:	980080e7          	jalr	-1664(ra) # 80001f2c <_ZN15MemoryAllocator5checkEv>
    check_bool("stress: free restored full heap",
               MemoryAllocator::total_free_bytes() == free0);
    800015b4:	00001097          	auipc	ra,0x1
    800015b8:	a48080e7          	jalr	-1464(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    check_bool("stress: free restored full heap",
    800015bc:	40a905b3          	sub	a1,s2,a0
    800015c0:	0015b593          	seqz	a1,a1
    800015c4:	00004517          	auipc	a0,0x4
    800015c8:	c0450513          	addi	a0,a0,-1020 # 800051c8 <CONSOLE_STATUS+0x1b8>
    800015cc:	00000097          	auipc	ra,0x0
    800015d0:	c30080e7          	jalr	-976(ra) # 800011fc <_ZL10check_boolPKcb>

    // Fragmentation: alloc 40 small, free every other, then try one big.
    const int M = 40;
    void* small[M];
    for (int i = 0; i < M; i++) small[i] = mem_alloc(128);
    800015d4:	00000493          	li	s1,0
    800015d8:	0240006f          	j	800015fc <_ZL12stress_testsv+0x12c>
    800015dc:	08000513          	li	a0,128
    800015e0:	00001097          	auipc	ra,0x1
    800015e4:	a58080e7          	jalr	-1448(ra) # 80002038 <mem_alloc>
    800015e8:	00349793          	slli	a5,s1,0x3
    800015ec:	fd040713          	addi	a4,s0,-48
    800015f0:	00f707b3          	add	a5,a4,a5
    800015f4:	cca7b023          	sd	a0,-832(a5)
    800015f8:	0014849b          	addiw	s1,s1,1
    800015fc:	02700793          	li	a5,39
    80001600:	fc97dee3          	bge	a5,s1,800015dc <_ZL12stress_testsv+0x10c>
    for (int i = 0; i < M; i += 2) mem_free(small[i]);
    80001604:	00000493          	li	s1,0
    80001608:	0200006f          	j	80001628 <_ZL12stress_testsv+0x158>
    8000160c:	00349793          	slli	a5,s1,0x3
    80001610:	fd040713          	addi	a4,s0,-48
    80001614:	00f707b3          	add	a5,a4,a5
    80001618:	cc07b503          	ld	a0,-832(a5)
    8000161c:	00001097          	auipc	ra,0x1
    80001620:	a50080e7          	jalr	-1456(ra) # 8000206c <mem_free>
    80001624:	0024849b          	addiw	s1,s1,2
    80001628:	02700793          	li	a5,39
    8000162c:	fe97d0e3          	bge	a5,s1,8000160c <_ZL12stress_testsv+0x13c>
    MemoryAllocator::check();
    80001630:	00001097          	auipc	ra,0x1
    80001634:	8fc080e7          	jalr	-1796(ra) # 80001f2c <_ZN15MemoryAllocator5checkEv>
    // After freeing odd-indexed-only, freelist has many small holes; clean up.
    for (int i = 1; i < M; i += 2) mem_free(small[i]);
    80001638:	00100493          	li	s1,1
    8000163c:	02700793          	li	a5,39
    80001640:	0297c263          	blt	a5,s1,80001664 <_ZL12stress_testsv+0x194>
    80001644:	00349793          	slli	a5,s1,0x3
    80001648:	fd040713          	addi	a4,s0,-48
    8000164c:	00f707b3          	add	a5,a4,a5
    80001650:	cc07b503          	ld	a0,-832(a5)
    80001654:	00001097          	auipc	ra,0x1
    80001658:	a18080e7          	jalr	-1512(ra) # 8000206c <mem_free>
    8000165c:	0024849b          	addiw	s1,s1,2
    80001660:	fddff06f          	j	8000163c <_ZL12stress_testsv+0x16c>
    MemoryAllocator::check();
    80001664:	00001097          	auipc	ra,0x1
    80001668:	8c8080e7          	jalr	-1848(ra) # 80001f2c <_ZN15MemoryAllocator5checkEv>
    check_bool("fragmentation: full free restored heap",
               MemoryAllocator::total_free_bytes() == free0);
    8000166c:	00001097          	auipc	ra,0x1
    80001670:	990080e7          	jalr	-1648(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    check_bool("fragmentation: full free restored heap",
    80001674:	40a905b3          	sub	a1,s2,a0
    80001678:	0015b593          	seqz	a1,a1
    8000167c:	00004517          	auipc	a0,0x4
    80001680:	b6c50513          	addi	a0,a0,-1172 # 800051e8 <CONSOLE_STATUS+0x1d8>
    80001684:	00000097          	auipc	ra,0x0
    80001688:	b78080e7          	jalr	-1160(ra) # 800011fc <_ZL10check_boolPKcb>
}
    8000168c:	36813083          	ld	ra,872(sp)
    80001690:	36013403          	ld	s0,864(sp)
    80001694:	35813483          	ld	s1,856(sp)
    80001698:	35013903          	ld	s2,848(sp)
    8000169c:	34813983          	ld	s3,840(sp)
    800016a0:	37010113          	addi	sp,sp,880
    800016a4:	00008067          	ret

00000000800016a8 <_ZL9e2e_testsv>:
static void e2e_tests() {
    800016a8:	fd010113          	addi	sp,sp,-48
    800016ac:	02113423          	sd	ra,40(sp)
    800016b0:	02813023          	sd	s0,32(sp)
    800016b4:	00913c23          	sd	s1,24(sp)
    800016b8:	01213823          	sd	s2,16(sp)
    800016bc:	01313423          	sd	s3,8(sp)
    800016c0:	01413023          	sd	s4,0(sp)
    800016c4:	03010413          	addi	s0,sp,48
    kputs("\n-- e2e (mem_alloc / mem_free via ecall) --\n");
    800016c8:	00004517          	auipc	a0,0x4
    800016cc:	b4850513          	addi	a0,a0,-1208 # 80005210 <CONSOLE_STATUS+0x200>
    800016d0:	00000097          	auipc	ra,0x0
    800016d4:	4a0080e7          	jalr	1184(ra) # 80001b70 <kputs>
    size_t free0 = MemoryAllocator::total_free_bytes();
    800016d8:	00001097          	auipc	ra,0x1
    800016dc:	924080e7          	jalr	-1756(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    800016e0:	00050913          	mv	s2,a0
    void* p1 = mem_alloc(100);
    800016e4:	06400513          	li	a0,100
    800016e8:	00001097          	auipc	ra,0x1
    800016ec:	950080e7          	jalr	-1712(ra) # 80002038 <mem_alloc>
    800016f0:	00050493          	mv	s1,a0
    void* p2 = mem_alloc(4096);
    800016f4:	00001537          	lui	a0,0x1
    800016f8:	00001097          	auipc	ra,0x1
    800016fc:	940080e7          	jalr	-1728(ra) # 80002038 <mem_alloc>
    80001700:	00050993          	mv	s3,a0
    check_bool("ecall mem_alloc(100)",  in_heap(p1));
    80001704:	00048513          	mv	a0,s1
    80001708:	00000097          	auipc	ra,0x0
    8000170c:	a8c080e7          	jalr	-1396(ra) # 80001194 <_ZL7in_heapPv>
    80001710:	00050593          	mv	a1,a0
    80001714:	00004517          	auipc	a0,0x4
    80001718:	b2c50513          	addi	a0,a0,-1236 # 80005240 <CONSOLE_STATUS+0x230>
    8000171c:	00000097          	auipc	ra,0x0
    80001720:	ae0080e7          	jalr	-1312(ra) # 800011fc <_ZL10check_boolPKcb>
    check_bool("ecall mem_alloc(4096)", in_heap(p2));
    80001724:	00098513          	mv	a0,s3
    80001728:	00000097          	auipc	ra,0x0
    8000172c:	a6c080e7          	jalr	-1428(ra) # 80001194 <_ZL7in_heapPv>
    80001730:	00050593          	mv	a1,a0
    80001734:	00004517          	auipc	a0,0x4
    80001738:	b2450513          	addi	a0,a0,-1244 # 80005258 <CONSOLE_STATUS+0x248>
    8000173c:	00000097          	auipc	ra,0x0
    80001740:	ac0080e7          	jalr	-1344(ra) # 800011fc <_ZL10check_boolPKcb>
    int r1 = mem_free(p1);
    80001744:	00048513          	mv	a0,s1
    80001748:	00001097          	auipc	ra,0x1
    8000174c:	924080e7          	jalr	-1756(ra) # 8000206c <mem_free>
    check_bool("ecall mem_free(p1) == 0", r1 == 0);
    80001750:	00153593          	seqz	a1,a0
    80001754:	00004517          	auipc	a0,0x4
    80001758:	b1c50513          	addi	a0,a0,-1252 # 80005270 <CONSOLE_STATUS+0x260>
    8000175c:	00000097          	auipc	ra,0x0
    80001760:	aa0080e7          	jalr	-1376(ra) # 800011fc <_ZL10check_boolPKcb>
    void* p1b = mem_alloc(100);
    80001764:	06400513          	li	a0,100
    80001768:	00001097          	auipc	ra,0x1
    8000176c:	8d0080e7          	jalr	-1840(ra) # 80002038 <mem_alloc>
    80001770:	00050a13          	mv	s4,a0
    check_bool("ecall reuse", p1b == p1);
    80001774:	40a485b3          	sub	a1,s1,a0
    80001778:	0015b593          	seqz	a1,a1
    8000177c:	00004517          	auipc	a0,0x4
    80001780:	b0c50513          	addi	a0,a0,-1268 # 80005288 <CONSOLE_STATUS+0x278>
    80001784:	00000097          	auipc	ra,0x0
    80001788:	a78080e7          	jalr	-1416(ra) # 800011fc <_ZL10check_boolPKcb>
    mem_free(p1b);
    8000178c:	000a0513          	mv	a0,s4
    80001790:	00001097          	auipc	ra,0x1
    80001794:	8dc080e7          	jalr	-1828(ra) # 8000206c <mem_free>
    mem_free(p2);
    80001798:	00098513          	mv	a0,s3
    8000179c:	00001097          	auipc	ra,0x1
    800017a0:	8d0080e7          	jalr	-1840(ra) # 8000206c <mem_free>
               MemoryAllocator::total_free_bytes() == free0);
    800017a4:	00001097          	auipc	ra,0x1
    800017a8:	858080e7          	jalr	-1960(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    check_bool("ecall full free restored heap",
    800017ac:	40a905b3          	sub	a1,s2,a0
    800017b0:	0015b593          	seqz	a1,a1
    800017b4:	00004517          	auipc	a0,0x4
    800017b8:	ae450513          	addi	a0,a0,-1308 # 80005298 <CONSOLE_STATUS+0x288>
    800017bc:	00000097          	auipc	ra,0x0
    800017c0:	a40080e7          	jalr	-1472(ra) # 800011fc <_ZL10check_boolPKcb>
    Foo* f = new Foo;
    800017c4:	04800513          	li	a0,72
    800017c8:	00000097          	auipc	ra,0x0
    800017cc:	27c080e7          	jalr	636(ra) # 80001a44 <_Znwm>
    800017d0:	00050493          	mv	s1,a0
    struct Foo { uint64 x[8]; virtual ~Foo() {} };
    800017d4:	00004797          	auipc	a5,0x4
    800017d8:	c1c78793          	addi	a5,a5,-996 # 800053f0 <_ZTVZL9e2e_testsvE3Foo+0x10>
    800017dc:	00f53023          	sd	a5,0(a0)
    check_bool("new Foo in heap", in_heap(f));
    800017e0:	00000097          	auipc	ra,0x0
    800017e4:	9b4080e7          	jalr	-1612(ra) # 80001194 <_ZL7in_heapPv>
    800017e8:	00050593          	mv	a1,a0
    800017ec:	00004517          	auipc	a0,0x4
    800017f0:	acc50513          	addi	a0,a0,-1332 # 800052b8 <CONSOLE_STATUS+0x2a8>
    800017f4:	00000097          	auipc	ra,0x0
    800017f8:	a08080e7          	jalr	-1528(ra) # 800011fc <_ZL10check_boolPKcb>
    delete f;
    800017fc:	00048a63          	beqz	s1,80001810 <_ZL9e2e_testsv+0x168>
    80001800:	0004b783          	ld	a5,0(s1)
    80001804:	0087b783          	ld	a5,8(a5)
    80001808:	00048513          	mv	a0,s1
    8000180c:	000780e7          	jalr	a5
               MemoryAllocator::total_free_bytes() == free0);
    80001810:	00000097          	auipc	ra,0x0
    80001814:	7ec080e7          	jalr	2028(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    check_bool("delete restored heap",
    80001818:	40a905b3          	sub	a1,s2,a0
    8000181c:	0015b593          	seqz	a1,a1
    80001820:	00004517          	auipc	a0,0x4
    80001824:	aa850513          	addi	a0,a0,-1368 # 800052c8 <CONSOLE_STATUS+0x2b8>
    80001828:	00000097          	auipc	ra,0x0
    8000182c:	9d4080e7          	jalr	-1580(ra) # 800011fc <_ZL10check_boolPKcb>
}
    80001830:	02813083          	ld	ra,40(sp)
    80001834:	02013403          	ld	s0,32(sp)
    80001838:	01813483          	ld	s1,24(sp)
    8000183c:	01013903          	ld	s2,16(sp)
    80001840:	00813983          	ld	s3,8(sp)
    80001844:	00013a03          	ld	s4,0(sp)
    80001848:	03010113          	addi	sp,sp,48
    8000184c:	00008067          	ret

0000000080001850 <_ZZL9e2e_testsvEN3FooD0Ev>:
    struct Foo { uint64 x[8]; virtual ~Foo() {} };
    80001850:	ff010113          	addi	sp,sp,-16
    80001854:	00113423          	sd	ra,8(sp)
    80001858:	00813023          	sd	s0,0(sp)
    8000185c:	01010413          	addi	s0,sp,16
    80001860:	00000097          	auipc	ra,0x0
    80001864:	234080e7          	jalr	564(ra) # 80001a94 <_ZdlPv>
    80001868:	00813083          	ld	ra,8(sp)
    8000186c:	00013403          	ld	s0,0(sp)
    80001870:	01010113          	addi	sp,sp,16
    80001874:	00008067          	ret

0000000080001878 <main>:

// ---- entry ----------------------------------------------------------------

extern "C" void main() {
    80001878:	fe010113          	addi	sp,sp,-32
    8000187c:	00113c23          	sd	ra,24(sp)
    80001880:	00813823          	sd	s0,16(sp)
    80001884:	00913423          	sd	s1,8(sp)
    80001888:	01213023          	sd	s2,0(sp)
    8000188c:	02010413          	addi	s0,sp,32
    kputs("==== OS1 boot ====\n");
    80001890:	00004517          	auipc	a0,0x4
    80001894:	a5050513          	addi	a0,a0,-1456 # 800052e0 <CONSOLE_STATUS+0x2d0>
    80001898:	00000097          	auipc	ra,0x0
    8000189c:	2d8080e7          	jalr	728(ra) # 80001b70 <kputs>
    kputs("HEAP_START_ADDR = "); kputhex((uint64)HEAP_START_ADDR); kputc('\n');
    800018a0:	00004517          	auipc	a0,0x4
    800018a4:	a5850513          	addi	a0,a0,-1448 # 800052f8 <CONSOLE_STATUS+0x2e8>
    800018a8:	00000097          	auipc	ra,0x0
    800018ac:	2c8080e7          	jalr	712(ra) # 80001b70 <kputs>
    800018b0:	00004497          	auipc	s1,0x4
    800018b4:	30048493          	addi	s1,s1,768 # 80005bb0 <HEAP_START_ADDR>
    800018b8:	0004b503          	ld	a0,0(s1)
    800018bc:	00000097          	auipc	ra,0x0
    800018c0:	2f8080e7          	jalr	760(ra) # 80001bb4 <kputhex>
    800018c4:	00a00513          	li	a0,10
    800018c8:	00000097          	auipc	ra,0x0
    800018cc:	26c080e7          	jalr	620(ra) # 80001b34 <kputc>
    kputs("HEAP_END_ADDR   = "); kputhex((uint64)HEAP_END_ADDR);   kputc('\n');
    800018d0:	00004517          	auipc	a0,0x4
    800018d4:	a4050513          	addi	a0,a0,-1472 # 80005310 <CONSOLE_STATUS+0x300>
    800018d8:	00000097          	auipc	ra,0x0
    800018dc:	298080e7          	jalr	664(ra) # 80001b70 <kputs>
    800018e0:	00004917          	auipc	s2,0x4
    800018e4:	2c890913          	addi	s2,s2,712 # 80005ba8 <HEAP_END_ADDR>
    800018e8:	00093503          	ld	a0,0(s2)
    800018ec:	00000097          	auipc	ra,0x0
    800018f0:	2c8080e7          	jalr	712(ra) # 80001bb4 <kputhex>
    800018f4:	00a00513          	li	a0,10
    800018f8:	00000097          	auipc	ra,0x0
    800018fc:	23c080e7          	jalr	572(ra) # 80001b34 <kputc>
    kputs("HEAP_SIZE       = ");
    80001900:	00004517          	auipc	a0,0x4
    80001904:	a2850513          	addi	a0,a0,-1496 # 80005328 <CONSOLE_STATUS+0x318>
    80001908:	00000097          	auipc	ra,0x0
    8000190c:	268080e7          	jalr	616(ra) # 80001b70 <kputs>
        kputdec((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR);  kputs(" bytes\n");
    80001910:	00093503          	ld	a0,0(s2)
    80001914:	0004b783          	ld	a5,0(s1)
    80001918:	40f50533          	sub	a0,a0,a5
    8000191c:	00000097          	auipc	ra,0x0
    80001920:	330080e7          	jalr	816(ra) # 80001c4c <kputdec>
    80001924:	00004517          	auipc	a0,0x4
    80001928:	a1c50513          	addi	a0,a0,-1508 # 80005340 <CONSOLE_STATUS+0x330>
    8000192c:	00000097          	auipc	ra,0x0
    80001930:	244080e7          	jalr	580(ra) # 80001b70 <kputs>

    MemoryAllocator::init();
    80001934:	00000097          	auipc	ra,0x0
    80001938:	408080e7          	jalr	1032(ra) # 80001d3c <_ZN15MemoryAllocator4initEv>
    kputs("free after init = "); kputdec(MemoryAllocator::total_free_bytes());
    8000193c:	00004517          	auipc	a0,0x4
    80001940:	a0c50513          	addi	a0,a0,-1524 # 80005348 <CONSOLE_STATUS+0x338>
    80001944:	00000097          	auipc	ra,0x0
    80001948:	22c080e7          	jalr	556(ra) # 80001b70 <kputs>
    8000194c:	00000097          	auipc	ra,0x0
    80001950:	6b0080e7          	jalr	1712(ra) # 80001ffc <_ZN15MemoryAllocator16total_free_bytesEv>
    80001954:	00000097          	auipc	ra,0x0
    80001958:	2f8080e7          	jalr	760(ra) # 80001c4c <kputdec>
    kputs(" bytes\n");
    8000195c:	00004517          	auipc	a0,0x4
    80001960:	9e450513          	addi	a0,a0,-1564 # 80005340 <CONSOLE_STATUS+0x330>
    80001964:	00000097          	auipc	ra,0x0
    80001968:	20c080e7          	jalr	524(ra) # 80001b70 <kputs>

    direct_tests();
    8000196c:	00000097          	auipc	ra,0x0
    80001970:	92c080e7          	jalr	-1748(ra) # 80001298 <_ZL12direct_testsv>

    // Install our trap vector and run the same tests through ecall.
    WRITE_CSR(stvec, (uint64)&trap_entry);
    80001974:	fffff497          	auipc	s1,0xfffff
    80001978:	68c48493          	addi	s1,s1,1676 # 80001000 <trap_entry>
    8000197c:	10549073          	csrw	stvec,s1
    kputs("\nstvec installed: "); kputhex((uint64)&trap_entry); kputc('\n');
    80001980:	00004517          	auipc	a0,0x4
    80001984:	9e050513          	addi	a0,a0,-1568 # 80005360 <CONSOLE_STATUS+0x350>
    80001988:	00000097          	auipc	ra,0x0
    8000198c:	1e8080e7          	jalr	488(ra) # 80001b70 <kputs>
    80001990:	00048513          	mv	a0,s1
    80001994:	00000097          	auipc	ra,0x0
    80001998:	220080e7          	jalr	544(ra) # 80001bb4 <kputhex>
    8000199c:	00a00513          	li	a0,10
    800019a0:	00000097          	auipc	ra,0x0
    800019a4:	194080e7          	jalr	404(ra) # 80001b34 <kputc>

    e2e_tests();
    800019a8:	00000097          	auipc	ra,0x0
    800019ac:	d00080e7          	jalr	-768(ra) # 800016a8 <_ZL9e2e_testsv>
    stress_tests();
    800019b0:	00000097          	auipc	ra,0x0
    800019b4:	b20080e7          	jalr	-1248(ra) # 800014d0 <_ZL12stress_testsv>

    kputs("\n==== Task 1 tests: ");
    800019b8:	00004517          	auipc	a0,0x4
    800019bc:	9c050513          	addi	a0,a0,-1600 # 80005378 <CONSOLE_STATUS+0x368>
    800019c0:	00000097          	auipc	ra,0x0
    800019c4:	1b0080e7          	jalr	432(ra) # 80001b70 <kputs>
    kputdec(tests_run - tests_failed); kputc('/'); kputdec(tests_run);
    800019c8:	00004917          	auipc	s2,0x4
    800019cc:	22c90913          	addi	s2,s2,556 # 80005bf4 <_ZL9tests_run>
    800019d0:	00004497          	auipc	s1,0x4
    800019d4:	22048493          	addi	s1,s1,544 # 80005bf0 <_ZL12tests_failed>
    800019d8:	00092503          	lw	a0,0(s2)
    800019dc:	0004a783          	lw	a5,0(s1)
    800019e0:	40f5053b          	subw	a0,a0,a5
    800019e4:	00000097          	auipc	ra,0x0
    800019e8:	268080e7          	jalr	616(ra) # 80001c4c <kputdec>
    800019ec:	02f00513          	li	a0,47
    800019f0:	00000097          	auipc	ra,0x0
    800019f4:	144080e7          	jalr	324(ra) # 80001b34 <kputc>
    800019f8:	00092503          	lw	a0,0(s2)
    800019fc:	00000097          	auipc	ra,0x0
    80001a00:	250080e7          	jalr	592(ra) # 80001c4c <kputdec>
    kputs(" passed ====\n");
    80001a04:	00004517          	auipc	a0,0x4
    80001a08:	98c50513          	addi	a0,a0,-1652 # 80005390 <CONSOLE_STATUS+0x380>
    80001a0c:	00000097          	auipc	ra,0x0
    80001a10:	164080e7          	jalr	356(ra) # 80001b70 <kputs>

    if (tests_failed != 0) kpanic("one or more tests failed");
    80001a14:	0004a783          	lw	a5,0(s1)
    80001a18:	00078a63          	beqz	a5,80001a2c <main+0x1b4>
    80001a1c:	00004517          	auipc	a0,0x4
    80001a20:	98450513          	addi	a0,a0,-1660 # 800053a0 <CONSOLE_STATUS+0x390>
    80001a24:	00000097          	auipc	ra,0x0
    80001a28:	2d0080e7          	jalr	720(ra) # 80001cf4 <kpanic>
    kputs("ALL OK — halting QEMU\n");
    80001a2c:	00004517          	auipc	a0,0x4
    80001a30:	99450513          	addi	a0,a0,-1644 # 800053c0 <CONSOLE_STATUS+0x3b0>
    80001a34:	00000097          	auipc	ra,0x0
    80001a38:	13c080e7          	jalr	316(ra) # 80001b70 <kputs>
    khalt();
    80001a3c:	00000097          	auipc	ra,0x0
    80001a40:	298080e7          	jalr	664(ra) # 80001cd4 <khalt>

0000000080001a44 <_Znwm>:
//
// NOTE: kernel code must NEVER use `new` — that would mean the kernel
// calling its own syscall. Kernel internals use MemoryAllocator::alloc
// directly + placement-new for construction.

void* operator new(size_t n)                       { return mem_alloc(n); }
    80001a44:	ff010113          	addi	sp,sp,-16
    80001a48:	00113423          	sd	ra,8(sp)
    80001a4c:	00813023          	sd	s0,0(sp)
    80001a50:	01010413          	addi	s0,sp,16
    80001a54:	00000097          	auipc	ra,0x0
    80001a58:	5e4080e7          	jalr	1508(ra) # 80002038 <mem_alloc>
    80001a5c:	00813083          	ld	ra,8(sp)
    80001a60:	00013403          	ld	s0,0(sp)
    80001a64:	01010113          	addi	sp,sp,16
    80001a68:	00008067          	ret

0000000080001a6c <_Znam>:
void* operator new[](size_t n)                     { return mem_alloc(n); }
    80001a6c:	ff010113          	addi	sp,sp,-16
    80001a70:	00113423          	sd	ra,8(sp)
    80001a74:	00813023          	sd	s0,0(sp)
    80001a78:	01010413          	addi	s0,sp,16
    80001a7c:	00000097          	auipc	ra,0x0
    80001a80:	5bc080e7          	jalr	1468(ra) # 80002038 <mem_alloc>
    80001a84:	00813083          	ld	ra,8(sp)
    80001a88:	00013403          	ld	s0,0(sp)
    80001a8c:	01010113          	addi	sp,sp,16
    80001a90:	00008067          	ret

0000000080001a94 <_ZdlPv>:

void  operator delete(void* p) noexcept            { mem_free(p); }
    80001a94:	ff010113          	addi	sp,sp,-16
    80001a98:	00113423          	sd	ra,8(sp)
    80001a9c:	00813023          	sd	s0,0(sp)
    80001aa0:	01010413          	addi	s0,sp,16
    80001aa4:	00000097          	auipc	ra,0x0
    80001aa8:	5c8080e7          	jalr	1480(ra) # 8000206c <mem_free>
    80001aac:	00813083          	ld	ra,8(sp)
    80001ab0:	00013403          	ld	s0,0(sp)
    80001ab4:	01010113          	addi	sp,sp,16
    80001ab8:	00008067          	ret

0000000080001abc <_ZdaPv>:
void  operator delete[](void* p) noexcept          { mem_free(p); }
    80001abc:	ff010113          	addi	sp,sp,-16
    80001ac0:	00113423          	sd	ra,8(sp)
    80001ac4:	00813023          	sd	s0,0(sp)
    80001ac8:	01010413          	addi	s0,sp,16
    80001acc:	00000097          	auipc	ra,0x0
    80001ad0:	5a0080e7          	jalr	1440(ra) # 8000206c <mem_free>
    80001ad4:	00813083          	ld	ra,8(sp)
    80001ad8:	00013403          	ld	s0,0(sp)
    80001adc:	01010113          	addi	sp,sp,16
    80001ae0:	00008067          	ret

0000000080001ae4 <_ZdlPvm>:

// C++14 sized-delete forms. Required when classes have non-trivial dtors.
void  operator delete(void* p, size_t) noexcept    { mem_free(p); }
    80001ae4:	ff010113          	addi	sp,sp,-16
    80001ae8:	00113423          	sd	ra,8(sp)
    80001aec:	00813023          	sd	s0,0(sp)
    80001af0:	01010413          	addi	s0,sp,16
    80001af4:	00000097          	auipc	ra,0x0
    80001af8:	578080e7          	jalr	1400(ra) # 8000206c <mem_free>
    80001afc:	00813083          	ld	ra,8(sp)
    80001b00:	00013403          	ld	s0,0(sp)
    80001b04:	01010113          	addi	sp,sp,16
    80001b08:	00008067          	ret

0000000080001b0c <_ZdaPvm>:
void  operator delete[](void* p, size_t) noexcept  { mem_free(p); }
    80001b0c:	ff010113          	addi	sp,sp,-16
    80001b10:	00113423          	sd	ra,8(sp)
    80001b14:	00813023          	sd	s0,0(sp)
    80001b18:	01010413          	addi	s0,sp,16
    80001b1c:	00000097          	auipc	ra,0x0
    80001b20:	550080e7          	jalr	1360(ra) # 8000206c <mem_free>
    80001b24:	00813083          	ld	ra,8(sp)
    80001b28:	00013403          	ld	s0,0(sp)
    80001b2c:	01010113          	addi	sp,sp,16
    80001b30:	00008067          	ret

0000000080001b34 <kputc>:
// We spin on that bit then store one byte into CONSOLE_TX_DATA.
//
// Note: CONSOLE_STATUS / CONSOLE_TX_DATA are extern const uint64 holding the
// MMIO addresses (defined in hw.lib). Cast their VALUE to a volatile pointer.

extern "C" void kputc(char c) {
    80001b34:	ff010113          	addi	sp,sp,-16
    80001b38:	00813423          	sd	s0,8(sp)
    80001b3c:	01010413          	addi	s0,sp,16
    volatile uint8* st = (volatile uint8*)CONSOLE_STATUS;
    80001b40:	00003717          	auipc	a4,0x3
    80001b44:	4d073703          	ld	a4,1232(a4) # 80005010 <CONSOLE_STATUS>
    volatile uint8* tx = (volatile uint8*)CONSOLE_TX_DATA;
    while (!(*st & CONSOLE_TX_STATUS_BIT)) { /* spin */ }
    80001b48:	00074783          	lbu	a5,0(a4)
    80001b4c:	0ff7f793          	andi	a5,a5,255
    80001b50:	0207f793          	andi	a5,a5,32
    80001b54:	fe078ae3          	beqz	a5,80001b48 <kputc+0x14>
    *tx = (uint8)c;
    80001b58:	00003797          	auipc	a5,0x3
    80001b5c:	4b07b783          	ld	a5,1200(a5) # 80005008 <CONSOLE_TX_DATA>
    80001b60:	00a78023          	sb	a0,0(a5)
}
    80001b64:	00813403          	ld	s0,8(sp)
    80001b68:	01010113          	addi	sp,sp,16
    80001b6c:	00008067          	ret

0000000080001b70 <kputs>:

extern "C" void kputs(const char* s) {
    80001b70:	fe010113          	addi	sp,sp,-32
    80001b74:	00113c23          	sd	ra,24(sp)
    80001b78:	00813823          	sd	s0,16(sp)
    80001b7c:	00913423          	sd	s1,8(sp)
    80001b80:	02010413          	addi	s0,sp,32
    80001b84:	00050493          	mv	s1,a0
    while (*s) kputc(*s++);
    80001b88:	0004c503          	lbu	a0,0(s1)
    80001b8c:	00050a63          	beqz	a0,80001ba0 <kputs+0x30>
    80001b90:	00148493          	addi	s1,s1,1
    80001b94:	00000097          	auipc	ra,0x0
    80001b98:	fa0080e7          	jalr	-96(ra) # 80001b34 <kputc>
    80001b9c:	fedff06f          	j	80001b88 <kputs+0x18>
}
    80001ba0:	01813083          	ld	ra,24(sp)
    80001ba4:	01013403          	ld	s0,16(sp)
    80001ba8:	00813483          	ld	s1,8(sp)
    80001bac:	02010113          	addi	sp,sp,32
    80001bb0:	00008067          	ret

0000000080001bb4 <kputhex>:

extern "C" void kputhex(uint64 v) {
    80001bb4:	fe010113          	addi	sp,sp,-32
    80001bb8:	00113c23          	sd	ra,24(sp)
    80001bbc:	00813823          	sd	s0,16(sp)
    80001bc0:	00913423          	sd	s1,8(sp)
    80001bc4:	01213023          	sd	s2,0(sp)
    80001bc8:	02010413          	addi	s0,sp,32
    80001bcc:	00050913          	mv	s2,a0
    kputc('0'); kputc('x');
    80001bd0:	03000513          	li	a0,48
    80001bd4:	00000097          	auipc	ra,0x0
    80001bd8:	f60080e7          	jalr	-160(ra) # 80001b34 <kputc>
    80001bdc:	07800513          	li	a0,120
    80001be0:	00000097          	auipc	ra,0x0
    80001be4:	f54080e7          	jalr	-172(ra) # 80001b34 <kputc>
    bool any = false;
    for (int i = 60; i >= 0; i -= 4) {
    80001be8:	03c00493          	li	s1,60
    bool any = false;
    80001bec:	00000793          	li	a5,0
    80001bf0:	0200006f          	j	80001c10 <kputhex+0x5c>
        uint8 nyb = (uint8)((v >> i) & 0xF);
        if (nyb || any || i == 0) {
            kputc(nyb < 10 ? char('0' + nyb) : char('a' + nyb - 10));
    80001bf4:	00900793          	li	a5,9
    80001bf8:	02a7ea63          	bltu	a5,a0,80001c2c <kputhex+0x78>
    80001bfc:	03050513          	addi	a0,a0,48
    80001c00:	00000097          	auipc	ra,0x0
    80001c04:	f34080e7          	jalr	-204(ra) # 80001b34 <kputc>
            any = true;
    80001c08:	00100793          	li	a5,1
    for (int i = 60; i >= 0; i -= 4) {
    80001c0c:	ffc4849b          	addiw	s1,s1,-4
    80001c10:	0204c263          	bltz	s1,80001c34 <kputhex+0x80>
        uint8 nyb = (uint8)((v >> i) & 0xF);
    80001c14:	00995533          	srl	a0,s2,s1
    80001c18:	00f57513          	andi	a0,a0,15
        if (nyb || any || i == 0) {
    80001c1c:	fc051ce3          	bnez	a0,80001bf4 <kputhex+0x40>
    80001c20:	fc079ae3          	bnez	a5,80001bf4 <kputhex+0x40>
    80001c24:	fe0494e3          	bnez	s1,80001c0c <kputhex+0x58>
    80001c28:	fcdff06f          	j	80001bf4 <kputhex+0x40>
            kputc(nyb < 10 ? char('0' + nyb) : char('a' + nyb - 10));
    80001c2c:	05750513          	addi	a0,a0,87
    80001c30:	fd1ff06f          	j	80001c00 <kputhex+0x4c>
        }
    }
}
    80001c34:	01813083          	ld	ra,24(sp)
    80001c38:	01013403          	ld	s0,16(sp)
    80001c3c:	00813483          	ld	s1,8(sp)
    80001c40:	00013903          	ld	s2,0(sp)
    80001c44:	02010113          	addi	sp,sp,32
    80001c48:	00008067          	ret

0000000080001c4c <kputdec>:

extern "C" void kputdec(uint64 v) {
    80001c4c:	fc010113          	addi	sp,sp,-64
    80001c50:	02113c23          	sd	ra,56(sp)
    80001c54:	02813823          	sd	s0,48(sp)
    80001c58:	02913423          	sd	s1,40(sp)
    80001c5c:	04010413          	addi	s0,sp,64
    if (v == 0) { kputc('0'); return; }
    80001c60:	02050863          	beqz	a0,80001c90 <kputdec+0x44>
    char buf[24]; int n = 0;
    80001c64:	00000793          	li	a5,0
    while (v) { buf[n++] = char('0' + v % 10); v /= 10; }
    80001c68:	04050863          	beqz	a0,80001cb8 <kputdec+0x6c>
    80001c6c:	00a00693          	li	a3,10
    80001c70:	02d57733          	remu	a4,a0,a3
    80001c74:	03070713          	addi	a4,a4,48
    80001c78:	fe040613          	addi	a2,s0,-32
    80001c7c:	00f60633          	add	a2,a2,a5
    80001c80:	fee60423          	sb	a4,-24(a2)
    80001c84:	02d55533          	divu	a0,a0,a3
    80001c88:	0017879b          	addiw	a5,a5,1
    80001c8c:	fddff06f          	j	80001c68 <kputdec+0x1c>
    if (v == 0) { kputc('0'); return; }
    80001c90:	03000513          	li	a0,48
    80001c94:	00000097          	auipc	ra,0x0
    80001c98:	ea0080e7          	jalr	-352(ra) # 80001b34 <kputc>
    80001c9c:	0240006f          	j	80001cc0 <kputdec+0x74>
    while (n--) kputc(buf[n]);
    80001ca0:	fe040793          	addi	a5,s0,-32
    80001ca4:	009787b3          	add	a5,a5,s1
    80001ca8:	fe87c503          	lbu	a0,-24(a5)
    80001cac:	00000097          	auipc	ra,0x0
    80001cb0:	e88080e7          	jalr	-376(ra) # 80001b34 <kputc>
    80001cb4:	00048793          	mv	a5,s1
    80001cb8:	fff7849b          	addiw	s1,a5,-1
    80001cbc:	fe0792e3          	bnez	a5,80001ca0 <kputdec+0x54>
}
    80001cc0:	03813083          	ld	ra,56(sp)
    80001cc4:	03013403          	ld	s0,48(sp)
    80001cc8:	02813483          	ld	s1,40(sp)
    80001ccc:	04010113          	addi	sp,sp,64
    80001cd0:	00008067          	ret

0000000080001cd4 <khalt>:

extern "C" __attribute__((noreturn)) void khalt() {
    80001cd4:	ff010113          	addi	sp,sp,-16
    80001cd8:	00813423          	sd	s0,8(sp)
    80001cdc:	01010413          	addi	s0,sp,16
    *(volatile uint32*)0x100000 = 0x5555;
    80001ce0:	00100737          	lui	a4,0x100
    80001ce4:	000057b7          	lui	a5,0x5
    80001ce8:	5557879b          	addiw	a5,a5,1365
    80001cec:	00f72023          	sw	a5,0(a4) # 100000 <_entry-0x7ff00000>
    for (;;) { /* unreachable, but compiler doesn't know */ }
    80001cf0:	0000006f          	j	80001cf0 <khalt+0x1c>

0000000080001cf4 <kpanic>:
}

extern "C" __attribute__((noreturn)) void kpanic(const char* msg) {
    80001cf4:	fe010113          	addi	sp,sp,-32
    80001cf8:	00113c23          	sd	ra,24(sp)
    80001cfc:	00813823          	sd	s0,16(sp)
    80001d00:	00913423          	sd	s1,8(sp)
    80001d04:	02010413          	addi	s0,sp,32
    80001d08:	00050493          	mv	s1,a0
    kputs("\nPANIC: ");
    80001d0c:	00003517          	auipc	a0,0x3
    80001d10:	6f450513          	addi	a0,a0,1780 # 80005400 <_ZTVZL9e2e_testsvE3Foo+0x20>
    80001d14:	00000097          	auipc	ra,0x0
    80001d18:	e5c080e7          	jalr	-420(ra) # 80001b70 <kputs>
    kputs(msg);
    80001d1c:	00048513          	mv	a0,s1
    80001d20:	00000097          	auipc	ra,0x0
    80001d24:	e50080e7          	jalr	-432(ra) # 80001b70 <kputs>
    kputc('\n');
    80001d28:	00a00513          	li	a0,10
    80001d2c:	00000097          	auipc	ra,0x0
    80001d30:	e08080e7          	jalr	-504(ra) # 80001b34 <kputc>
    khalt();
    80001d34:	00000097          	auipc	ra,0x0
    80001d38:	fa0080e7          	jalr	-96(ra) # 80001cd4 <khalt>

0000000080001d3c <_ZN15MemoryAllocator4initEv>:
static inline uint64 align_down(uint64 v, uint64 a) { return v & ~(a - 1); }

// ---- init -----------------------------------------------------------------

void MemoryAllocator::init() {
    uint64 start = align_up  ((uint64)HEAP_START_ADDR, MEM_BLOCK_SIZE);
    80001d3c:	00004717          	auipc	a4,0x4
    80001d40:	e7473703          	ld	a4,-396(a4) # 80005bb0 <HEAP_START_ADDR>
static inline uint64 align_up(uint64 v, uint64 a)   { return (v + a - 1) & ~(a - 1); }
    80001d44:	03f70713          	addi	a4,a4,63
    80001d48:	fc077713          	andi	a4,a4,-64
    uint64 end   = align_down((uint64)HEAP_END_ADDR,   MEM_BLOCK_SIZE);
    80001d4c:	00004797          	auipc	a5,0x4
    80001d50:	e5c7b783          	ld	a5,-420(a5) # 80005ba8 <HEAP_END_ADDR>
static inline uint64 align_down(uint64 v, uint64 a) { return v & ~(a - 1); }
    80001d54:	fc07f793          	andi	a5,a5,-64

    if (end <= start || (end - start) < 2 * MEM_BLOCK_SIZE) {
    80001d58:	02f77863          	bgeu	a4,a5,80001d88 <_ZN15MemoryAllocator4initEv+0x4c>
    80001d5c:	40e787b3          	sub	a5,a5,a4
    80001d60:	07f00693          	li	a3,127
    80001d64:	02f6f263          	bgeu	a3,a5,80001d88 <_ZN15MemoryAllocator4initEv+0x4c>
        kpanic("MemoryAllocator::init: heap region too small");
    }

    freelist = (Header*)start;
    80001d68:	00004697          	auipc	a3,0x4
    80001d6c:	e9068693          	addi	a3,a3,-368 # 80005bf8 <_ZN15MemoryAllocator8freelistE>
    80001d70:	00e6b023          	sd	a4,0(a3)
    freelist->next   = nullptr;
    80001d74:	00073023          	sd	zero,0(a4)
    freelist->blocks = (end - start) / MEM_BLOCK_SIZE;
    80001d78:	0006b703          	ld	a4,0(a3)
    80001d7c:	0067d793          	srli	a5,a5,0x6
    80001d80:	00f73423          	sd	a5,8(a4)
    80001d84:	00008067          	ret
void MemoryAllocator::init() {
    80001d88:	ff010113          	addi	sp,sp,-16
    80001d8c:	00113423          	sd	ra,8(sp)
    80001d90:	00813023          	sd	s0,0(sp)
    80001d94:	01010413          	addi	s0,sp,16
        kpanic("MemoryAllocator::init: heap region too small");
    80001d98:	00003517          	auipc	a0,0x3
    80001d9c:	67850513          	addi	a0,a0,1656 # 80005410 <_ZTVZL9e2e_testsvE3Foo+0x30>
    80001da0:	00000097          	auipc	ra,0x0
    80001da4:	f54080e7          	jalr	-172(ra) # 80001cf4 <kpanic>

0000000080001da8 <_ZN15MemoryAllocator5allocEm>:
}

// ---- alloc ----------------------------------------------------------------

void* MemoryAllocator::alloc(size_t bytes) {
    80001da8:	ff010113          	addi	sp,sp,-16
    80001dac:	00813423          	sd	s0,8(sp)
    80001db0:	01010413          	addi	s0,sp,16
    if (bytes == 0) return nullptr;
    80001db4:	06050a63          	beqz	a0,80001e28 <_ZN15MemoryAllocator5allocEm+0x80>

    // We need enough space for the user's payload AND our header, rounded up
    // to whole blocks.
    size_t need_bytes = bytes + sizeof(Header);
    80001db8:	01050793          	addi	a5,a0,16
    // Overflow guard (degenerate but cheap).
    if (need_bytes < bytes) return nullptr;
    80001dbc:	06a7ea63          	bltu	a5,a0,80001e30 <_ZN15MemoryAllocator5allocEm+0x88>
    size_t need = (need_bytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    80001dc0:	04f50513          	addi	a0,a0,79
    80001dc4:	00655793          	srli	a5,a0,0x6

    Header** pp = &freelist;
    for (Header* p = freelist; p != nullptr; pp = &p->next, p = p->next) {
    80001dc8:	00004717          	auipc	a4,0x4
    80001dcc:	e3070713          	addi	a4,a4,-464 # 80005bf8 <_ZN15MemoryAllocator8freelistE>
    80001dd0:	00073503          	ld	a0,0(a4)
    Header** pp = &freelist;
    80001dd4:	00070693          	mv	a3,a4
    for (Header* p = freelist; p != nullptr; pp = &p->next, p = p->next) {
    80001dd8:	02050463          	beqz	a0,80001e00 <_ZN15MemoryAllocator5allocEm+0x58>
        if (p->blocks < need) continue;
    80001ddc:	00853703          	ld	a4,8(a0)
    80001de0:	02f76663          	bltu	a4,a5,80001e0c <_ZN15MemoryAllocator5allocEm+0x64>

        if (p->blocks == need) {
    80001de4:	02f70a63          	beq	a4,a5,80001e18 <_ZN15MemoryAllocator5allocEm+0x70>
            return (void*)(p + 1);
        }

        // Split: shrink the front node in place, carve the tail off as the
        // allocation. Keeping the front linked means we don't have to fix *pp.
        p->blocks -= need;
    80001de8:	40f70733          	sub	a4,a4,a5
    80001dec:	00e53423          	sd	a4,8(a0)
        Header* tail = (Header*)((uchar*)p + p->blocks * MEM_BLOCK_SIZE);
    80001df0:	00671713          	slli	a4,a4,0x6
    80001df4:	00e50533          	add	a0,a0,a4
        tail->blocks = need;
    80001df8:	00f53423          	sd	a5,8(a0)
        return (void*)(tail + 1);
    80001dfc:	01050513          	addi	a0,a0,16
    }

    return nullptr;   // heap exhausted (or too fragmented for this request)
}
    80001e00:	00813403          	ld	s0,8(sp)
    80001e04:	01010113          	addi	sp,sp,16
    80001e08:	00008067          	ret
    for (Header* p = freelist; p != nullptr; pp = &p->next, p = p->next) {
    80001e0c:	00050693          	mv	a3,a0
    80001e10:	00053503          	ld	a0,0(a0)
    80001e14:	fc5ff06f          	j	80001dd8 <_ZN15MemoryAllocator5allocEm+0x30>
            *pp = p->next;
    80001e18:	00053783          	ld	a5,0(a0)
    80001e1c:	00f6b023          	sd	a5,0(a3)
            return (void*)(p + 1);
    80001e20:	01050513          	addi	a0,a0,16
    80001e24:	fddff06f          	j	80001e00 <_ZN15MemoryAllocator5allocEm+0x58>
    if (bytes == 0) return nullptr;
    80001e28:	00000513          	li	a0,0
    80001e2c:	fd5ff06f          	j	80001e00 <_ZN15MemoryAllocator5allocEm+0x58>
    if (need_bytes < bytes) return nullptr;
    80001e30:	00000513          	li	a0,0
    80001e34:	fcdff06f          	j	80001e00 <_ZN15MemoryAllocator5allocEm+0x58>

0000000080001e38 <_ZN15MemoryAllocator4freeEPv>:

// ---- free -----------------------------------------------------------------

int MemoryAllocator::free(void* ptr) {
    80001e38:	ff010113          	addi	sp,sp,-16
    80001e3c:	00813423          	sd	s0,8(sp)
    80001e40:	01010413          	addi	s0,sp,16
    if (ptr == nullptr) return 0;   // free(NULL) is a no-op
    80001e44:	0c050463          	beqz	a0,80001f0c <_ZN15MemoryAllocator4freeEPv+0xd4>

    Header* h = (Header*)ptr - 1;
    80001e48:	ff050713          	addi	a4,a0,-16

    // Reject obviously-not-ours pointers.
    if ((uint64)h <  (uint64)HEAP_START_ADDR) return -1;
    80001e4c:	00004797          	auipc	a5,0x4
    80001e50:	d647b783          	ld	a5,-668(a5) # 80005bb0 <HEAP_START_ADDR>
    80001e54:	0cf76063          	bltu	a4,a5,80001f14 <_ZN15MemoryAllocator4freeEPv+0xdc>
    if ((uint64)h >= (uint64)HEAP_END_ADDR)   return -1;
    80001e58:	00004797          	auipc	a5,0x4
    80001e5c:	d507b783          	ld	a5,-688(a5) # 80005ba8 <HEAP_END_ADDR>
    80001e60:	0af77e63          	bgeu	a4,a5,80001f1c <_ZN15MemoryAllocator4freeEPv+0xe4>
    if (h->blocks == 0)                       return -1;   // corrupt header
    80001e64:	ff853583          	ld	a1,-8(a0)
    80001e68:	0a058e63          	beqz	a1,80001f24 <_ZN15MemoryAllocator4freeEPv+0xec>

    // Find the freelist slot: largest `prev` with prev < h, and `cur` = its successor.
    Header *prev = nullptr, *cur = freelist;
    80001e6c:	00004797          	auipc	a5,0x4
    80001e70:	d8c7b783          	ld	a5,-628(a5) # 80005bf8 <_ZN15MemoryAllocator8freelistE>
    80001e74:	00000693          	li	a3,0
    80001e78:	00c0006f          	j	80001e84 <_ZN15MemoryAllocator4freeEPv+0x4c>
    while (cur != nullptr && cur < h) { prev = cur; cur = cur->next; }
    80001e7c:	00078693          	mv	a3,a5
    80001e80:	0007b783          	ld	a5,0(a5)
    80001e84:	00078463          	beqz	a5,80001e8c <_ZN15MemoryAllocator4freeEPv+0x54>
    80001e88:	fee7eae3          	bltu	a5,a4,80001e7c <_ZN15MemoryAllocator4freeEPv+0x44>

    // 1) Coalesce with successor if h and cur are adjacent in memory.
    uchar* h_end = (uchar*)h + h->blocks * MEM_BLOCK_SIZE;
    80001e8c:	00659613          	slli	a2,a1,0x6
    80001e90:	00c70633          	add	a2,a4,a2
    if (cur != nullptr && h_end == (uchar*)cur) {
    80001e94:	00078463          	beqz	a5,80001e9c <_ZN15MemoryAllocator4freeEPv+0x64>
    80001e98:	02c78863          	beq	a5,a2,80001ec8 <_ZN15MemoryAllocator4freeEPv+0x90>
        h->blocks += cur->blocks;
        h->next    = cur->next;
    } else {
        h->next = cur;
    80001e9c:	fef53823          	sd	a5,-16(a0)
    }

    // 2) Coalesce with predecessor, or just splice in.
    if (prev != nullptr) {
    80001ea0:	04068e63          	beqz	a3,80001efc <_ZN15MemoryAllocator4freeEPv+0xc4>
        uchar* prev_end = (uchar*)prev + prev->blocks * MEM_BLOCK_SIZE;
    80001ea4:	0086b603          	ld	a2,8(a3)
    80001ea8:	00661793          	slli	a5,a2,0x6
    80001eac:	00f687b3          	add	a5,a3,a5
        if (prev_end == (uchar*)h) {
    80001eb0:	02f70863          	beq	a4,a5,80001ee0 <_ZN15MemoryAllocator4freeEPv+0xa8>
            prev->blocks += h->blocks;
            prev->next    = h->next;
        } else {
            prev->next = h;
    80001eb4:	00e6b023          	sd	a4,0(a3)
        }
    } else {
        freelist = h;
    }
    return 0;
    80001eb8:	00000513          	li	a0,0
}
    80001ebc:	00813403          	ld	s0,8(sp)
    80001ec0:	01010113          	addi	sp,sp,16
    80001ec4:	00008067          	ret
        h->blocks += cur->blocks;
    80001ec8:	0087b603          	ld	a2,8(a5)
    80001ecc:	00c585b3          	add	a1,a1,a2
    80001ed0:	feb53c23          	sd	a1,-8(a0)
        h->next    = cur->next;
    80001ed4:	0007b783          	ld	a5,0(a5)
    80001ed8:	fef53823          	sd	a5,-16(a0)
    80001edc:	fc5ff06f          	j	80001ea0 <_ZN15MemoryAllocator4freeEPv+0x68>
            prev->blocks += h->blocks;
    80001ee0:	ff853783          	ld	a5,-8(a0)
    80001ee4:	00f60633          	add	a2,a2,a5
    80001ee8:	00c6b423          	sd	a2,8(a3)
            prev->next    = h->next;
    80001eec:	ff053783          	ld	a5,-16(a0)
    80001ef0:	00f6b023          	sd	a5,0(a3)
    return 0;
    80001ef4:	00000513          	li	a0,0
    80001ef8:	fc5ff06f          	j	80001ebc <_ZN15MemoryAllocator4freeEPv+0x84>
        freelist = h;
    80001efc:	00004797          	auipc	a5,0x4
    80001f00:	cee7be23          	sd	a4,-772(a5) # 80005bf8 <_ZN15MemoryAllocator8freelistE>
    return 0;
    80001f04:	00000513          	li	a0,0
    80001f08:	fb5ff06f          	j	80001ebc <_ZN15MemoryAllocator4freeEPv+0x84>
    if (ptr == nullptr) return 0;   // free(NULL) is a no-op
    80001f0c:	00000513          	li	a0,0
    80001f10:	fadff06f          	j	80001ebc <_ZN15MemoryAllocator4freeEPv+0x84>
    if ((uint64)h <  (uint64)HEAP_START_ADDR) return -1;
    80001f14:	fff00513          	li	a0,-1
    80001f18:	fa5ff06f          	j	80001ebc <_ZN15MemoryAllocator4freeEPv+0x84>
    if ((uint64)h >= (uint64)HEAP_END_ADDR)   return -1;
    80001f1c:	fff00513          	li	a0,-1
    80001f20:	f9dff06f          	j	80001ebc <_ZN15MemoryAllocator4freeEPv+0x84>
    if (h->blocks == 0)                       return -1;   // corrupt header
    80001f24:	fff00513          	li	a0,-1
    80001f28:	f95ff06f          	j	80001ebc <_ZN15MemoryAllocator4freeEPv+0x84>

0000000080001f2c <_ZN15MemoryAllocator5checkEv>:

// ---- check ----------------------------------------------------------------

void MemoryAllocator::check() {
    Header* prev = nullptr;
    for (Header* p = freelist; p != nullptr; prev = p, p = p->next) {
    80001f2c:	00004797          	auipc	a5,0x4
    80001f30:	ccc7b783          	ld	a5,-820(a5) # 80005bf8 <_ZN15MemoryAllocator8freelistE>
    Header* prev = nullptr;
    80001f34:	00000713          	li	a4,0
    for (Header* p = freelist; p != nullptr; prev = p, p = p->next) {
    80001f38:	0c078063          	beqz	a5,80001ff8 <_ZN15MemoryAllocator5checkEv+0xcc>
void MemoryAllocator::check() {
    80001f3c:	ff010113          	addi	sp,sp,-16
    80001f40:	00113423          	sd	ra,8(sp)
    80001f44:	00813023          	sd	s0,0(sp)
    80001f48:	01010413          	addi	s0,sp,16
    80001f4c:	0100006f          	j	80001f5c <_ZN15MemoryAllocator5checkEv+0x30>
    for (Header* p = freelist; p != nullptr; prev = p, p = p->next) {
    80001f50:	00078713          	mv	a4,a5
    80001f54:	0007b783          	ld	a5,0(a5)
    80001f58:	08078863          	beqz	a5,80001fe8 <_ZN15MemoryAllocator5checkEv+0xbc>
        if ((uint64)p < (uint64)HEAP_START_ADDR ||
    80001f5c:	00004697          	auipc	a3,0x4
    80001f60:	c546b683          	ld	a3,-940(a3) # 80005bb0 <HEAP_START_ADDR>
    80001f64:	04d7e263          	bltu	a5,a3,80001fa8 <_ZN15MemoryAllocator5checkEv+0x7c>
            (uint64)p >= (uint64)HEAP_END_ADDR)
    80001f68:	00004697          	auipc	a3,0x4
    80001f6c:	c406b683          	ld	a3,-960(a3) # 80005ba8 <HEAP_END_ADDR>
        if ((uint64)p < (uint64)HEAP_START_ADDR ||
    80001f70:	02d7fc63          	bgeu	a5,a3,80001fa8 <_ZN15MemoryAllocator5checkEv+0x7c>
            kpanic("freelist: node out of bounds");
        if (p->blocks == 0)
    80001f74:	0087b683          	ld	a3,8(a5)
    80001f78:	04068063          	beqz	a3,80001fb8 <_ZN15MemoryAllocator5checkEv+0x8c>
            kpanic("freelist: zero-size node");
        if (prev != nullptr) {
    80001f7c:	fc070ae3          	beqz	a4,80001f50 <_ZN15MemoryAllocator5checkEv+0x24>
            if (prev >= p)
    80001f80:	04f77463          	bgeu	a4,a5,80001fc8 <_ZN15MemoryAllocator5checkEv+0x9c>
                kpanic("freelist: not sorted");
            uchar* prev_end = (uchar*)prev + prev->blocks * MEM_BLOCK_SIZE;
    80001f84:	00873683          	ld	a3,8(a4)
    80001f88:	00669693          	slli	a3,a3,0x6
    80001f8c:	00d70733          	add	a4,a4,a3
            if (prev_end > (uchar*)p)
    80001f90:	04e7e463          	bltu	a5,a4,80001fd8 <_ZN15MemoryAllocator5checkEv+0xac>
                kpanic("freelist: overlapping nodes");
            if (prev_end == (uchar*)p)
    80001f94:	fae79ee3          	bne	a5,a4,80001f50 <_ZN15MemoryAllocator5checkEv+0x24>
                kpanic("freelist: adjacent nodes not coalesced");
    80001f98:	00003517          	auipc	a0,0x3
    80001f9c:	52050513          	addi	a0,a0,1312 # 800054b8 <_ZTVZL9e2e_testsvE3Foo+0xd8>
    80001fa0:	00000097          	auipc	ra,0x0
    80001fa4:	d54080e7          	jalr	-684(ra) # 80001cf4 <kpanic>
            kpanic("freelist: node out of bounds");
    80001fa8:	00003517          	auipc	a0,0x3
    80001fac:	49850513          	addi	a0,a0,1176 # 80005440 <_ZTVZL9e2e_testsvE3Foo+0x60>
    80001fb0:	00000097          	auipc	ra,0x0
    80001fb4:	d44080e7          	jalr	-700(ra) # 80001cf4 <kpanic>
            kpanic("freelist: zero-size node");
    80001fb8:	00003517          	auipc	a0,0x3
    80001fbc:	4a850513          	addi	a0,a0,1192 # 80005460 <_ZTVZL9e2e_testsvE3Foo+0x80>
    80001fc0:	00000097          	auipc	ra,0x0
    80001fc4:	d34080e7          	jalr	-716(ra) # 80001cf4 <kpanic>
                kpanic("freelist: not sorted");
    80001fc8:	00003517          	auipc	a0,0x3
    80001fcc:	4b850513          	addi	a0,a0,1208 # 80005480 <_ZTVZL9e2e_testsvE3Foo+0xa0>
    80001fd0:	00000097          	auipc	ra,0x0
    80001fd4:	d24080e7          	jalr	-732(ra) # 80001cf4 <kpanic>
                kpanic("freelist: overlapping nodes");
    80001fd8:	00003517          	auipc	a0,0x3
    80001fdc:	4c050513          	addi	a0,a0,1216 # 80005498 <_ZTVZL9e2e_testsvE3Foo+0xb8>
    80001fe0:	00000097          	auipc	ra,0x0
    80001fe4:	d14080e7          	jalr	-748(ra) # 80001cf4 <kpanic>
        }
    }
}
    80001fe8:	00813083          	ld	ra,8(sp)
    80001fec:	00013403          	ld	s0,0(sp)
    80001ff0:	01010113          	addi	sp,sp,16
    80001ff4:	00008067          	ret
    80001ff8:	00008067          	ret

0000000080001ffc <_ZN15MemoryAllocator16total_free_bytesEv>:

// ---- stats ----------------------------------------------------------------

size_t MemoryAllocator::total_free_bytes() {
    80001ffc:	ff010113          	addi	sp,sp,-16
    80002000:	00813423          	sd	s0,8(sp)
    80002004:	01010413          	addi	s0,sp,16
    size_t total = 0;
    for (Header* p = freelist; p != nullptr; p = p->next)
    80002008:	00004797          	auipc	a5,0x4
    8000200c:	bf07b783          	ld	a5,-1040(a5) # 80005bf8 <_ZN15MemoryAllocator8freelistE>
    size_t total = 0;
    80002010:	00000513          	li	a0,0
    for (Header* p = freelist; p != nullptr; p = p->next)
    80002014:	00078c63          	beqz	a5,8000202c <_ZN15MemoryAllocator16total_free_bytesEv+0x30>
        total += p->blocks * MEM_BLOCK_SIZE;
    80002018:	0087b703          	ld	a4,8(a5)
    8000201c:	00671713          	slli	a4,a4,0x6
    80002020:	00e50533          	add	a0,a0,a4
    for (Header* p = freelist; p != nullptr; p = p->next)
    80002024:	0007b783          	ld	a5,0(a5)
    80002028:	fedff06f          	j	80002014 <_ZN15MemoryAllocator16total_free_bytesEv+0x18>
    return total;
}
    8000202c:	00813403          	ld	s0,8(sp)
    80002030:	01010113          	addi	sp,sp,16
    80002034:	00008067          	ret

0000000080002038 <mem_alloc>:
// bytes->blocks conversion before the trap.
//
// Inline-asm idiom: use register-named locals and "+r"(a0) so the compiler
// keeps the syscall number and the return value in the same physical register.

extern "C" void* mem_alloc(size_t size) {
    80002038:	ff010113          	addi	sp,sp,-16
    8000203c:	00813423          	sd	s0,8(sp)
    80002040:	01010413          	addi	s0,sp,16
    if (size == 0) return nullptr;
    80002044:	02050063          	beqz	a0,80002064 <mem_alloc+0x2c>
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    80002048:	03f50593          	addi	a1,a0,63

    register uint64 a0 asm("a0") = SYS_MEM_ALLOC;
    8000204c:	00100513          	li	a0,1
    register uint64 a1 asm("a1") = blocks;
    80002050:	0065d593          	srli	a1,a1,0x6
    asm volatile ("ecall"
                  : "+r"(a0)
                  : "r"(a1)
                  : "memory");
    80002054:	00000073          	ecall
    return (void*)a0;
}
    80002058:	00813403          	ld	s0,8(sp)
    8000205c:	01010113          	addi	sp,sp,16
    80002060:	00008067          	ret
    if (size == 0) return nullptr;
    80002064:	00000513          	li	a0,0
    80002068:	ff1ff06f          	j	80002058 <mem_alloc+0x20>

000000008000206c <mem_free>:

extern "C" int mem_free(void* ptr) {
    8000206c:	ff010113          	addi	sp,sp,-16
    80002070:	00813423          	sd	s0,8(sp)
    80002074:	01010413          	addi	s0,sp,16
    80002078:	00050593          	mv	a1,a0
    register uint64 a0 asm("a0") = SYS_MEM_FREE;
    8000207c:	00200513          	li	a0,2
    register uint64 a1 asm("a1") = (uint64)ptr;
    asm volatile ("ecall"
                  : "+r"(a0)
                  : "r"(a1)
                  : "memory");
    80002080:	00000073          	ecall
    return (int)a0;
}
    80002084:	0005051b          	sext.w	a0,a0
    80002088:	00813403          	ld	s0,8(sp)
    8000208c:	01010113          	addi	sp,sp,16
    80002090:	00008067          	ret

0000000080002094 <start>:
    80002094:	ff010113          	addi	sp,sp,-16
    80002098:	00813423          	sd	s0,8(sp)
    8000209c:	01010413          	addi	s0,sp,16
    800020a0:	300027f3          	csrr	a5,mstatus
    800020a4:	ffffe737          	lui	a4,0xffffe
    800020a8:	7ff70713          	addi	a4,a4,2047 # ffffffffffffe7ff <end+0xffffffff7fff796f>
    800020ac:	00e7f7b3          	and	a5,a5,a4
    800020b0:	00001737          	lui	a4,0x1
    800020b4:	80070713          	addi	a4,a4,-2048 # 800 <_entry-0x7ffff800>
    800020b8:	00e7e7b3          	or	a5,a5,a4
    800020bc:	30079073          	csrw	mstatus,a5
    800020c0:	00000797          	auipc	a5,0x0
    800020c4:	16078793          	addi	a5,a5,352 # 80002220 <system_main>
    800020c8:	34179073          	csrw	mepc,a5
    800020cc:	00000793          	li	a5,0
    800020d0:	18079073          	csrw	satp,a5
    800020d4:	000107b7          	lui	a5,0x10
    800020d8:	fff78793          	addi	a5,a5,-1 # ffff <_entry-0x7fff0001>
    800020dc:	30279073          	csrw	medeleg,a5
    800020e0:	30379073          	csrw	mideleg,a5
    800020e4:	104027f3          	csrr	a5,sie
    800020e8:	2227e793          	ori	a5,a5,546
    800020ec:	10479073          	csrw	sie,a5
    800020f0:	fff00793          	li	a5,-1
    800020f4:	00a7d793          	srli	a5,a5,0xa
    800020f8:	3b079073          	csrw	pmpaddr0,a5
    800020fc:	00f00793          	li	a5,15
    80002100:	3a079073          	csrw	pmpcfg0,a5
    80002104:	f14027f3          	csrr	a5,mhartid
    80002108:	0200c737          	lui	a4,0x200c
    8000210c:	ff873583          	ld	a1,-8(a4) # 200bff8 <_entry-0x7dff4008>
    80002110:	0007869b          	sext.w	a3,a5
    80002114:	00269713          	slli	a4,a3,0x2
    80002118:	000f4637          	lui	a2,0xf4
    8000211c:	24060613          	addi	a2,a2,576 # f4240 <_entry-0x7ff0bdc0>
    80002120:	00d70733          	add	a4,a4,a3
    80002124:	0037979b          	slliw	a5,a5,0x3
    80002128:	020046b7          	lui	a3,0x2004
    8000212c:	00d787b3          	add	a5,a5,a3
    80002130:	00c585b3          	add	a1,a1,a2
    80002134:	00371693          	slli	a3,a4,0x3
    80002138:	00004717          	auipc	a4,0x4
    8000213c:	af870713          	addi	a4,a4,-1288 # 80005c30 <timer_scratch>
    80002140:	00b7b023          	sd	a1,0(a5)
    80002144:	00d70733          	add	a4,a4,a3
    80002148:	00f73c23          	sd	a5,24(a4)
    8000214c:	02c73023          	sd	a2,32(a4)
    80002150:	34071073          	csrw	mscratch,a4
    80002154:	00000797          	auipc	a5,0x0
    80002158:	6ec78793          	addi	a5,a5,1772 # 80002840 <timervec>
    8000215c:	30579073          	csrw	mtvec,a5
    80002160:	300027f3          	csrr	a5,mstatus
    80002164:	0087e793          	ori	a5,a5,8
    80002168:	30079073          	csrw	mstatus,a5
    8000216c:	304027f3          	csrr	a5,mie
    80002170:	0807e793          	ori	a5,a5,128
    80002174:	30479073          	csrw	mie,a5
    80002178:	f14027f3          	csrr	a5,mhartid
    8000217c:	0007879b          	sext.w	a5,a5
    80002180:	00078213          	mv	tp,a5
    80002184:	30200073          	mret
    80002188:	00813403          	ld	s0,8(sp)
    8000218c:	01010113          	addi	sp,sp,16
    80002190:	00008067          	ret

0000000080002194 <timerinit>:
    80002194:	ff010113          	addi	sp,sp,-16
    80002198:	00813423          	sd	s0,8(sp)
    8000219c:	01010413          	addi	s0,sp,16
    800021a0:	f14027f3          	csrr	a5,mhartid
    800021a4:	0200c737          	lui	a4,0x200c
    800021a8:	ff873583          	ld	a1,-8(a4) # 200bff8 <_entry-0x7dff4008>
    800021ac:	0007869b          	sext.w	a3,a5
    800021b0:	00269713          	slli	a4,a3,0x2
    800021b4:	000f4637          	lui	a2,0xf4
    800021b8:	24060613          	addi	a2,a2,576 # f4240 <_entry-0x7ff0bdc0>
    800021bc:	00d70733          	add	a4,a4,a3
    800021c0:	0037979b          	slliw	a5,a5,0x3
    800021c4:	020046b7          	lui	a3,0x2004
    800021c8:	00d787b3          	add	a5,a5,a3
    800021cc:	00c585b3          	add	a1,a1,a2
    800021d0:	00371693          	slli	a3,a4,0x3
    800021d4:	00004717          	auipc	a4,0x4
    800021d8:	a5c70713          	addi	a4,a4,-1444 # 80005c30 <timer_scratch>
    800021dc:	00b7b023          	sd	a1,0(a5)
    800021e0:	00d70733          	add	a4,a4,a3
    800021e4:	00f73c23          	sd	a5,24(a4)
    800021e8:	02c73023          	sd	a2,32(a4)
    800021ec:	34071073          	csrw	mscratch,a4
    800021f0:	00000797          	auipc	a5,0x0
    800021f4:	65078793          	addi	a5,a5,1616 # 80002840 <timervec>
    800021f8:	30579073          	csrw	mtvec,a5
    800021fc:	300027f3          	csrr	a5,mstatus
    80002200:	0087e793          	ori	a5,a5,8
    80002204:	30079073          	csrw	mstatus,a5
    80002208:	304027f3          	csrr	a5,mie
    8000220c:	0807e793          	ori	a5,a5,128
    80002210:	30479073          	csrw	mie,a5
    80002214:	00813403          	ld	s0,8(sp)
    80002218:	01010113          	addi	sp,sp,16
    8000221c:	00008067          	ret

0000000080002220 <system_main>:
    80002220:	fe010113          	addi	sp,sp,-32
    80002224:	00813823          	sd	s0,16(sp)
    80002228:	00913423          	sd	s1,8(sp)
    8000222c:	00113c23          	sd	ra,24(sp)
    80002230:	02010413          	addi	s0,sp,32
    80002234:	00000097          	auipc	ra,0x0
    80002238:	0c4080e7          	jalr	196(ra) # 800022f8 <cpuid>
    8000223c:	00004497          	auipc	s1,0x4
    80002240:	9c448493          	addi	s1,s1,-1596 # 80005c00 <started>
    80002244:	02050263          	beqz	a0,80002268 <system_main+0x48>
    80002248:	0004a783          	lw	a5,0(s1)
    8000224c:	0007879b          	sext.w	a5,a5
    80002250:	fe078ce3          	beqz	a5,80002248 <system_main+0x28>
    80002254:	0ff0000f          	fence
    80002258:	00003517          	auipc	a0,0x3
    8000225c:	2b850513          	addi	a0,a0,696 # 80005510 <_ZTVZL9e2e_testsvE3Foo+0x130>
    80002260:	00001097          	auipc	ra,0x1
    80002264:	a7c080e7          	jalr	-1412(ra) # 80002cdc <panic>
    80002268:	00001097          	auipc	ra,0x1
    8000226c:	9d0080e7          	jalr	-1584(ra) # 80002c38 <consoleinit>
    80002270:	00001097          	auipc	ra,0x1
    80002274:	15c080e7          	jalr	348(ra) # 800033cc <printfinit>
    80002278:	00003517          	auipc	a0,0x3
    8000227c:	37850513          	addi	a0,a0,888 # 800055f0 <_ZTVZL9e2e_testsvE3Foo+0x210>
    80002280:	00001097          	auipc	ra,0x1
    80002284:	ab8080e7          	jalr	-1352(ra) # 80002d38 <__printf>
    80002288:	00003517          	auipc	a0,0x3
    8000228c:	25850513          	addi	a0,a0,600 # 800054e0 <_ZTVZL9e2e_testsvE3Foo+0x100>
    80002290:	00001097          	auipc	ra,0x1
    80002294:	aa8080e7          	jalr	-1368(ra) # 80002d38 <__printf>
    80002298:	00003517          	auipc	a0,0x3
    8000229c:	35850513          	addi	a0,a0,856 # 800055f0 <_ZTVZL9e2e_testsvE3Foo+0x210>
    800022a0:	00001097          	auipc	ra,0x1
    800022a4:	a98080e7          	jalr	-1384(ra) # 80002d38 <__printf>
    800022a8:	00001097          	auipc	ra,0x1
    800022ac:	4b0080e7          	jalr	1200(ra) # 80003758 <kinit>
    800022b0:	00000097          	auipc	ra,0x0
    800022b4:	148080e7          	jalr	328(ra) # 800023f8 <trapinit>
    800022b8:	00000097          	auipc	ra,0x0
    800022bc:	16c080e7          	jalr	364(ra) # 80002424 <trapinithart>
    800022c0:	00000097          	auipc	ra,0x0
    800022c4:	5c0080e7          	jalr	1472(ra) # 80002880 <plicinit>
    800022c8:	00000097          	auipc	ra,0x0
    800022cc:	5e0080e7          	jalr	1504(ra) # 800028a8 <plicinithart>
    800022d0:	00000097          	auipc	ra,0x0
    800022d4:	078080e7          	jalr	120(ra) # 80002348 <userinit>
    800022d8:	0ff0000f          	fence
    800022dc:	00100793          	li	a5,1
    800022e0:	00003517          	auipc	a0,0x3
    800022e4:	21850513          	addi	a0,a0,536 # 800054f8 <_ZTVZL9e2e_testsvE3Foo+0x118>
    800022e8:	00f4a023          	sw	a5,0(s1)
    800022ec:	00001097          	auipc	ra,0x1
    800022f0:	a4c080e7          	jalr	-1460(ra) # 80002d38 <__printf>
    800022f4:	0000006f          	j	800022f4 <system_main+0xd4>

00000000800022f8 <cpuid>:
    800022f8:	ff010113          	addi	sp,sp,-16
    800022fc:	00813423          	sd	s0,8(sp)
    80002300:	01010413          	addi	s0,sp,16
    80002304:	00020513          	mv	a0,tp
    80002308:	00813403          	ld	s0,8(sp)
    8000230c:	0005051b          	sext.w	a0,a0
    80002310:	01010113          	addi	sp,sp,16
    80002314:	00008067          	ret

0000000080002318 <mycpu>:
    80002318:	ff010113          	addi	sp,sp,-16
    8000231c:	00813423          	sd	s0,8(sp)
    80002320:	01010413          	addi	s0,sp,16
    80002324:	00020793          	mv	a5,tp
    80002328:	00813403          	ld	s0,8(sp)
    8000232c:	0007879b          	sext.w	a5,a5
    80002330:	00779793          	slli	a5,a5,0x7
    80002334:	00005517          	auipc	a0,0x5
    80002338:	92c50513          	addi	a0,a0,-1748 # 80006c60 <cpus>
    8000233c:	00f50533          	add	a0,a0,a5
    80002340:	01010113          	addi	sp,sp,16
    80002344:	00008067          	ret

0000000080002348 <userinit>:
    80002348:	ff010113          	addi	sp,sp,-16
    8000234c:	00813423          	sd	s0,8(sp)
    80002350:	01010413          	addi	s0,sp,16
    80002354:	00813403          	ld	s0,8(sp)
    80002358:	01010113          	addi	sp,sp,16
    8000235c:	fffff317          	auipc	t1,0xfffff
    80002360:	51c30067          	jr	1308(t1) # 80001878 <main>

0000000080002364 <either_copyout>:
    80002364:	ff010113          	addi	sp,sp,-16
    80002368:	00813023          	sd	s0,0(sp)
    8000236c:	00113423          	sd	ra,8(sp)
    80002370:	01010413          	addi	s0,sp,16
    80002374:	02051663          	bnez	a0,800023a0 <either_copyout+0x3c>
    80002378:	00058513          	mv	a0,a1
    8000237c:	00060593          	mv	a1,a2
    80002380:	0006861b          	sext.w	a2,a3
    80002384:	00002097          	auipc	ra,0x2
    80002388:	c60080e7          	jalr	-928(ra) # 80003fe4 <__memmove>
    8000238c:	00813083          	ld	ra,8(sp)
    80002390:	00013403          	ld	s0,0(sp)
    80002394:	00000513          	li	a0,0
    80002398:	01010113          	addi	sp,sp,16
    8000239c:	00008067          	ret
    800023a0:	00003517          	auipc	a0,0x3
    800023a4:	19850513          	addi	a0,a0,408 # 80005538 <_ZTVZL9e2e_testsvE3Foo+0x158>
    800023a8:	00001097          	auipc	ra,0x1
    800023ac:	934080e7          	jalr	-1740(ra) # 80002cdc <panic>

00000000800023b0 <either_copyin>:
    800023b0:	ff010113          	addi	sp,sp,-16
    800023b4:	00813023          	sd	s0,0(sp)
    800023b8:	00113423          	sd	ra,8(sp)
    800023bc:	01010413          	addi	s0,sp,16
    800023c0:	02059463          	bnez	a1,800023e8 <either_copyin+0x38>
    800023c4:	00060593          	mv	a1,a2
    800023c8:	0006861b          	sext.w	a2,a3
    800023cc:	00002097          	auipc	ra,0x2
    800023d0:	c18080e7          	jalr	-1000(ra) # 80003fe4 <__memmove>
    800023d4:	00813083          	ld	ra,8(sp)
    800023d8:	00013403          	ld	s0,0(sp)
    800023dc:	00000513          	li	a0,0
    800023e0:	01010113          	addi	sp,sp,16
    800023e4:	00008067          	ret
    800023e8:	00003517          	auipc	a0,0x3
    800023ec:	17850513          	addi	a0,a0,376 # 80005560 <_ZTVZL9e2e_testsvE3Foo+0x180>
    800023f0:	00001097          	auipc	ra,0x1
    800023f4:	8ec080e7          	jalr	-1812(ra) # 80002cdc <panic>

00000000800023f8 <trapinit>:
    800023f8:	ff010113          	addi	sp,sp,-16
    800023fc:	00813423          	sd	s0,8(sp)
    80002400:	01010413          	addi	s0,sp,16
    80002404:	00813403          	ld	s0,8(sp)
    80002408:	00003597          	auipc	a1,0x3
    8000240c:	18058593          	addi	a1,a1,384 # 80005588 <_ZTVZL9e2e_testsvE3Foo+0x1a8>
    80002410:	00005517          	auipc	a0,0x5
    80002414:	8d050513          	addi	a0,a0,-1840 # 80006ce0 <tickslock>
    80002418:	01010113          	addi	sp,sp,16
    8000241c:	00001317          	auipc	t1,0x1
    80002420:	5cc30067          	jr	1484(t1) # 800039e8 <initlock>

0000000080002424 <trapinithart>:
    80002424:	ff010113          	addi	sp,sp,-16
    80002428:	00813423          	sd	s0,8(sp)
    8000242c:	01010413          	addi	s0,sp,16
    80002430:	00000797          	auipc	a5,0x0
    80002434:	30078793          	addi	a5,a5,768 # 80002730 <kernelvec>
    80002438:	10579073          	csrw	stvec,a5
    8000243c:	00813403          	ld	s0,8(sp)
    80002440:	01010113          	addi	sp,sp,16
    80002444:	00008067          	ret

0000000080002448 <usertrap>:
    80002448:	ff010113          	addi	sp,sp,-16
    8000244c:	00813423          	sd	s0,8(sp)
    80002450:	01010413          	addi	s0,sp,16
    80002454:	00813403          	ld	s0,8(sp)
    80002458:	01010113          	addi	sp,sp,16
    8000245c:	00008067          	ret

0000000080002460 <usertrapret>:
    80002460:	ff010113          	addi	sp,sp,-16
    80002464:	00813423          	sd	s0,8(sp)
    80002468:	01010413          	addi	s0,sp,16
    8000246c:	00813403          	ld	s0,8(sp)
    80002470:	01010113          	addi	sp,sp,16
    80002474:	00008067          	ret

0000000080002478 <kerneltrap>:
    80002478:	fe010113          	addi	sp,sp,-32
    8000247c:	00813823          	sd	s0,16(sp)
    80002480:	00113c23          	sd	ra,24(sp)
    80002484:	00913423          	sd	s1,8(sp)
    80002488:	02010413          	addi	s0,sp,32
    8000248c:	142025f3          	csrr	a1,scause
    80002490:	100027f3          	csrr	a5,sstatus
    80002494:	0027f793          	andi	a5,a5,2
    80002498:	10079c63          	bnez	a5,800025b0 <kerneltrap+0x138>
    8000249c:	142027f3          	csrr	a5,scause
    800024a0:	0207ce63          	bltz	a5,800024dc <kerneltrap+0x64>
    800024a4:	00003517          	auipc	a0,0x3
    800024a8:	12c50513          	addi	a0,a0,300 # 800055d0 <_ZTVZL9e2e_testsvE3Foo+0x1f0>
    800024ac:	00001097          	auipc	ra,0x1
    800024b0:	88c080e7          	jalr	-1908(ra) # 80002d38 <__printf>
    800024b4:	141025f3          	csrr	a1,sepc
    800024b8:	14302673          	csrr	a2,stval
    800024bc:	00003517          	auipc	a0,0x3
    800024c0:	12450513          	addi	a0,a0,292 # 800055e0 <_ZTVZL9e2e_testsvE3Foo+0x200>
    800024c4:	00001097          	auipc	ra,0x1
    800024c8:	874080e7          	jalr	-1932(ra) # 80002d38 <__printf>
    800024cc:	00003517          	auipc	a0,0x3
    800024d0:	12c50513          	addi	a0,a0,300 # 800055f8 <_ZTVZL9e2e_testsvE3Foo+0x218>
    800024d4:	00001097          	auipc	ra,0x1
    800024d8:	808080e7          	jalr	-2040(ra) # 80002cdc <panic>
    800024dc:	0ff7f713          	andi	a4,a5,255
    800024e0:	00900693          	li	a3,9
    800024e4:	04d70063          	beq	a4,a3,80002524 <kerneltrap+0xac>
    800024e8:	fff00713          	li	a4,-1
    800024ec:	03f71713          	slli	a4,a4,0x3f
    800024f0:	00170713          	addi	a4,a4,1
    800024f4:	fae798e3          	bne	a5,a4,800024a4 <kerneltrap+0x2c>
    800024f8:	00000097          	auipc	ra,0x0
    800024fc:	e00080e7          	jalr	-512(ra) # 800022f8 <cpuid>
    80002500:	06050663          	beqz	a0,8000256c <kerneltrap+0xf4>
    80002504:	144027f3          	csrr	a5,sip
    80002508:	ffd7f793          	andi	a5,a5,-3
    8000250c:	14479073          	csrw	sip,a5
    80002510:	01813083          	ld	ra,24(sp)
    80002514:	01013403          	ld	s0,16(sp)
    80002518:	00813483          	ld	s1,8(sp)
    8000251c:	02010113          	addi	sp,sp,32
    80002520:	00008067          	ret
    80002524:	00000097          	auipc	ra,0x0
    80002528:	3d0080e7          	jalr	976(ra) # 800028f4 <plic_claim>
    8000252c:	00a00793          	li	a5,10
    80002530:	00050493          	mv	s1,a0
    80002534:	06f50863          	beq	a0,a5,800025a4 <kerneltrap+0x12c>
    80002538:	fc050ce3          	beqz	a0,80002510 <kerneltrap+0x98>
    8000253c:	00050593          	mv	a1,a0
    80002540:	00003517          	auipc	a0,0x3
    80002544:	07050513          	addi	a0,a0,112 # 800055b0 <_ZTVZL9e2e_testsvE3Foo+0x1d0>
    80002548:	00000097          	auipc	ra,0x0
    8000254c:	7f0080e7          	jalr	2032(ra) # 80002d38 <__printf>
    80002550:	01013403          	ld	s0,16(sp)
    80002554:	01813083          	ld	ra,24(sp)
    80002558:	00048513          	mv	a0,s1
    8000255c:	00813483          	ld	s1,8(sp)
    80002560:	02010113          	addi	sp,sp,32
    80002564:	00000317          	auipc	t1,0x0
    80002568:	3c830067          	jr	968(t1) # 8000292c <plic_complete>
    8000256c:	00004517          	auipc	a0,0x4
    80002570:	77450513          	addi	a0,a0,1908 # 80006ce0 <tickslock>
    80002574:	00001097          	auipc	ra,0x1
    80002578:	498080e7          	jalr	1176(ra) # 80003a0c <acquire>
    8000257c:	00003717          	auipc	a4,0x3
    80002580:	68870713          	addi	a4,a4,1672 # 80005c04 <ticks>
    80002584:	00072783          	lw	a5,0(a4)
    80002588:	00004517          	auipc	a0,0x4
    8000258c:	75850513          	addi	a0,a0,1880 # 80006ce0 <tickslock>
    80002590:	0017879b          	addiw	a5,a5,1
    80002594:	00f72023          	sw	a5,0(a4)
    80002598:	00001097          	auipc	ra,0x1
    8000259c:	540080e7          	jalr	1344(ra) # 80003ad8 <release>
    800025a0:	f65ff06f          	j	80002504 <kerneltrap+0x8c>
    800025a4:	00001097          	auipc	ra,0x1
    800025a8:	09c080e7          	jalr	156(ra) # 80003640 <uartintr>
    800025ac:	fa5ff06f          	j	80002550 <kerneltrap+0xd8>
    800025b0:	00003517          	auipc	a0,0x3
    800025b4:	fe050513          	addi	a0,a0,-32 # 80005590 <_ZTVZL9e2e_testsvE3Foo+0x1b0>
    800025b8:	00000097          	auipc	ra,0x0
    800025bc:	724080e7          	jalr	1828(ra) # 80002cdc <panic>

00000000800025c0 <clockintr>:
    800025c0:	fe010113          	addi	sp,sp,-32
    800025c4:	00813823          	sd	s0,16(sp)
    800025c8:	00913423          	sd	s1,8(sp)
    800025cc:	00113c23          	sd	ra,24(sp)
    800025d0:	02010413          	addi	s0,sp,32
    800025d4:	00004497          	auipc	s1,0x4
    800025d8:	70c48493          	addi	s1,s1,1804 # 80006ce0 <tickslock>
    800025dc:	00048513          	mv	a0,s1
    800025e0:	00001097          	auipc	ra,0x1
    800025e4:	42c080e7          	jalr	1068(ra) # 80003a0c <acquire>
    800025e8:	00003717          	auipc	a4,0x3
    800025ec:	61c70713          	addi	a4,a4,1564 # 80005c04 <ticks>
    800025f0:	00072783          	lw	a5,0(a4)
    800025f4:	01013403          	ld	s0,16(sp)
    800025f8:	01813083          	ld	ra,24(sp)
    800025fc:	00048513          	mv	a0,s1
    80002600:	0017879b          	addiw	a5,a5,1
    80002604:	00813483          	ld	s1,8(sp)
    80002608:	00f72023          	sw	a5,0(a4)
    8000260c:	02010113          	addi	sp,sp,32
    80002610:	00001317          	auipc	t1,0x1
    80002614:	4c830067          	jr	1224(t1) # 80003ad8 <release>

0000000080002618 <devintr>:
    80002618:	142027f3          	csrr	a5,scause
    8000261c:	00000513          	li	a0,0
    80002620:	0007c463          	bltz	a5,80002628 <devintr+0x10>
    80002624:	00008067          	ret
    80002628:	fe010113          	addi	sp,sp,-32
    8000262c:	00813823          	sd	s0,16(sp)
    80002630:	00113c23          	sd	ra,24(sp)
    80002634:	00913423          	sd	s1,8(sp)
    80002638:	02010413          	addi	s0,sp,32
    8000263c:	0ff7f713          	andi	a4,a5,255
    80002640:	00900693          	li	a3,9
    80002644:	04d70c63          	beq	a4,a3,8000269c <devintr+0x84>
    80002648:	fff00713          	li	a4,-1
    8000264c:	03f71713          	slli	a4,a4,0x3f
    80002650:	00170713          	addi	a4,a4,1
    80002654:	00e78c63          	beq	a5,a4,8000266c <devintr+0x54>
    80002658:	01813083          	ld	ra,24(sp)
    8000265c:	01013403          	ld	s0,16(sp)
    80002660:	00813483          	ld	s1,8(sp)
    80002664:	02010113          	addi	sp,sp,32
    80002668:	00008067          	ret
    8000266c:	00000097          	auipc	ra,0x0
    80002670:	c8c080e7          	jalr	-884(ra) # 800022f8 <cpuid>
    80002674:	06050663          	beqz	a0,800026e0 <devintr+0xc8>
    80002678:	144027f3          	csrr	a5,sip
    8000267c:	ffd7f793          	andi	a5,a5,-3
    80002680:	14479073          	csrw	sip,a5
    80002684:	01813083          	ld	ra,24(sp)
    80002688:	01013403          	ld	s0,16(sp)
    8000268c:	00813483          	ld	s1,8(sp)
    80002690:	00200513          	li	a0,2
    80002694:	02010113          	addi	sp,sp,32
    80002698:	00008067          	ret
    8000269c:	00000097          	auipc	ra,0x0
    800026a0:	258080e7          	jalr	600(ra) # 800028f4 <plic_claim>
    800026a4:	00a00793          	li	a5,10
    800026a8:	00050493          	mv	s1,a0
    800026ac:	06f50663          	beq	a0,a5,80002718 <devintr+0x100>
    800026b0:	00100513          	li	a0,1
    800026b4:	fa0482e3          	beqz	s1,80002658 <devintr+0x40>
    800026b8:	00048593          	mv	a1,s1
    800026bc:	00003517          	auipc	a0,0x3
    800026c0:	ef450513          	addi	a0,a0,-268 # 800055b0 <_ZTVZL9e2e_testsvE3Foo+0x1d0>
    800026c4:	00000097          	auipc	ra,0x0
    800026c8:	674080e7          	jalr	1652(ra) # 80002d38 <__printf>
    800026cc:	00048513          	mv	a0,s1
    800026d0:	00000097          	auipc	ra,0x0
    800026d4:	25c080e7          	jalr	604(ra) # 8000292c <plic_complete>
    800026d8:	00100513          	li	a0,1
    800026dc:	f7dff06f          	j	80002658 <devintr+0x40>
    800026e0:	00004517          	auipc	a0,0x4
    800026e4:	60050513          	addi	a0,a0,1536 # 80006ce0 <tickslock>
    800026e8:	00001097          	auipc	ra,0x1
    800026ec:	324080e7          	jalr	804(ra) # 80003a0c <acquire>
    800026f0:	00003717          	auipc	a4,0x3
    800026f4:	51470713          	addi	a4,a4,1300 # 80005c04 <ticks>
    800026f8:	00072783          	lw	a5,0(a4)
    800026fc:	00004517          	auipc	a0,0x4
    80002700:	5e450513          	addi	a0,a0,1508 # 80006ce0 <tickslock>
    80002704:	0017879b          	addiw	a5,a5,1
    80002708:	00f72023          	sw	a5,0(a4)
    8000270c:	00001097          	auipc	ra,0x1
    80002710:	3cc080e7          	jalr	972(ra) # 80003ad8 <release>
    80002714:	f65ff06f          	j	80002678 <devintr+0x60>
    80002718:	00001097          	auipc	ra,0x1
    8000271c:	f28080e7          	jalr	-216(ra) # 80003640 <uartintr>
    80002720:	fadff06f          	j	800026cc <devintr+0xb4>
	...

0000000080002730 <kernelvec>:
    80002730:	f0010113          	addi	sp,sp,-256
    80002734:	00113023          	sd	ra,0(sp)
    80002738:	00213423          	sd	sp,8(sp)
    8000273c:	00313823          	sd	gp,16(sp)
    80002740:	00413c23          	sd	tp,24(sp)
    80002744:	02513023          	sd	t0,32(sp)
    80002748:	02613423          	sd	t1,40(sp)
    8000274c:	02713823          	sd	t2,48(sp)
    80002750:	02813c23          	sd	s0,56(sp)
    80002754:	04913023          	sd	s1,64(sp)
    80002758:	04a13423          	sd	a0,72(sp)
    8000275c:	04b13823          	sd	a1,80(sp)
    80002760:	04c13c23          	sd	a2,88(sp)
    80002764:	06d13023          	sd	a3,96(sp)
    80002768:	06e13423          	sd	a4,104(sp)
    8000276c:	06f13823          	sd	a5,112(sp)
    80002770:	07013c23          	sd	a6,120(sp)
    80002774:	09113023          	sd	a7,128(sp)
    80002778:	09213423          	sd	s2,136(sp)
    8000277c:	09313823          	sd	s3,144(sp)
    80002780:	09413c23          	sd	s4,152(sp)
    80002784:	0b513023          	sd	s5,160(sp)
    80002788:	0b613423          	sd	s6,168(sp)
    8000278c:	0b713823          	sd	s7,176(sp)
    80002790:	0b813c23          	sd	s8,184(sp)
    80002794:	0d913023          	sd	s9,192(sp)
    80002798:	0da13423          	sd	s10,200(sp)
    8000279c:	0db13823          	sd	s11,208(sp)
    800027a0:	0dc13c23          	sd	t3,216(sp)
    800027a4:	0fd13023          	sd	t4,224(sp)
    800027a8:	0fe13423          	sd	t5,232(sp)
    800027ac:	0ff13823          	sd	t6,240(sp)
    800027b0:	cc9ff0ef          	jal	ra,80002478 <kerneltrap>
    800027b4:	00013083          	ld	ra,0(sp)
    800027b8:	00813103          	ld	sp,8(sp)
    800027bc:	01013183          	ld	gp,16(sp)
    800027c0:	02013283          	ld	t0,32(sp)
    800027c4:	02813303          	ld	t1,40(sp)
    800027c8:	03013383          	ld	t2,48(sp)
    800027cc:	03813403          	ld	s0,56(sp)
    800027d0:	04013483          	ld	s1,64(sp)
    800027d4:	04813503          	ld	a0,72(sp)
    800027d8:	05013583          	ld	a1,80(sp)
    800027dc:	05813603          	ld	a2,88(sp)
    800027e0:	06013683          	ld	a3,96(sp)
    800027e4:	06813703          	ld	a4,104(sp)
    800027e8:	07013783          	ld	a5,112(sp)
    800027ec:	07813803          	ld	a6,120(sp)
    800027f0:	08013883          	ld	a7,128(sp)
    800027f4:	08813903          	ld	s2,136(sp)
    800027f8:	09013983          	ld	s3,144(sp)
    800027fc:	09813a03          	ld	s4,152(sp)
    80002800:	0a013a83          	ld	s5,160(sp)
    80002804:	0a813b03          	ld	s6,168(sp)
    80002808:	0b013b83          	ld	s7,176(sp)
    8000280c:	0b813c03          	ld	s8,184(sp)
    80002810:	0c013c83          	ld	s9,192(sp)
    80002814:	0c813d03          	ld	s10,200(sp)
    80002818:	0d013d83          	ld	s11,208(sp)
    8000281c:	0d813e03          	ld	t3,216(sp)
    80002820:	0e013e83          	ld	t4,224(sp)
    80002824:	0e813f03          	ld	t5,232(sp)
    80002828:	0f013f83          	ld	t6,240(sp)
    8000282c:	10010113          	addi	sp,sp,256
    80002830:	10200073          	sret
    80002834:	00000013          	nop
    80002838:	00000013          	nop
    8000283c:	00000013          	nop

0000000080002840 <timervec>:
    80002840:	34051573          	csrrw	a0,mscratch,a0
    80002844:	00b53023          	sd	a1,0(a0)
    80002848:	00c53423          	sd	a2,8(a0)
    8000284c:	00d53823          	sd	a3,16(a0)
    80002850:	01853583          	ld	a1,24(a0)
    80002854:	02053603          	ld	a2,32(a0)
    80002858:	0005b683          	ld	a3,0(a1)
    8000285c:	00c686b3          	add	a3,a3,a2
    80002860:	00d5b023          	sd	a3,0(a1)
    80002864:	00200593          	li	a1,2
    80002868:	14459073          	csrw	sip,a1
    8000286c:	01053683          	ld	a3,16(a0)
    80002870:	00853603          	ld	a2,8(a0)
    80002874:	00053583          	ld	a1,0(a0)
    80002878:	34051573          	csrrw	a0,mscratch,a0
    8000287c:	30200073          	mret

0000000080002880 <plicinit>:
    80002880:	ff010113          	addi	sp,sp,-16
    80002884:	00813423          	sd	s0,8(sp)
    80002888:	01010413          	addi	s0,sp,16
    8000288c:	00813403          	ld	s0,8(sp)
    80002890:	0c0007b7          	lui	a5,0xc000
    80002894:	00100713          	li	a4,1
    80002898:	02e7a423          	sw	a4,40(a5) # c000028 <_entry-0x73ffffd8>
    8000289c:	00e7a223          	sw	a4,4(a5)
    800028a0:	01010113          	addi	sp,sp,16
    800028a4:	00008067          	ret

00000000800028a8 <plicinithart>:
    800028a8:	ff010113          	addi	sp,sp,-16
    800028ac:	00813023          	sd	s0,0(sp)
    800028b0:	00113423          	sd	ra,8(sp)
    800028b4:	01010413          	addi	s0,sp,16
    800028b8:	00000097          	auipc	ra,0x0
    800028bc:	a40080e7          	jalr	-1472(ra) # 800022f8 <cpuid>
    800028c0:	0085171b          	slliw	a4,a0,0x8
    800028c4:	0c0027b7          	lui	a5,0xc002
    800028c8:	00e787b3          	add	a5,a5,a4
    800028cc:	40200713          	li	a4,1026
    800028d0:	08e7a023          	sw	a4,128(a5) # c002080 <_entry-0x73ffdf80>
    800028d4:	00813083          	ld	ra,8(sp)
    800028d8:	00013403          	ld	s0,0(sp)
    800028dc:	00d5151b          	slliw	a0,a0,0xd
    800028e0:	0c2017b7          	lui	a5,0xc201
    800028e4:	00a78533          	add	a0,a5,a0
    800028e8:	00052023          	sw	zero,0(a0)
    800028ec:	01010113          	addi	sp,sp,16
    800028f0:	00008067          	ret

00000000800028f4 <plic_claim>:
    800028f4:	ff010113          	addi	sp,sp,-16
    800028f8:	00813023          	sd	s0,0(sp)
    800028fc:	00113423          	sd	ra,8(sp)
    80002900:	01010413          	addi	s0,sp,16
    80002904:	00000097          	auipc	ra,0x0
    80002908:	9f4080e7          	jalr	-1548(ra) # 800022f8 <cpuid>
    8000290c:	00813083          	ld	ra,8(sp)
    80002910:	00013403          	ld	s0,0(sp)
    80002914:	00d5151b          	slliw	a0,a0,0xd
    80002918:	0c2017b7          	lui	a5,0xc201
    8000291c:	00a78533          	add	a0,a5,a0
    80002920:	00452503          	lw	a0,4(a0)
    80002924:	01010113          	addi	sp,sp,16
    80002928:	00008067          	ret

000000008000292c <plic_complete>:
    8000292c:	fe010113          	addi	sp,sp,-32
    80002930:	00813823          	sd	s0,16(sp)
    80002934:	00913423          	sd	s1,8(sp)
    80002938:	00113c23          	sd	ra,24(sp)
    8000293c:	02010413          	addi	s0,sp,32
    80002940:	00050493          	mv	s1,a0
    80002944:	00000097          	auipc	ra,0x0
    80002948:	9b4080e7          	jalr	-1612(ra) # 800022f8 <cpuid>
    8000294c:	01813083          	ld	ra,24(sp)
    80002950:	01013403          	ld	s0,16(sp)
    80002954:	00d5179b          	slliw	a5,a0,0xd
    80002958:	0c201737          	lui	a4,0xc201
    8000295c:	00f707b3          	add	a5,a4,a5
    80002960:	0097a223          	sw	s1,4(a5) # c201004 <_entry-0x73dfeffc>
    80002964:	00813483          	ld	s1,8(sp)
    80002968:	02010113          	addi	sp,sp,32
    8000296c:	00008067          	ret

0000000080002970 <consolewrite>:
    80002970:	fb010113          	addi	sp,sp,-80
    80002974:	04813023          	sd	s0,64(sp)
    80002978:	04113423          	sd	ra,72(sp)
    8000297c:	02913c23          	sd	s1,56(sp)
    80002980:	03213823          	sd	s2,48(sp)
    80002984:	03313423          	sd	s3,40(sp)
    80002988:	03413023          	sd	s4,32(sp)
    8000298c:	01513c23          	sd	s5,24(sp)
    80002990:	05010413          	addi	s0,sp,80
    80002994:	06c05c63          	blez	a2,80002a0c <consolewrite+0x9c>
    80002998:	00060993          	mv	s3,a2
    8000299c:	00050a13          	mv	s4,a0
    800029a0:	00058493          	mv	s1,a1
    800029a4:	00000913          	li	s2,0
    800029a8:	fff00a93          	li	s5,-1
    800029ac:	01c0006f          	j	800029c8 <consolewrite+0x58>
    800029b0:	fbf44503          	lbu	a0,-65(s0)
    800029b4:	0019091b          	addiw	s2,s2,1
    800029b8:	00148493          	addi	s1,s1,1
    800029bc:	00001097          	auipc	ra,0x1
    800029c0:	a9c080e7          	jalr	-1380(ra) # 80003458 <uartputc>
    800029c4:	03298063          	beq	s3,s2,800029e4 <consolewrite+0x74>
    800029c8:	00048613          	mv	a2,s1
    800029cc:	00100693          	li	a3,1
    800029d0:	000a0593          	mv	a1,s4
    800029d4:	fbf40513          	addi	a0,s0,-65
    800029d8:	00000097          	auipc	ra,0x0
    800029dc:	9d8080e7          	jalr	-1576(ra) # 800023b0 <either_copyin>
    800029e0:	fd5518e3          	bne	a0,s5,800029b0 <consolewrite+0x40>
    800029e4:	04813083          	ld	ra,72(sp)
    800029e8:	04013403          	ld	s0,64(sp)
    800029ec:	03813483          	ld	s1,56(sp)
    800029f0:	02813983          	ld	s3,40(sp)
    800029f4:	02013a03          	ld	s4,32(sp)
    800029f8:	01813a83          	ld	s5,24(sp)
    800029fc:	00090513          	mv	a0,s2
    80002a00:	03013903          	ld	s2,48(sp)
    80002a04:	05010113          	addi	sp,sp,80
    80002a08:	00008067          	ret
    80002a0c:	00000913          	li	s2,0
    80002a10:	fd5ff06f          	j	800029e4 <consolewrite+0x74>

0000000080002a14 <consoleread>:
    80002a14:	f9010113          	addi	sp,sp,-112
    80002a18:	06813023          	sd	s0,96(sp)
    80002a1c:	04913c23          	sd	s1,88(sp)
    80002a20:	05213823          	sd	s2,80(sp)
    80002a24:	05313423          	sd	s3,72(sp)
    80002a28:	05413023          	sd	s4,64(sp)
    80002a2c:	03513c23          	sd	s5,56(sp)
    80002a30:	03613823          	sd	s6,48(sp)
    80002a34:	03713423          	sd	s7,40(sp)
    80002a38:	03813023          	sd	s8,32(sp)
    80002a3c:	06113423          	sd	ra,104(sp)
    80002a40:	01913c23          	sd	s9,24(sp)
    80002a44:	07010413          	addi	s0,sp,112
    80002a48:	00060b93          	mv	s7,a2
    80002a4c:	00050913          	mv	s2,a0
    80002a50:	00058c13          	mv	s8,a1
    80002a54:	00060b1b          	sext.w	s6,a2
    80002a58:	00004497          	auipc	s1,0x4
    80002a5c:	2b048493          	addi	s1,s1,688 # 80006d08 <cons>
    80002a60:	00400993          	li	s3,4
    80002a64:	fff00a13          	li	s4,-1
    80002a68:	00a00a93          	li	s5,10
    80002a6c:	05705e63          	blez	s7,80002ac8 <consoleread+0xb4>
    80002a70:	09c4a703          	lw	a4,156(s1)
    80002a74:	0984a783          	lw	a5,152(s1)
    80002a78:	0007071b          	sext.w	a4,a4
    80002a7c:	08e78463          	beq	a5,a4,80002b04 <consoleread+0xf0>
    80002a80:	07f7f713          	andi	a4,a5,127
    80002a84:	00e48733          	add	a4,s1,a4
    80002a88:	01874703          	lbu	a4,24(a4) # c201018 <_entry-0x73dfefe8>
    80002a8c:	0017869b          	addiw	a3,a5,1
    80002a90:	08d4ac23          	sw	a3,152(s1)
    80002a94:	00070c9b          	sext.w	s9,a4
    80002a98:	0b370663          	beq	a4,s3,80002b44 <consoleread+0x130>
    80002a9c:	00100693          	li	a3,1
    80002aa0:	f9f40613          	addi	a2,s0,-97
    80002aa4:	000c0593          	mv	a1,s8
    80002aa8:	00090513          	mv	a0,s2
    80002aac:	f8e40fa3          	sb	a4,-97(s0)
    80002ab0:	00000097          	auipc	ra,0x0
    80002ab4:	8b4080e7          	jalr	-1868(ra) # 80002364 <either_copyout>
    80002ab8:	01450863          	beq	a0,s4,80002ac8 <consoleread+0xb4>
    80002abc:	001c0c13          	addi	s8,s8,1
    80002ac0:	fffb8b9b          	addiw	s7,s7,-1
    80002ac4:	fb5c94e3          	bne	s9,s5,80002a6c <consoleread+0x58>
    80002ac8:	000b851b          	sext.w	a0,s7
    80002acc:	06813083          	ld	ra,104(sp)
    80002ad0:	06013403          	ld	s0,96(sp)
    80002ad4:	05813483          	ld	s1,88(sp)
    80002ad8:	05013903          	ld	s2,80(sp)
    80002adc:	04813983          	ld	s3,72(sp)
    80002ae0:	04013a03          	ld	s4,64(sp)
    80002ae4:	03813a83          	ld	s5,56(sp)
    80002ae8:	02813b83          	ld	s7,40(sp)
    80002aec:	02013c03          	ld	s8,32(sp)
    80002af0:	01813c83          	ld	s9,24(sp)
    80002af4:	40ab053b          	subw	a0,s6,a0
    80002af8:	03013b03          	ld	s6,48(sp)
    80002afc:	07010113          	addi	sp,sp,112
    80002b00:	00008067          	ret
    80002b04:	00001097          	auipc	ra,0x1
    80002b08:	1d8080e7          	jalr	472(ra) # 80003cdc <push_on>
    80002b0c:	0984a703          	lw	a4,152(s1)
    80002b10:	09c4a783          	lw	a5,156(s1)
    80002b14:	0007879b          	sext.w	a5,a5
    80002b18:	fef70ce3          	beq	a4,a5,80002b10 <consoleread+0xfc>
    80002b1c:	00001097          	auipc	ra,0x1
    80002b20:	234080e7          	jalr	564(ra) # 80003d50 <pop_on>
    80002b24:	0984a783          	lw	a5,152(s1)
    80002b28:	07f7f713          	andi	a4,a5,127
    80002b2c:	00e48733          	add	a4,s1,a4
    80002b30:	01874703          	lbu	a4,24(a4)
    80002b34:	0017869b          	addiw	a3,a5,1
    80002b38:	08d4ac23          	sw	a3,152(s1)
    80002b3c:	00070c9b          	sext.w	s9,a4
    80002b40:	f5371ee3          	bne	a4,s3,80002a9c <consoleread+0x88>
    80002b44:	000b851b          	sext.w	a0,s7
    80002b48:	f96bf2e3          	bgeu	s7,s6,80002acc <consoleread+0xb8>
    80002b4c:	08f4ac23          	sw	a5,152(s1)
    80002b50:	f7dff06f          	j	80002acc <consoleread+0xb8>

0000000080002b54 <consputc>:
    80002b54:	10000793          	li	a5,256
    80002b58:	00f50663          	beq	a0,a5,80002b64 <consputc+0x10>
    80002b5c:	00001317          	auipc	t1,0x1
    80002b60:	9f430067          	jr	-1548(t1) # 80003550 <uartputc_sync>
    80002b64:	ff010113          	addi	sp,sp,-16
    80002b68:	00113423          	sd	ra,8(sp)
    80002b6c:	00813023          	sd	s0,0(sp)
    80002b70:	01010413          	addi	s0,sp,16
    80002b74:	00800513          	li	a0,8
    80002b78:	00001097          	auipc	ra,0x1
    80002b7c:	9d8080e7          	jalr	-1576(ra) # 80003550 <uartputc_sync>
    80002b80:	02000513          	li	a0,32
    80002b84:	00001097          	auipc	ra,0x1
    80002b88:	9cc080e7          	jalr	-1588(ra) # 80003550 <uartputc_sync>
    80002b8c:	00013403          	ld	s0,0(sp)
    80002b90:	00813083          	ld	ra,8(sp)
    80002b94:	00800513          	li	a0,8
    80002b98:	01010113          	addi	sp,sp,16
    80002b9c:	00001317          	auipc	t1,0x1
    80002ba0:	9b430067          	jr	-1612(t1) # 80003550 <uartputc_sync>

0000000080002ba4 <consoleintr>:
    80002ba4:	fe010113          	addi	sp,sp,-32
    80002ba8:	00813823          	sd	s0,16(sp)
    80002bac:	00913423          	sd	s1,8(sp)
    80002bb0:	01213023          	sd	s2,0(sp)
    80002bb4:	00113c23          	sd	ra,24(sp)
    80002bb8:	02010413          	addi	s0,sp,32
    80002bbc:	00004917          	auipc	s2,0x4
    80002bc0:	14c90913          	addi	s2,s2,332 # 80006d08 <cons>
    80002bc4:	00050493          	mv	s1,a0
    80002bc8:	00090513          	mv	a0,s2
    80002bcc:	00001097          	auipc	ra,0x1
    80002bd0:	e40080e7          	jalr	-448(ra) # 80003a0c <acquire>
    80002bd4:	02048c63          	beqz	s1,80002c0c <consoleintr+0x68>
    80002bd8:	0a092783          	lw	a5,160(s2)
    80002bdc:	09892703          	lw	a4,152(s2)
    80002be0:	07f00693          	li	a3,127
    80002be4:	40e7873b          	subw	a4,a5,a4
    80002be8:	02e6e263          	bltu	a3,a4,80002c0c <consoleintr+0x68>
    80002bec:	00d00713          	li	a4,13
    80002bf0:	04e48063          	beq	s1,a4,80002c30 <consoleintr+0x8c>
    80002bf4:	07f7f713          	andi	a4,a5,127
    80002bf8:	00e90733          	add	a4,s2,a4
    80002bfc:	0017879b          	addiw	a5,a5,1
    80002c00:	0af92023          	sw	a5,160(s2)
    80002c04:	00970c23          	sb	s1,24(a4)
    80002c08:	08f92e23          	sw	a5,156(s2)
    80002c0c:	01013403          	ld	s0,16(sp)
    80002c10:	01813083          	ld	ra,24(sp)
    80002c14:	00813483          	ld	s1,8(sp)
    80002c18:	00013903          	ld	s2,0(sp)
    80002c1c:	00004517          	auipc	a0,0x4
    80002c20:	0ec50513          	addi	a0,a0,236 # 80006d08 <cons>
    80002c24:	02010113          	addi	sp,sp,32
    80002c28:	00001317          	auipc	t1,0x1
    80002c2c:	eb030067          	jr	-336(t1) # 80003ad8 <release>
    80002c30:	00a00493          	li	s1,10
    80002c34:	fc1ff06f          	j	80002bf4 <consoleintr+0x50>

0000000080002c38 <consoleinit>:
    80002c38:	fe010113          	addi	sp,sp,-32
    80002c3c:	00113c23          	sd	ra,24(sp)
    80002c40:	00813823          	sd	s0,16(sp)
    80002c44:	00913423          	sd	s1,8(sp)
    80002c48:	02010413          	addi	s0,sp,32
    80002c4c:	00004497          	auipc	s1,0x4
    80002c50:	0bc48493          	addi	s1,s1,188 # 80006d08 <cons>
    80002c54:	00048513          	mv	a0,s1
    80002c58:	00003597          	auipc	a1,0x3
    80002c5c:	9b058593          	addi	a1,a1,-1616 # 80005608 <_ZTVZL9e2e_testsvE3Foo+0x228>
    80002c60:	00001097          	auipc	ra,0x1
    80002c64:	d88080e7          	jalr	-632(ra) # 800039e8 <initlock>
    80002c68:	00000097          	auipc	ra,0x0
    80002c6c:	7ac080e7          	jalr	1964(ra) # 80003414 <uartinit>
    80002c70:	01813083          	ld	ra,24(sp)
    80002c74:	01013403          	ld	s0,16(sp)
    80002c78:	00000797          	auipc	a5,0x0
    80002c7c:	d9c78793          	addi	a5,a5,-612 # 80002a14 <consoleread>
    80002c80:	0af4bc23          	sd	a5,184(s1)
    80002c84:	00000797          	auipc	a5,0x0
    80002c88:	cec78793          	addi	a5,a5,-788 # 80002970 <consolewrite>
    80002c8c:	0cf4b023          	sd	a5,192(s1)
    80002c90:	00813483          	ld	s1,8(sp)
    80002c94:	02010113          	addi	sp,sp,32
    80002c98:	00008067          	ret

0000000080002c9c <console_read>:
    80002c9c:	ff010113          	addi	sp,sp,-16
    80002ca0:	00813423          	sd	s0,8(sp)
    80002ca4:	01010413          	addi	s0,sp,16
    80002ca8:	00813403          	ld	s0,8(sp)
    80002cac:	00004317          	auipc	t1,0x4
    80002cb0:	11433303          	ld	t1,276(t1) # 80006dc0 <devsw+0x10>
    80002cb4:	01010113          	addi	sp,sp,16
    80002cb8:	00030067          	jr	t1

0000000080002cbc <console_write>:
    80002cbc:	ff010113          	addi	sp,sp,-16
    80002cc0:	00813423          	sd	s0,8(sp)
    80002cc4:	01010413          	addi	s0,sp,16
    80002cc8:	00813403          	ld	s0,8(sp)
    80002ccc:	00004317          	auipc	t1,0x4
    80002cd0:	0fc33303          	ld	t1,252(t1) # 80006dc8 <devsw+0x18>
    80002cd4:	01010113          	addi	sp,sp,16
    80002cd8:	00030067          	jr	t1

0000000080002cdc <panic>:
    80002cdc:	fe010113          	addi	sp,sp,-32
    80002ce0:	00113c23          	sd	ra,24(sp)
    80002ce4:	00813823          	sd	s0,16(sp)
    80002ce8:	00913423          	sd	s1,8(sp)
    80002cec:	02010413          	addi	s0,sp,32
    80002cf0:	00050493          	mv	s1,a0
    80002cf4:	00003517          	auipc	a0,0x3
    80002cf8:	91c50513          	addi	a0,a0,-1764 # 80005610 <_ZTVZL9e2e_testsvE3Foo+0x230>
    80002cfc:	00004797          	auipc	a5,0x4
    80002d00:	1607a623          	sw	zero,364(a5) # 80006e68 <pr+0x18>
    80002d04:	00000097          	auipc	ra,0x0
    80002d08:	034080e7          	jalr	52(ra) # 80002d38 <__printf>
    80002d0c:	00048513          	mv	a0,s1
    80002d10:	00000097          	auipc	ra,0x0
    80002d14:	028080e7          	jalr	40(ra) # 80002d38 <__printf>
    80002d18:	00003517          	auipc	a0,0x3
    80002d1c:	8d850513          	addi	a0,a0,-1832 # 800055f0 <_ZTVZL9e2e_testsvE3Foo+0x210>
    80002d20:	00000097          	auipc	ra,0x0
    80002d24:	018080e7          	jalr	24(ra) # 80002d38 <__printf>
    80002d28:	00100793          	li	a5,1
    80002d2c:	00003717          	auipc	a4,0x3
    80002d30:	ecf72e23          	sw	a5,-292(a4) # 80005c08 <panicked>
    80002d34:	0000006f          	j	80002d34 <panic+0x58>

0000000080002d38 <__printf>:
    80002d38:	f3010113          	addi	sp,sp,-208
    80002d3c:	08813023          	sd	s0,128(sp)
    80002d40:	07313423          	sd	s3,104(sp)
    80002d44:	09010413          	addi	s0,sp,144
    80002d48:	05813023          	sd	s8,64(sp)
    80002d4c:	08113423          	sd	ra,136(sp)
    80002d50:	06913c23          	sd	s1,120(sp)
    80002d54:	07213823          	sd	s2,112(sp)
    80002d58:	07413023          	sd	s4,96(sp)
    80002d5c:	05513c23          	sd	s5,88(sp)
    80002d60:	05613823          	sd	s6,80(sp)
    80002d64:	05713423          	sd	s7,72(sp)
    80002d68:	03913c23          	sd	s9,56(sp)
    80002d6c:	03a13823          	sd	s10,48(sp)
    80002d70:	03b13423          	sd	s11,40(sp)
    80002d74:	00004317          	auipc	t1,0x4
    80002d78:	0dc30313          	addi	t1,t1,220 # 80006e50 <pr>
    80002d7c:	01832c03          	lw	s8,24(t1)
    80002d80:	00b43423          	sd	a1,8(s0)
    80002d84:	00c43823          	sd	a2,16(s0)
    80002d88:	00d43c23          	sd	a3,24(s0)
    80002d8c:	02e43023          	sd	a4,32(s0)
    80002d90:	02f43423          	sd	a5,40(s0)
    80002d94:	03043823          	sd	a6,48(s0)
    80002d98:	03143c23          	sd	a7,56(s0)
    80002d9c:	00050993          	mv	s3,a0
    80002da0:	4a0c1663          	bnez	s8,8000324c <__printf+0x514>
    80002da4:	60098c63          	beqz	s3,800033bc <__printf+0x684>
    80002da8:	0009c503          	lbu	a0,0(s3)
    80002dac:	00840793          	addi	a5,s0,8
    80002db0:	f6f43c23          	sd	a5,-136(s0)
    80002db4:	00000493          	li	s1,0
    80002db8:	22050063          	beqz	a0,80002fd8 <__printf+0x2a0>
    80002dbc:	00002a37          	lui	s4,0x2
    80002dc0:	00018ab7          	lui	s5,0x18
    80002dc4:	000f4b37          	lui	s6,0xf4
    80002dc8:	00989bb7          	lui	s7,0x989
    80002dcc:	70fa0a13          	addi	s4,s4,1807 # 270f <_entry-0x7fffd8f1>
    80002dd0:	69fa8a93          	addi	s5,s5,1695 # 1869f <_entry-0x7ffe7961>
    80002dd4:	23fb0b13          	addi	s6,s6,575 # f423f <_entry-0x7ff0bdc1>
    80002dd8:	67fb8b93          	addi	s7,s7,1663 # 98967f <_entry-0x7f676981>
    80002ddc:	00148c9b          	addiw	s9,s1,1
    80002de0:	02500793          	li	a5,37
    80002de4:	01998933          	add	s2,s3,s9
    80002de8:	38f51263          	bne	a0,a5,8000316c <__printf+0x434>
    80002dec:	00094783          	lbu	a5,0(s2)
    80002df0:	00078c9b          	sext.w	s9,a5
    80002df4:	1e078263          	beqz	a5,80002fd8 <__printf+0x2a0>
    80002df8:	0024849b          	addiw	s1,s1,2
    80002dfc:	07000713          	li	a4,112
    80002e00:	00998933          	add	s2,s3,s1
    80002e04:	38e78a63          	beq	a5,a4,80003198 <__printf+0x460>
    80002e08:	20f76863          	bltu	a4,a5,80003018 <__printf+0x2e0>
    80002e0c:	42a78863          	beq	a5,a0,8000323c <__printf+0x504>
    80002e10:	06400713          	li	a4,100
    80002e14:	40e79663          	bne	a5,a4,80003220 <__printf+0x4e8>
    80002e18:	f7843783          	ld	a5,-136(s0)
    80002e1c:	0007a603          	lw	a2,0(a5)
    80002e20:	00878793          	addi	a5,a5,8
    80002e24:	f6f43c23          	sd	a5,-136(s0)
    80002e28:	42064a63          	bltz	a2,8000325c <__printf+0x524>
    80002e2c:	00a00713          	li	a4,10
    80002e30:	02e677bb          	remuw	a5,a2,a4
    80002e34:	00003d97          	auipc	s11,0x3
    80002e38:	804d8d93          	addi	s11,s11,-2044 # 80005638 <digits>
    80002e3c:	00900593          	li	a1,9
    80002e40:	0006051b          	sext.w	a0,a2
    80002e44:	00000c93          	li	s9,0
    80002e48:	02079793          	slli	a5,a5,0x20
    80002e4c:	0207d793          	srli	a5,a5,0x20
    80002e50:	00fd87b3          	add	a5,s11,a5
    80002e54:	0007c783          	lbu	a5,0(a5)
    80002e58:	02e656bb          	divuw	a3,a2,a4
    80002e5c:	f8f40023          	sb	a5,-128(s0)
    80002e60:	14c5d863          	bge	a1,a2,80002fb0 <__printf+0x278>
    80002e64:	06300593          	li	a1,99
    80002e68:	00100c93          	li	s9,1
    80002e6c:	02e6f7bb          	remuw	a5,a3,a4
    80002e70:	02079793          	slli	a5,a5,0x20
    80002e74:	0207d793          	srli	a5,a5,0x20
    80002e78:	00fd87b3          	add	a5,s11,a5
    80002e7c:	0007c783          	lbu	a5,0(a5)
    80002e80:	02e6d73b          	divuw	a4,a3,a4
    80002e84:	f8f400a3          	sb	a5,-127(s0)
    80002e88:	12a5f463          	bgeu	a1,a0,80002fb0 <__printf+0x278>
    80002e8c:	00a00693          	li	a3,10
    80002e90:	00900593          	li	a1,9
    80002e94:	02d777bb          	remuw	a5,a4,a3
    80002e98:	02079793          	slli	a5,a5,0x20
    80002e9c:	0207d793          	srli	a5,a5,0x20
    80002ea0:	00fd87b3          	add	a5,s11,a5
    80002ea4:	0007c503          	lbu	a0,0(a5)
    80002ea8:	02d757bb          	divuw	a5,a4,a3
    80002eac:	f8a40123          	sb	a0,-126(s0)
    80002eb0:	48e5f263          	bgeu	a1,a4,80003334 <__printf+0x5fc>
    80002eb4:	06300513          	li	a0,99
    80002eb8:	02d7f5bb          	remuw	a1,a5,a3
    80002ebc:	02059593          	slli	a1,a1,0x20
    80002ec0:	0205d593          	srli	a1,a1,0x20
    80002ec4:	00bd85b3          	add	a1,s11,a1
    80002ec8:	0005c583          	lbu	a1,0(a1)
    80002ecc:	02d7d7bb          	divuw	a5,a5,a3
    80002ed0:	f8b401a3          	sb	a1,-125(s0)
    80002ed4:	48e57263          	bgeu	a0,a4,80003358 <__printf+0x620>
    80002ed8:	3e700513          	li	a0,999
    80002edc:	02d7f5bb          	remuw	a1,a5,a3
    80002ee0:	02059593          	slli	a1,a1,0x20
    80002ee4:	0205d593          	srli	a1,a1,0x20
    80002ee8:	00bd85b3          	add	a1,s11,a1
    80002eec:	0005c583          	lbu	a1,0(a1)
    80002ef0:	02d7d7bb          	divuw	a5,a5,a3
    80002ef4:	f8b40223          	sb	a1,-124(s0)
    80002ef8:	46e57663          	bgeu	a0,a4,80003364 <__printf+0x62c>
    80002efc:	02d7f5bb          	remuw	a1,a5,a3
    80002f00:	02059593          	slli	a1,a1,0x20
    80002f04:	0205d593          	srli	a1,a1,0x20
    80002f08:	00bd85b3          	add	a1,s11,a1
    80002f0c:	0005c583          	lbu	a1,0(a1)
    80002f10:	02d7d7bb          	divuw	a5,a5,a3
    80002f14:	f8b402a3          	sb	a1,-123(s0)
    80002f18:	46ea7863          	bgeu	s4,a4,80003388 <__printf+0x650>
    80002f1c:	02d7f5bb          	remuw	a1,a5,a3
    80002f20:	02059593          	slli	a1,a1,0x20
    80002f24:	0205d593          	srli	a1,a1,0x20
    80002f28:	00bd85b3          	add	a1,s11,a1
    80002f2c:	0005c583          	lbu	a1,0(a1)
    80002f30:	02d7d7bb          	divuw	a5,a5,a3
    80002f34:	f8b40323          	sb	a1,-122(s0)
    80002f38:	3eeaf863          	bgeu	s5,a4,80003328 <__printf+0x5f0>
    80002f3c:	02d7f5bb          	remuw	a1,a5,a3
    80002f40:	02059593          	slli	a1,a1,0x20
    80002f44:	0205d593          	srli	a1,a1,0x20
    80002f48:	00bd85b3          	add	a1,s11,a1
    80002f4c:	0005c583          	lbu	a1,0(a1)
    80002f50:	02d7d7bb          	divuw	a5,a5,a3
    80002f54:	f8b403a3          	sb	a1,-121(s0)
    80002f58:	42eb7e63          	bgeu	s6,a4,80003394 <__printf+0x65c>
    80002f5c:	02d7f5bb          	remuw	a1,a5,a3
    80002f60:	02059593          	slli	a1,a1,0x20
    80002f64:	0205d593          	srli	a1,a1,0x20
    80002f68:	00bd85b3          	add	a1,s11,a1
    80002f6c:	0005c583          	lbu	a1,0(a1)
    80002f70:	02d7d7bb          	divuw	a5,a5,a3
    80002f74:	f8b40423          	sb	a1,-120(s0)
    80002f78:	42ebfc63          	bgeu	s7,a4,800033b0 <__printf+0x678>
    80002f7c:	02079793          	slli	a5,a5,0x20
    80002f80:	0207d793          	srli	a5,a5,0x20
    80002f84:	00fd8db3          	add	s11,s11,a5
    80002f88:	000dc703          	lbu	a4,0(s11)
    80002f8c:	00a00793          	li	a5,10
    80002f90:	00900c93          	li	s9,9
    80002f94:	f8e404a3          	sb	a4,-119(s0)
    80002f98:	00065c63          	bgez	a2,80002fb0 <__printf+0x278>
    80002f9c:	f9040713          	addi	a4,s0,-112
    80002fa0:	00f70733          	add	a4,a4,a5
    80002fa4:	02d00693          	li	a3,45
    80002fa8:	fed70823          	sb	a3,-16(a4)
    80002fac:	00078c93          	mv	s9,a5
    80002fb0:	f8040793          	addi	a5,s0,-128
    80002fb4:	01978cb3          	add	s9,a5,s9
    80002fb8:	f7f40d13          	addi	s10,s0,-129
    80002fbc:	000cc503          	lbu	a0,0(s9)
    80002fc0:	fffc8c93          	addi	s9,s9,-1
    80002fc4:	00000097          	auipc	ra,0x0
    80002fc8:	b90080e7          	jalr	-1136(ra) # 80002b54 <consputc>
    80002fcc:	ffac98e3          	bne	s9,s10,80002fbc <__printf+0x284>
    80002fd0:	00094503          	lbu	a0,0(s2)
    80002fd4:	e00514e3          	bnez	a0,80002ddc <__printf+0xa4>
    80002fd8:	1a0c1663          	bnez	s8,80003184 <__printf+0x44c>
    80002fdc:	08813083          	ld	ra,136(sp)
    80002fe0:	08013403          	ld	s0,128(sp)
    80002fe4:	07813483          	ld	s1,120(sp)
    80002fe8:	07013903          	ld	s2,112(sp)
    80002fec:	06813983          	ld	s3,104(sp)
    80002ff0:	06013a03          	ld	s4,96(sp)
    80002ff4:	05813a83          	ld	s5,88(sp)
    80002ff8:	05013b03          	ld	s6,80(sp)
    80002ffc:	04813b83          	ld	s7,72(sp)
    80003000:	04013c03          	ld	s8,64(sp)
    80003004:	03813c83          	ld	s9,56(sp)
    80003008:	03013d03          	ld	s10,48(sp)
    8000300c:	02813d83          	ld	s11,40(sp)
    80003010:	0d010113          	addi	sp,sp,208
    80003014:	00008067          	ret
    80003018:	07300713          	li	a4,115
    8000301c:	1ce78a63          	beq	a5,a4,800031f0 <__printf+0x4b8>
    80003020:	07800713          	li	a4,120
    80003024:	1ee79e63          	bne	a5,a4,80003220 <__printf+0x4e8>
    80003028:	f7843783          	ld	a5,-136(s0)
    8000302c:	0007a703          	lw	a4,0(a5)
    80003030:	00878793          	addi	a5,a5,8
    80003034:	f6f43c23          	sd	a5,-136(s0)
    80003038:	28074263          	bltz	a4,800032bc <__printf+0x584>
    8000303c:	00002d97          	auipc	s11,0x2
    80003040:	5fcd8d93          	addi	s11,s11,1532 # 80005638 <digits>
    80003044:	00f77793          	andi	a5,a4,15
    80003048:	00fd87b3          	add	a5,s11,a5
    8000304c:	0007c683          	lbu	a3,0(a5)
    80003050:	00f00613          	li	a2,15
    80003054:	0007079b          	sext.w	a5,a4
    80003058:	f8d40023          	sb	a3,-128(s0)
    8000305c:	0047559b          	srliw	a1,a4,0x4
    80003060:	0047569b          	srliw	a3,a4,0x4
    80003064:	00000c93          	li	s9,0
    80003068:	0ee65063          	bge	a2,a4,80003148 <__printf+0x410>
    8000306c:	00f6f693          	andi	a3,a3,15
    80003070:	00dd86b3          	add	a3,s11,a3
    80003074:	0006c683          	lbu	a3,0(a3) # 2004000 <_entry-0x7dffc000>
    80003078:	0087d79b          	srliw	a5,a5,0x8
    8000307c:	00100c93          	li	s9,1
    80003080:	f8d400a3          	sb	a3,-127(s0)
    80003084:	0cb67263          	bgeu	a2,a1,80003148 <__printf+0x410>
    80003088:	00f7f693          	andi	a3,a5,15
    8000308c:	00dd86b3          	add	a3,s11,a3
    80003090:	0006c583          	lbu	a1,0(a3)
    80003094:	00f00613          	li	a2,15
    80003098:	0047d69b          	srliw	a3,a5,0x4
    8000309c:	f8b40123          	sb	a1,-126(s0)
    800030a0:	0047d593          	srli	a1,a5,0x4
    800030a4:	28f67e63          	bgeu	a2,a5,80003340 <__printf+0x608>
    800030a8:	00f6f693          	andi	a3,a3,15
    800030ac:	00dd86b3          	add	a3,s11,a3
    800030b0:	0006c503          	lbu	a0,0(a3)
    800030b4:	0087d813          	srli	a6,a5,0x8
    800030b8:	0087d69b          	srliw	a3,a5,0x8
    800030bc:	f8a401a3          	sb	a0,-125(s0)
    800030c0:	28b67663          	bgeu	a2,a1,8000334c <__printf+0x614>
    800030c4:	00f6f693          	andi	a3,a3,15
    800030c8:	00dd86b3          	add	a3,s11,a3
    800030cc:	0006c583          	lbu	a1,0(a3)
    800030d0:	00c7d513          	srli	a0,a5,0xc
    800030d4:	00c7d69b          	srliw	a3,a5,0xc
    800030d8:	f8b40223          	sb	a1,-124(s0)
    800030dc:	29067a63          	bgeu	a2,a6,80003370 <__printf+0x638>
    800030e0:	00f6f693          	andi	a3,a3,15
    800030e4:	00dd86b3          	add	a3,s11,a3
    800030e8:	0006c583          	lbu	a1,0(a3)
    800030ec:	0107d813          	srli	a6,a5,0x10
    800030f0:	0107d69b          	srliw	a3,a5,0x10
    800030f4:	f8b402a3          	sb	a1,-123(s0)
    800030f8:	28a67263          	bgeu	a2,a0,8000337c <__printf+0x644>
    800030fc:	00f6f693          	andi	a3,a3,15
    80003100:	00dd86b3          	add	a3,s11,a3
    80003104:	0006c683          	lbu	a3,0(a3)
    80003108:	0147d79b          	srliw	a5,a5,0x14
    8000310c:	f8d40323          	sb	a3,-122(s0)
    80003110:	21067663          	bgeu	a2,a6,8000331c <__printf+0x5e4>
    80003114:	02079793          	slli	a5,a5,0x20
    80003118:	0207d793          	srli	a5,a5,0x20
    8000311c:	00fd8db3          	add	s11,s11,a5
    80003120:	000dc683          	lbu	a3,0(s11)
    80003124:	00800793          	li	a5,8
    80003128:	00700c93          	li	s9,7
    8000312c:	f8d403a3          	sb	a3,-121(s0)
    80003130:	00075c63          	bgez	a4,80003148 <__printf+0x410>
    80003134:	f9040713          	addi	a4,s0,-112
    80003138:	00f70733          	add	a4,a4,a5
    8000313c:	02d00693          	li	a3,45
    80003140:	fed70823          	sb	a3,-16(a4)
    80003144:	00078c93          	mv	s9,a5
    80003148:	f8040793          	addi	a5,s0,-128
    8000314c:	01978cb3          	add	s9,a5,s9
    80003150:	f7f40d13          	addi	s10,s0,-129
    80003154:	000cc503          	lbu	a0,0(s9)
    80003158:	fffc8c93          	addi	s9,s9,-1
    8000315c:	00000097          	auipc	ra,0x0
    80003160:	9f8080e7          	jalr	-1544(ra) # 80002b54 <consputc>
    80003164:	ff9d18e3          	bne	s10,s9,80003154 <__printf+0x41c>
    80003168:	0100006f          	j	80003178 <__printf+0x440>
    8000316c:	00000097          	auipc	ra,0x0
    80003170:	9e8080e7          	jalr	-1560(ra) # 80002b54 <consputc>
    80003174:	000c8493          	mv	s1,s9
    80003178:	00094503          	lbu	a0,0(s2)
    8000317c:	c60510e3          	bnez	a0,80002ddc <__printf+0xa4>
    80003180:	e40c0ee3          	beqz	s8,80002fdc <__printf+0x2a4>
    80003184:	00004517          	auipc	a0,0x4
    80003188:	ccc50513          	addi	a0,a0,-820 # 80006e50 <pr>
    8000318c:	00001097          	auipc	ra,0x1
    80003190:	94c080e7          	jalr	-1716(ra) # 80003ad8 <release>
    80003194:	e49ff06f          	j	80002fdc <__printf+0x2a4>
    80003198:	f7843783          	ld	a5,-136(s0)
    8000319c:	03000513          	li	a0,48
    800031a0:	01000d13          	li	s10,16
    800031a4:	00878713          	addi	a4,a5,8
    800031a8:	0007bc83          	ld	s9,0(a5)
    800031ac:	f6e43c23          	sd	a4,-136(s0)
    800031b0:	00000097          	auipc	ra,0x0
    800031b4:	9a4080e7          	jalr	-1628(ra) # 80002b54 <consputc>
    800031b8:	07800513          	li	a0,120
    800031bc:	00000097          	auipc	ra,0x0
    800031c0:	998080e7          	jalr	-1640(ra) # 80002b54 <consputc>
    800031c4:	00002d97          	auipc	s11,0x2
    800031c8:	474d8d93          	addi	s11,s11,1140 # 80005638 <digits>
    800031cc:	03ccd793          	srli	a5,s9,0x3c
    800031d0:	00fd87b3          	add	a5,s11,a5
    800031d4:	0007c503          	lbu	a0,0(a5)
    800031d8:	fffd0d1b          	addiw	s10,s10,-1
    800031dc:	004c9c93          	slli	s9,s9,0x4
    800031e0:	00000097          	auipc	ra,0x0
    800031e4:	974080e7          	jalr	-1676(ra) # 80002b54 <consputc>
    800031e8:	fe0d12e3          	bnez	s10,800031cc <__printf+0x494>
    800031ec:	f8dff06f          	j	80003178 <__printf+0x440>
    800031f0:	f7843783          	ld	a5,-136(s0)
    800031f4:	0007bc83          	ld	s9,0(a5)
    800031f8:	00878793          	addi	a5,a5,8
    800031fc:	f6f43c23          	sd	a5,-136(s0)
    80003200:	000c9a63          	bnez	s9,80003214 <__printf+0x4dc>
    80003204:	1080006f          	j	8000330c <__printf+0x5d4>
    80003208:	001c8c93          	addi	s9,s9,1
    8000320c:	00000097          	auipc	ra,0x0
    80003210:	948080e7          	jalr	-1720(ra) # 80002b54 <consputc>
    80003214:	000cc503          	lbu	a0,0(s9)
    80003218:	fe0518e3          	bnez	a0,80003208 <__printf+0x4d0>
    8000321c:	f5dff06f          	j	80003178 <__printf+0x440>
    80003220:	02500513          	li	a0,37
    80003224:	00000097          	auipc	ra,0x0
    80003228:	930080e7          	jalr	-1744(ra) # 80002b54 <consputc>
    8000322c:	000c8513          	mv	a0,s9
    80003230:	00000097          	auipc	ra,0x0
    80003234:	924080e7          	jalr	-1756(ra) # 80002b54 <consputc>
    80003238:	f41ff06f          	j	80003178 <__printf+0x440>
    8000323c:	02500513          	li	a0,37
    80003240:	00000097          	auipc	ra,0x0
    80003244:	914080e7          	jalr	-1772(ra) # 80002b54 <consputc>
    80003248:	f31ff06f          	j	80003178 <__printf+0x440>
    8000324c:	00030513          	mv	a0,t1
    80003250:	00000097          	auipc	ra,0x0
    80003254:	7bc080e7          	jalr	1980(ra) # 80003a0c <acquire>
    80003258:	b4dff06f          	j	80002da4 <__printf+0x6c>
    8000325c:	40c0053b          	negw	a0,a2
    80003260:	00a00713          	li	a4,10
    80003264:	02e576bb          	remuw	a3,a0,a4
    80003268:	00002d97          	auipc	s11,0x2
    8000326c:	3d0d8d93          	addi	s11,s11,976 # 80005638 <digits>
    80003270:	ff700593          	li	a1,-9
    80003274:	02069693          	slli	a3,a3,0x20
    80003278:	0206d693          	srli	a3,a3,0x20
    8000327c:	00dd86b3          	add	a3,s11,a3
    80003280:	0006c683          	lbu	a3,0(a3)
    80003284:	02e557bb          	divuw	a5,a0,a4
    80003288:	f8d40023          	sb	a3,-128(s0)
    8000328c:	10b65e63          	bge	a2,a1,800033a8 <__printf+0x670>
    80003290:	06300593          	li	a1,99
    80003294:	02e7f6bb          	remuw	a3,a5,a4
    80003298:	02069693          	slli	a3,a3,0x20
    8000329c:	0206d693          	srli	a3,a3,0x20
    800032a0:	00dd86b3          	add	a3,s11,a3
    800032a4:	0006c683          	lbu	a3,0(a3)
    800032a8:	02e7d73b          	divuw	a4,a5,a4
    800032ac:	00200793          	li	a5,2
    800032b0:	f8d400a3          	sb	a3,-127(s0)
    800032b4:	bca5ece3          	bltu	a1,a0,80002e8c <__printf+0x154>
    800032b8:	ce5ff06f          	j	80002f9c <__printf+0x264>
    800032bc:	40e007bb          	negw	a5,a4
    800032c0:	00002d97          	auipc	s11,0x2
    800032c4:	378d8d93          	addi	s11,s11,888 # 80005638 <digits>
    800032c8:	00f7f693          	andi	a3,a5,15
    800032cc:	00dd86b3          	add	a3,s11,a3
    800032d0:	0006c583          	lbu	a1,0(a3)
    800032d4:	ff100613          	li	a2,-15
    800032d8:	0047d69b          	srliw	a3,a5,0x4
    800032dc:	f8b40023          	sb	a1,-128(s0)
    800032e0:	0047d59b          	srliw	a1,a5,0x4
    800032e4:	0ac75e63          	bge	a4,a2,800033a0 <__printf+0x668>
    800032e8:	00f6f693          	andi	a3,a3,15
    800032ec:	00dd86b3          	add	a3,s11,a3
    800032f0:	0006c603          	lbu	a2,0(a3)
    800032f4:	00f00693          	li	a3,15
    800032f8:	0087d79b          	srliw	a5,a5,0x8
    800032fc:	f8c400a3          	sb	a2,-127(s0)
    80003300:	d8b6e4e3          	bltu	a3,a1,80003088 <__printf+0x350>
    80003304:	00200793          	li	a5,2
    80003308:	e2dff06f          	j	80003134 <__printf+0x3fc>
    8000330c:	00002c97          	auipc	s9,0x2
    80003310:	30cc8c93          	addi	s9,s9,780 # 80005618 <_ZTVZL9e2e_testsvE3Foo+0x238>
    80003314:	02800513          	li	a0,40
    80003318:	ef1ff06f          	j	80003208 <__printf+0x4d0>
    8000331c:	00700793          	li	a5,7
    80003320:	00600c93          	li	s9,6
    80003324:	e0dff06f          	j	80003130 <__printf+0x3f8>
    80003328:	00700793          	li	a5,7
    8000332c:	00600c93          	li	s9,6
    80003330:	c69ff06f          	j	80002f98 <__printf+0x260>
    80003334:	00300793          	li	a5,3
    80003338:	00200c93          	li	s9,2
    8000333c:	c5dff06f          	j	80002f98 <__printf+0x260>
    80003340:	00300793          	li	a5,3
    80003344:	00200c93          	li	s9,2
    80003348:	de9ff06f          	j	80003130 <__printf+0x3f8>
    8000334c:	00400793          	li	a5,4
    80003350:	00300c93          	li	s9,3
    80003354:	dddff06f          	j	80003130 <__printf+0x3f8>
    80003358:	00400793          	li	a5,4
    8000335c:	00300c93          	li	s9,3
    80003360:	c39ff06f          	j	80002f98 <__printf+0x260>
    80003364:	00500793          	li	a5,5
    80003368:	00400c93          	li	s9,4
    8000336c:	c2dff06f          	j	80002f98 <__printf+0x260>
    80003370:	00500793          	li	a5,5
    80003374:	00400c93          	li	s9,4
    80003378:	db9ff06f          	j	80003130 <__printf+0x3f8>
    8000337c:	00600793          	li	a5,6
    80003380:	00500c93          	li	s9,5
    80003384:	dadff06f          	j	80003130 <__printf+0x3f8>
    80003388:	00600793          	li	a5,6
    8000338c:	00500c93          	li	s9,5
    80003390:	c09ff06f          	j	80002f98 <__printf+0x260>
    80003394:	00800793          	li	a5,8
    80003398:	00700c93          	li	s9,7
    8000339c:	bfdff06f          	j	80002f98 <__printf+0x260>
    800033a0:	00100793          	li	a5,1
    800033a4:	d91ff06f          	j	80003134 <__printf+0x3fc>
    800033a8:	00100793          	li	a5,1
    800033ac:	bf1ff06f          	j	80002f9c <__printf+0x264>
    800033b0:	00900793          	li	a5,9
    800033b4:	00800c93          	li	s9,8
    800033b8:	be1ff06f          	j	80002f98 <__printf+0x260>
    800033bc:	00002517          	auipc	a0,0x2
    800033c0:	26450513          	addi	a0,a0,612 # 80005620 <_ZTVZL9e2e_testsvE3Foo+0x240>
    800033c4:	00000097          	auipc	ra,0x0
    800033c8:	918080e7          	jalr	-1768(ra) # 80002cdc <panic>

00000000800033cc <printfinit>:
    800033cc:	fe010113          	addi	sp,sp,-32
    800033d0:	00813823          	sd	s0,16(sp)
    800033d4:	00913423          	sd	s1,8(sp)
    800033d8:	00113c23          	sd	ra,24(sp)
    800033dc:	02010413          	addi	s0,sp,32
    800033e0:	00004497          	auipc	s1,0x4
    800033e4:	a7048493          	addi	s1,s1,-1424 # 80006e50 <pr>
    800033e8:	00048513          	mv	a0,s1
    800033ec:	00002597          	auipc	a1,0x2
    800033f0:	24458593          	addi	a1,a1,580 # 80005630 <_ZTVZL9e2e_testsvE3Foo+0x250>
    800033f4:	00000097          	auipc	ra,0x0
    800033f8:	5f4080e7          	jalr	1524(ra) # 800039e8 <initlock>
    800033fc:	01813083          	ld	ra,24(sp)
    80003400:	01013403          	ld	s0,16(sp)
    80003404:	0004ac23          	sw	zero,24(s1)
    80003408:	00813483          	ld	s1,8(sp)
    8000340c:	02010113          	addi	sp,sp,32
    80003410:	00008067          	ret

0000000080003414 <uartinit>:
    80003414:	ff010113          	addi	sp,sp,-16
    80003418:	00813423          	sd	s0,8(sp)
    8000341c:	01010413          	addi	s0,sp,16
    80003420:	100007b7          	lui	a5,0x10000
    80003424:	000780a3          	sb	zero,1(a5) # 10000001 <_entry-0x6fffffff>
    80003428:	f8000713          	li	a4,-128
    8000342c:	00e781a3          	sb	a4,3(a5)
    80003430:	00300713          	li	a4,3
    80003434:	00e78023          	sb	a4,0(a5)
    80003438:	000780a3          	sb	zero,1(a5)
    8000343c:	00e781a3          	sb	a4,3(a5)
    80003440:	00700693          	li	a3,7
    80003444:	00d78123          	sb	a3,2(a5)
    80003448:	00e780a3          	sb	a4,1(a5)
    8000344c:	00813403          	ld	s0,8(sp)
    80003450:	01010113          	addi	sp,sp,16
    80003454:	00008067          	ret

0000000080003458 <uartputc>:
    80003458:	00002797          	auipc	a5,0x2
    8000345c:	7b07a783          	lw	a5,1968(a5) # 80005c08 <panicked>
    80003460:	00078463          	beqz	a5,80003468 <uartputc+0x10>
    80003464:	0000006f          	j	80003464 <uartputc+0xc>
    80003468:	fd010113          	addi	sp,sp,-48
    8000346c:	02813023          	sd	s0,32(sp)
    80003470:	00913c23          	sd	s1,24(sp)
    80003474:	01213823          	sd	s2,16(sp)
    80003478:	01313423          	sd	s3,8(sp)
    8000347c:	02113423          	sd	ra,40(sp)
    80003480:	03010413          	addi	s0,sp,48
    80003484:	00002917          	auipc	s2,0x2
    80003488:	78c90913          	addi	s2,s2,1932 # 80005c10 <uart_tx_r>
    8000348c:	00093783          	ld	a5,0(s2)
    80003490:	00002497          	auipc	s1,0x2
    80003494:	78848493          	addi	s1,s1,1928 # 80005c18 <uart_tx_w>
    80003498:	0004b703          	ld	a4,0(s1)
    8000349c:	02078693          	addi	a3,a5,32
    800034a0:	00050993          	mv	s3,a0
    800034a4:	02e69c63          	bne	a3,a4,800034dc <uartputc+0x84>
    800034a8:	00001097          	auipc	ra,0x1
    800034ac:	834080e7          	jalr	-1996(ra) # 80003cdc <push_on>
    800034b0:	00093783          	ld	a5,0(s2)
    800034b4:	0004b703          	ld	a4,0(s1)
    800034b8:	02078793          	addi	a5,a5,32
    800034bc:	00e79463          	bne	a5,a4,800034c4 <uartputc+0x6c>
    800034c0:	0000006f          	j	800034c0 <uartputc+0x68>
    800034c4:	00001097          	auipc	ra,0x1
    800034c8:	88c080e7          	jalr	-1908(ra) # 80003d50 <pop_on>
    800034cc:	00093783          	ld	a5,0(s2)
    800034d0:	0004b703          	ld	a4,0(s1)
    800034d4:	02078693          	addi	a3,a5,32
    800034d8:	fce688e3          	beq	a3,a4,800034a8 <uartputc+0x50>
    800034dc:	01f77693          	andi	a3,a4,31
    800034e0:	00004597          	auipc	a1,0x4
    800034e4:	99058593          	addi	a1,a1,-1648 # 80006e70 <uart_tx_buf>
    800034e8:	00d586b3          	add	a3,a1,a3
    800034ec:	00170713          	addi	a4,a4,1
    800034f0:	01368023          	sb	s3,0(a3)
    800034f4:	00e4b023          	sd	a4,0(s1)
    800034f8:	10000637          	lui	a2,0x10000
    800034fc:	02f71063          	bne	a4,a5,8000351c <uartputc+0xc4>
    80003500:	0340006f          	j	80003534 <uartputc+0xdc>
    80003504:	00074703          	lbu	a4,0(a4)
    80003508:	00f93023          	sd	a5,0(s2)
    8000350c:	00e60023          	sb	a4,0(a2) # 10000000 <_entry-0x70000000>
    80003510:	00093783          	ld	a5,0(s2)
    80003514:	0004b703          	ld	a4,0(s1)
    80003518:	00f70e63          	beq	a4,a5,80003534 <uartputc+0xdc>
    8000351c:	00564683          	lbu	a3,5(a2)
    80003520:	01f7f713          	andi	a4,a5,31
    80003524:	00e58733          	add	a4,a1,a4
    80003528:	0206f693          	andi	a3,a3,32
    8000352c:	00178793          	addi	a5,a5,1
    80003530:	fc069ae3          	bnez	a3,80003504 <uartputc+0xac>
    80003534:	02813083          	ld	ra,40(sp)
    80003538:	02013403          	ld	s0,32(sp)
    8000353c:	01813483          	ld	s1,24(sp)
    80003540:	01013903          	ld	s2,16(sp)
    80003544:	00813983          	ld	s3,8(sp)
    80003548:	03010113          	addi	sp,sp,48
    8000354c:	00008067          	ret

0000000080003550 <uartputc_sync>:
    80003550:	ff010113          	addi	sp,sp,-16
    80003554:	00813423          	sd	s0,8(sp)
    80003558:	01010413          	addi	s0,sp,16
    8000355c:	00002717          	auipc	a4,0x2
    80003560:	6ac72703          	lw	a4,1708(a4) # 80005c08 <panicked>
    80003564:	02071663          	bnez	a4,80003590 <uartputc_sync+0x40>
    80003568:	00050793          	mv	a5,a0
    8000356c:	100006b7          	lui	a3,0x10000
    80003570:	0056c703          	lbu	a4,5(a3) # 10000005 <_entry-0x6ffffffb>
    80003574:	02077713          	andi	a4,a4,32
    80003578:	fe070ce3          	beqz	a4,80003570 <uartputc_sync+0x20>
    8000357c:	0ff7f793          	andi	a5,a5,255
    80003580:	00f68023          	sb	a5,0(a3)
    80003584:	00813403          	ld	s0,8(sp)
    80003588:	01010113          	addi	sp,sp,16
    8000358c:	00008067          	ret
    80003590:	0000006f          	j	80003590 <uartputc_sync+0x40>

0000000080003594 <uartstart>:
    80003594:	ff010113          	addi	sp,sp,-16
    80003598:	00813423          	sd	s0,8(sp)
    8000359c:	01010413          	addi	s0,sp,16
    800035a0:	00002617          	auipc	a2,0x2
    800035a4:	67060613          	addi	a2,a2,1648 # 80005c10 <uart_tx_r>
    800035a8:	00002517          	auipc	a0,0x2
    800035ac:	67050513          	addi	a0,a0,1648 # 80005c18 <uart_tx_w>
    800035b0:	00063783          	ld	a5,0(a2)
    800035b4:	00053703          	ld	a4,0(a0)
    800035b8:	04f70263          	beq	a4,a5,800035fc <uartstart+0x68>
    800035bc:	100005b7          	lui	a1,0x10000
    800035c0:	00004817          	auipc	a6,0x4
    800035c4:	8b080813          	addi	a6,a6,-1872 # 80006e70 <uart_tx_buf>
    800035c8:	01c0006f          	j	800035e4 <uartstart+0x50>
    800035cc:	0006c703          	lbu	a4,0(a3)
    800035d0:	00f63023          	sd	a5,0(a2)
    800035d4:	00e58023          	sb	a4,0(a1) # 10000000 <_entry-0x70000000>
    800035d8:	00063783          	ld	a5,0(a2)
    800035dc:	00053703          	ld	a4,0(a0)
    800035e0:	00f70e63          	beq	a4,a5,800035fc <uartstart+0x68>
    800035e4:	01f7f713          	andi	a4,a5,31
    800035e8:	00e806b3          	add	a3,a6,a4
    800035ec:	0055c703          	lbu	a4,5(a1)
    800035f0:	00178793          	addi	a5,a5,1
    800035f4:	02077713          	andi	a4,a4,32
    800035f8:	fc071ae3          	bnez	a4,800035cc <uartstart+0x38>
    800035fc:	00813403          	ld	s0,8(sp)
    80003600:	01010113          	addi	sp,sp,16
    80003604:	00008067          	ret

0000000080003608 <uartgetc>:
    80003608:	ff010113          	addi	sp,sp,-16
    8000360c:	00813423          	sd	s0,8(sp)
    80003610:	01010413          	addi	s0,sp,16
    80003614:	10000737          	lui	a4,0x10000
    80003618:	00574783          	lbu	a5,5(a4) # 10000005 <_entry-0x6ffffffb>
    8000361c:	0017f793          	andi	a5,a5,1
    80003620:	00078c63          	beqz	a5,80003638 <uartgetc+0x30>
    80003624:	00074503          	lbu	a0,0(a4)
    80003628:	0ff57513          	andi	a0,a0,255
    8000362c:	00813403          	ld	s0,8(sp)
    80003630:	01010113          	addi	sp,sp,16
    80003634:	00008067          	ret
    80003638:	fff00513          	li	a0,-1
    8000363c:	ff1ff06f          	j	8000362c <uartgetc+0x24>

0000000080003640 <uartintr>:
    80003640:	100007b7          	lui	a5,0x10000
    80003644:	0057c783          	lbu	a5,5(a5) # 10000005 <_entry-0x6ffffffb>
    80003648:	0017f793          	andi	a5,a5,1
    8000364c:	0a078463          	beqz	a5,800036f4 <uartintr+0xb4>
    80003650:	fe010113          	addi	sp,sp,-32
    80003654:	00813823          	sd	s0,16(sp)
    80003658:	00913423          	sd	s1,8(sp)
    8000365c:	00113c23          	sd	ra,24(sp)
    80003660:	02010413          	addi	s0,sp,32
    80003664:	100004b7          	lui	s1,0x10000
    80003668:	0004c503          	lbu	a0,0(s1) # 10000000 <_entry-0x70000000>
    8000366c:	0ff57513          	andi	a0,a0,255
    80003670:	fffff097          	auipc	ra,0xfffff
    80003674:	534080e7          	jalr	1332(ra) # 80002ba4 <consoleintr>
    80003678:	0054c783          	lbu	a5,5(s1)
    8000367c:	0017f793          	andi	a5,a5,1
    80003680:	fe0794e3          	bnez	a5,80003668 <uartintr+0x28>
    80003684:	00002617          	auipc	a2,0x2
    80003688:	58c60613          	addi	a2,a2,1420 # 80005c10 <uart_tx_r>
    8000368c:	00002517          	auipc	a0,0x2
    80003690:	58c50513          	addi	a0,a0,1420 # 80005c18 <uart_tx_w>
    80003694:	00063783          	ld	a5,0(a2)
    80003698:	00053703          	ld	a4,0(a0)
    8000369c:	04f70263          	beq	a4,a5,800036e0 <uartintr+0xa0>
    800036a0:	100005b7          	lui	a1,0x10000
    800036a4:	00003817          	auipc	a6,0x3
    800036a8:	7cc80813          	addi	a6,a6,1996 # 80006e70 <uart_tx_buf>
    800036ac:	01c0006f          	j	800036c8 <uartintr+0x88>
    800036b0:	0006c703          	lbu	a4,0(a3)
    800036b4:	00f63023          	sd	a5,0(a2)
    800036b8:	00e58023          	sb	a4,0(a1) # 10000000 <_entry-0x70000000>
    800036bc:	00063783          	ld	a5,0(a2)
    800036c0:	00053703          	ld	a4,0(a0)
    800036c4:	00f70e63          	beq	a4,a5,800036e0 <uartintr+0xa0>
    800036c8:	01f7f713          	andi	a4,a5,31
    800036cc:	00e806b3          	add	a3,a6,a4
    800036d0:	0055c703          	lbu	a4,5(a1)
    800036d4:	00178793          	addi	a5,a5,1
    800036d8:	02077713          	andi	a4,a4,32
    800036dc:	fc071ae3          	bnez	a4,800036b0 <uartintr+0x70>
    800036e0:	01813083          	ld	ra,24(sp)
    800036e4:	01013403          	ld	s0,16(sp)
    800036e8:	00813483          	ld	s1,8(sp)
    800036ec:	02010113          	addi	sp,sp,32
    800036f0:	00008067          	ret
    800036f4:	00002617          	auipc	a2,0x2
    800036f8:	51c60613          	addi	a2,a2,1308 # 80005c10 <uart_tx_r>
    800036fc:	00002517          	auipc	a0,0x2
    80003700:	51c50513          	addi	a0,a0,1308 # 80005c18 <uart_tx_w>
    80003704:	00063783          	ld	a5,0(a2)
    80003708:	00053703          	ld	a4,0(a0)
    8000370c:	04f70263          	beq	a4,a5,80003750 <uartintr+0x110>
    80003710:	100005b7          	lui	a1,0x10000
    80003714:	00003817          	auipc	a6,0x3
    80003718:	75c80813          	addi	a6,a6,1884 # 80006e70 <uart_tx_buf>
    8000371c:	01c0006f          	j	80003738 <uartintr+0xf8>
    80003720:	0006c703          	lbu	a4,0(a3)
    80003724:	00f63023          	sd	a5,0(a2)
    80003728:	00e58023          	sb	a4,0(a1) # 10000000 <_entry-0x70000000>
    8000372c:	00063783          	ld	a5,0(a2)
    80003730:	00053703          	ld	a4,0(a0)
    80003734:	02f70063          	beq	a4,a5,80003754 <uartintr+0x114>
    80003738:	01f7f713          	andi	a4,a5,31
    8000373c:	00e806b3          	add	a3,a6,a4
    80003740:	0055c703          	lbu	a4,5(a1)
    80003744:	00178793          	addi	a5,a5,1
    80003748:	02077713          	andi	a4,a4,32
    8000374c:	fc071ae3          	bnez	a4,80003720 <uartintr+0xe0>
    80003750:	00008067          	ret
    80003754:	00008067          	ret

0000000080003758 <kinit>:
    80003758:	fc010113          	addi	sp,sp,-64
    8000375c:	02913423          	sd	s1,40(sp)
    80003760:	fffff7b7          	lui	a5,0xfffff
    80003764:	00004497          	auipc	s1,0x4
    80003768:	72b48493          	addi	s1,s1,1835 # 80007e8f <end+0xfff>
    8000376c:	02813823          	sd	s0,48(sp)
    80003770:	01313c23          	sd	s3,24(sp)
    80003774:	00f4f4b3          	and	s1,s1,a5
    80003778:	02113c23          	sd	ra,56(sp)
    8000377c:	03213023          	sd	s2,32(sp)
    80003780:	01413823          	sd	s4,16(sp)
    80003784:	01513423          	sd	s5,8(sp)
    80003788:	04010413          	addi	s0,sp,64
    8000378c:	000017b7          	lui	a5,0x1
    80003790:	01100993          	li	s3,17
    80003794:	00f487b3          	add	a5,s1,a5
    80003798:	01b99993          	slli	s3,s3,0x1b
    8000379c:	06f9e063          	bltu	s3,a5,800037fc <kinit+0xa4>
    800037a0:	00003a97          	auipc	s5,0x3
    800037a4:	6f0a8a93          	addi	s5,s5,1776 # 80006e90 <end>
    800037a8:	0754ec63          	bltu	s1,s5,80003820 <kinit+0xc8>
    800037ac:	0734fa63          	bgeu	s1,s3,80003820 <kinit+0xc8>
    800037b0:	00088a37          	lui	s4,0x88
    800037b4:	fffa0a13          	addi	s4,s4,-1 # 87fff <_entry-0x7ff78001>
    800037b8:	00002917          	auipc	s2,0x2
    800037bc:	46890913          	addi	s2,s2,1128 # 80005c20 <kmem>
    800037c0:	00ca1a13          	slli	s4,s4,0xc
    800037c4:	0140006f          	j	800037d8 <kinit+0x80>
    800037c8:	000017b7          	lui	a5,0x1
    800037cc:	00f484b3          	add	s1,s1,a5
    800037d0:	0554e863          	bltu	s1,s5,80003820 <kinit+0xc8>
    800037d4:	0534f663          	bgeu	s1,s3,80003820 <kinit+0xc8>
    800037d8:	00001637          	lui	a2,0x1
    800037dc:	00100593          	li	a1,1
    800037e0:	00048513          	mv	a0,s1
    800037e4:	00000097          	auipc	ra,0x0
    800037e8:	5e4080e7          	jalr	1508(ra) # 80003dc8 <__memset>
    800037ec:	00093783          	ld	a5,0(s2)
    800037f0:	00f4b023          	sd	a5,0(s1)
    800037f4:	00993023          	sd	s1,0(s2)
    800037f8:	fd4498e3          	bne	s1,s4,800037c8 <kinit+0x70>
    800037fc:	03813083          	ld	ra,56(sp)
    80003800:	03013403          	ld	s0,48(sp)
    80003804:	02813483          	ld	s1,40(sp)
    80003808:	02013903          	ld	s2,32(sp)
    8000380c:	01813983          	ld	s3,24(sp)
    80003810:	01013a03          	ld	s4,16(sp)
    80003814:	00813a83          	ld	s5,8(sp)
    80003818:	04010113          	addi	sp,sp,64
    8000381c:	00008067          	ret
    80003820:	00002517          	auipc	a0,0x2
    80003824:	e3050513          	addi	a0,a0,-464 # 80005650 <digits+0x18>
    80003828:	fffff097          	auipc	ra,0xfffff
    8000382c:	4b4080e7          	jalr	1204(ra) # 80002cdc <panic>

0000000080003830 <freerange>:
    80003830:	fc010113          	addi	sp,sp,-64
    80003834:	000017b7          	lui	a5,0x1
    80003838:	02913423          	sd	s1,40(sp)
    8000383c:	fff78493          	addi	s1,a5,-1 # fff <_entry-0x7ffff001>
    80003840:	009504b3          	add	s1,a0,s1
    80003844:	fffff537          	lui	a0,0xfffff
    80003848:	02813823          	sd	s0,48(sp)
    8000384c:	02113c23          	sd	ra,56(sp)
    80003850:	03213023          	sd	s2,32(sp)
    80003854:	01313c23          	sd	s3,24(sp)
    80003858:	01413823          	sd	s4,16(sp)
    8000385c:	01513423          	sd	s5,8(sp)
    80003860:	01613023          	sd	s6,0(sp)
    80003864:	04010413          	addi	s0,sp,64
    80003868:	00a4f4b3          	and	s1,s1,a0
    8000386c:	00f487b3          	add	a5,s1,a5
    80003870:	06f5e463          	bltu	a1,a5,800038d8 <freerange+0xa8>
    80003874:	00003a97          	auipc	s5,0x3
    80003878:	61ca8a93          	addi	s5,s5,1564 # 80006e90 <end>
    8000387c:	0954e263          	bltu	s1,s5,80003900 <freerange+0xd0>
    80003880:	01100993          	li	s3,17
    80003884:	01b99993          	slli	s3,s3,0x1b
    80003888:	0734fc63          	bgeu	s1,s3,80003900 <freerange+0xd0>
    8000388c:	00058a13          	mv	s4,a1
    80003890:	00002917          	auipc	s2,0x2
    80003894:	39090913          	addi	s2,s2,912 # 80005c20 <kmem>
    80003898:	00002b37          	lui	s6,0x2
    8000389c:	0140006f          	j	800038b0 <freerange+0x80>
    800038a0:	000017b7          	lui	a5,0x1
    800038a4:	00f484b3          	add	s1,s1,a5
    800038a8:	0554ec63          	bltu	s1,s5,80003900 <freerange+0xd0>
    800038ac:	0534fa63          	bgeu	s1,s3,80003900 <freerange+0xd0>
    800038b0:	00001637          	lui	a2,0x1
    800038b4:	00100593          	li	a1,1
    800038b8:	00048513          	mv	a0,s1
    800038bc:	00000097          	auipc	ra,0x0
    800038c0:	50c080e7          	jalr	1292(ra) # 80003dc8 <__memset>
    800038c4:	00093703          	ld	a4,0(s2)
    800038c8:	016487b3          	add	a5,s1,s6
    800038cc:	00e4b023          	sd	a4,0(s1)
    800038d0:	00993023          	sd	s1,0(s2)
    800038d4:	fcfa76e3          	bgeu	s4,a5,800038a0 <freerange+0x70>
    800038d8:	03813083          	ld	ra,56(sp)
    800038dc:	03013403          	ld	s0,48(sp)
    800038e0:	02813483          	ld	s1,40(sp)
    800038e4:	02013903          	ld	s2,32(sp)
    800038e8:	01813983          	ld	s3,24(sp)
    800038ec:	01013a03          	ld	s4,16(sp)
    800038f0:	00813a83          	ld	s5,8(sp)
    800038f4:	00013b03          	ld	s6,0(sp)
    800038f8:	04010113          	addi	sp,sp,64
    800038fc:	00008067          	ret
    80003900:	00002517          	auipc	a0,0x2
    80003904:	d5050513          	addi	a0,a0,-688 # 80005650 <digits+0x18>
    80003908:	fffff097          	auipc	ra,0xfffff
    8000390c:	3d4080e7          	jalr	980(ra) # 80002cdc <panic>

0000000080003910 <kfree>:
    80003910:	fe010113          	addi	sp,sp,-32
    80003914:	00813823          	sd	s0,16(sp)
    80003918:	00113c23          	sd	ra,24(sp)
    8000391c:	00913423          	sd	s1,8(sp)
    80003920:	02010413          	addi	s0,sp,32
    80003924:	03451793          	slli	a5,a0,0x34
    80003928:	04079c63          	bnez	a5,80003980 <kfree+0x70>
    8000392c:	00003797          	auipc	a5,0x3
    80003930:	56478793          	addi	a5,a5,1380 # 80006e90 <end>
    80003934:	00050493          	mv	s1,a0
    80003938:	04f56463          	bltu	a0,a5,80003980 <kfree+0x70>
    8000393c:	01100793          	li	a5,17
    80003940:	01b79793          	slli	a5,a5,0x1b
    80003944:	02f57e63          	bgeu	a0,a5,80003980 <kfree+0x70>
    80003948:	00001637          	lui	a2,0x1
    8000394c:	00100593          	li	a1,1
    80003950:	00000097          	auipc	ra,0x0
    80003954:	478080e7          	jalr	1144(ra) # 80003dc8 <__memset>
    80003958:	00002797          	auipc	a5,0x2
    8000395c:	2c878793          	addi	a5,a5,712 # 80005c20 <kmem>
    80003960:	0007b703          	ld	a4,0(a5)
    80003964:	01813083          	ld	ra,24(sp)
    80003968:	01013403          	ld	s0,16(sp)
    8000396c:	00e4b023          	sd	a4,0(s1)
    80003970:	0097b023          	sd	s1,0(a5)
    80003974:	00813483          	ld	s1,8(sp)
    80003978:	02010113          	addi	sp,sp,32
    8000397c:	00008067          	ret
    80003980:	00002517          	auipc	a0,0x2
    80003984:	cd050513          	addi	a0,a0,-816 # 80005650 <digits+0x18>
    80003988:	fffff097          	auipc	ra,0xfffff
    8000398c:	354080e7          	jalr	852(ra) # 80002cdc <panic>

0000000080003990 <kalloc>:
    80003990:	fe010113          	addi	sp,sp,-32
    80003994:	00813823          	sd	s0,16(sp)
    80003998:	00913423          	sd	s1,8(sp)
    8000399c:	00113c23          	sd	ra,24(sp)
    800039a0:	02010413          	addi	s0,sp,32
    800039a4:	00002797          	auipc	a5,0x2
    800039a8:	27c78793          	addi	a5,a5,636 # 80005c20 <kmem>
    800039ac:	0007b483          	ld	s1,0(a5)
    800039b0:	02048063          	beqz	s1,800039d0 <kalloc+0x40>
    800039b4:	0004b703          	ld	a4,0(s1)
    800039b8:	00001637          	lui	a2,0x1
    800039bc:	00500593          	li	a1,5
    800039c0:	00048513          	mv	a0,s1
    800039c4:	00e7b023          	sd	a4,0(a5)
    800039c8:	00000097          	auipc	ra,0x0
    800039cc:	400080e7          	jalr	1024(ra) # 80003dc8 <__memset>
    800039d0:	01813083          	ld	ra,24(sp)
    800039d4:	01013403          	ld	s0,16(sp)
    800039d8:	00048513          	mv	a0,s1
    800039dc:	00813483          	ld	s1,8(sp)
    800039e0:	02010113          	addi	sp,sp,32
    800039e4:	00008067          	ret

00000000800039e8 <initlock>:
    800039e8:	ff010113          	addi	sp,sp,-16
    800039ec:	00813423          	sd	s0,8(sp)
    800039f0:	01010413          	addi	s0,sp,16
    800039f4:	00813403          	ld	s0,8(sp)
    800039f8:	00b53423          	sd	a1,8(a0)
    800039fc:	00052023          	sw	zero,0(a0)
    80003a00:	00053823          	sd	zero,16(a0)
    80003a04:	01010113          	addi	sp,sp,16
    80003a08:	00008067          	ret

0000000080003a0c <acquire>:
    80003a0c:	fe010113          	addi	sp,sp,-32
    80003a10:	00813823          	sd	s0,16(sp)
    80003a14:	00913423          	sd	s1,8(sp)
    80003a18:	00113c23          	sd	ra,24(sp)
    80003a1c:	01213023          	sd	s2,0(sp)
    80003a20:	02010413          	addi	s0,sp,32
    80003a24:	00050493          	mv	s1,a0
    80003a28:	10002973          	csrr	s2,sstatus
    80003a2c:	100027f3          	csrr	a5,sstatus
    80003a30:	ffd7f793          	andi	a5,a5,-3
    80003a34:	10079073          	csrw	sstatus,a5
    80003a38:	fffff097          	auipc	ra,0xfffff
    80003a3c:	8e0080e7          	jalr	-1824(ra) # 80002318 <mycpu>
    80003a40:	07852783          	lw	a5,120(a0)
    80003a44:	06078e63          	beqz	a5,80003ac0 <acquire+0xb4>
    80003a48:	fffff097          	auipc	ra,0xfffff
    80003a4c:	8d0080e7          	jalr	-1840(ra) # 80002318 <mycpu>
    80003a50:	07852783          	lw	a5,120(a0)
    80003a54:	0004a703          	lw	a4,0(s1)
    80003a58:	0017879b          	addiw	a5,a5,1
    80003a5c:	06f52c23          	sw	a5,120(a0)
    80003a60:	04071063          	bnez	a4,80003aa0 <acquire+0x94>
    80003a64:	00100713          	li	a4,1
    80003a68:	00070793          	mv	a5,a4
    80003a6c:	0cf4a7af          	amoswap.w.aq	a5,a5,(s1)
    80003a70:	0007879b          	sext.w	a5,a5
    80003a74:	fe079ae3          	bnez	a5,80003a68 <acquire+0x5c>
    80003a78:	0ff0000f          	fence
    80003a7c:	fffff097          	auipc	ra,0xfffff
    80003a80:	89c080e7          	jalr	-1892(ra) # 80002318 <mycpu>
    80003a84:	01813083          	ld	ra,24(sp)
    80003a88:	01013403          	ld	s0,16(sp)
    80003a8c:	00a4b823          	sd	a0,16(s1)
    80003a90:	00013903          	ld	s2,0(sp)
    80003a94:	00813483          	ld	s1,8(sp)
    80003a98:	02010113          	addi	sp,sp,32
    80003a9c:	00008067          	ret
    80003aa0:	0104b903          	ld	s2,16(s1)
    80003aa4:	fffff097          	auipc	ra,0xfffff
    80003aa8:	874080e7          	jalr	-1932(ra) # 80002318 <mycpu>
    80003aac:	faa91ce3          	bne	s2,a0,80003a64 <acquire+0x58>
    80003ab0:	00002517          	auipc	a0,0x2
    80003ab4:	ba850513          	addi	a0,a0,-1112 # 80005658 <digits+0x20>
    80003ab8:	fffff097          	auipc	ra,0xfffff
    80003abc:	224080e7          	jalr	548(ra) # 80002cdc <panic>
    80003ac0:	00195913          	srli	s2,s2,0x1
    80003ac4:	fffff097          	auipc	ra,0xfffff
    80003ac8:	854080e7          	jalr	-1964(ra) # 80002318 <mycpu>
    80003acc:	00197913          	andi	s2,s2,1
    80003ad0:	07252e23          	sw	s2,124(a0)
    80003ad4:	f75ff06f          	j	80003a48 <acquire+0x3c>

0000000080003ad8 <release>:
    80003ad8:	fe010113          	addi	sp,sp,-32
    80003adc:	00813823          	sd	s0,16(sp)
    80003ae0:	00113c23          	sd	ra,24(sp)
    80003ae4:	00913423          	sd	s1,8(sp)
    80003ae8:	01213023          	sd	s2,0(sp)
    80003aec:	02010413          	addi	s0,sp,32
    80003af0:	00052783          	lw	a5,0(a0)
    80003af4:	00079a63          	bnez	a5,80003b08 <release+0x30>
    80003af8:	00002517          	auipc	a0,0x2
    80003afc:	b6850513          	addi	a0,a0,-1176 # 80005660 <digits+0x28>
    80003b00:	fffff097          	auipc	ra,0xfffff
    80003b04:	1dc080e7          	jalr	476(ra) # 80002cdc <panic>
    80003b08:	01053903          	ld	s2,16(a0)
    80003b0c:	00050493          	mv	s1,a0
    80003b10:	fffff097          	auipc	ra,0xfffff
    80003b14:	808080e7          	jalr	-2040(ra) # 80002318 <mycpu>
    80003b18:	fea910e3          	bne	s2,a0,80003af8 <release+0x20>
    80003b1c:	0004b823          	sd	zero,16(s1)
    80003b20:	0ff0000f          	fence
    80003b24:	0f50000f          	fence	iorw,ow
    80003b28:	0804a02f          	amoswap.w	zero,zero,(s1)
    80003b2c:	ffffe097          	auipc	ra,0xffffe
    80003b30:	7ec080e7          	jalr	2028(ra) # 80002318 <mycpu>
    80003b34:	100027f3          	csrr	a5,sstatus
    80003b38:	0027f793          	andi	a5,a5,2
    80003b3c:	04079a63          	bnez	a5,80003b90 <release+0xb8>
    80003b40:	07852783          	lw	a5,120(a0)
    80003b44:	02f05e63          	blez	a5,80003b80 <release+0xa8>
    80003b48:	fff7871b          	addiw	a4,a5,-1
    80003b4c:	06e52c23          	sw	a4,120(a0)
    80003b50:	00071c63          	bnez	a4,80003b68 <release+0x90>
    80003b54:	07c52783          	lw	a5,124(a0)
    80003b58:	00078863          	beqz	a5,80003b68 <release+0x90>
    80003b5c:	100027f3          	csrr	a5,sstatus
    80003b60:	0027e793          	ori	a5,a5,2
    80003b64:	10079073          	csrw	sstatus,a5
    80003b68:	01813083          	ld	ra,24(sp)
    80003b6c:	01013403          	ld	s0,16(sp)
    80003b70:	00813483          	ld	s1,8(sp)
    80003b74:	00013903          	ld	s2,0(sp)
    80003b78:	02010113          	addi	sp,sp,32
    80003b7c:	00008067          	ret
    80003b80:	00002517          	auipc	a0,0x2
    80003b84:	b0050513          	addi	a0,a0,-1280 # 80005680 <digits+0x48>
    80003b88:	fffff097          	auipc	ra,0xfffff
    80003b8c:	154080e7          	jalr	340(ra) # 80002cdc <panic>
    80003b90:	00002517          	auipc	a0,0x2
    80003b94:	ad850513          	addi	a0,a0,-1320 # 80005668 <digits+0x30>
    80003b98:	fffff097          	auipc	ra,0xfffff
    80003b9c:	144080e7          	jalr	324(ra) # 80002cdc <panic>

0000000080003ba0 <holding>:
    80003ba0:	00052783          	lw	a5,0(a0)
    80003ba4:	00079663          	bnez	a5,80003bb0 <holding+0x10>
    80003ba8:	00000513          	li	a0,0
    80003bac:	00008067          	ret
    80003bb0:	fe010113          	addi	sp,sp,-32
    80003bb4:	00813823          	sd	s0,16(sp)
    80003bb8:	00913423          	sd	s1,8(sp)
    80003bbc:	00113c23          	sd	ra,24(sp)
    80003bc0:	02010413          	addi	s0,sp,32
    80003bc4:	01053483          	ld	s1,16(a0)
    80003bc8:	ffffe097          	auipc	ra,0xffffe
    80003bcc:	750080e7          	jalr	1872(ra) # 80002318 <mycpu>
    80003bd0:	01813083          	ld	ra,24(sp)
    80003bd4:	01013403          	ld	s0,16(sp)
    80003bd8:	40a48533          	sub	a0,s1,a0
    80003bdc:	00153513          	seqz	a0,a0
    80003be0:	00813483          	ld	s1,8(sp)
    80003be4:	02010113          	addi	sp,sp,32
    80003be8:	00008067          	ret

0000000080003bec <push_off>:
    80003bec:	fe010113          	addi	sp,sp,-32
    80003bf0:	00813823          	sd	s0,16(sp)
    80003bf4:	00113c23          	sd	ra,24(sp)
    80003bf8:	00913423          	sd	s1,8(sp)
    80003bfc:	02010413          	addi	s0,sp,32
    80003c00:	100024f3          	csrr	s1,sstatus
    80003c04:	100027f3          	csrr	a5,sstatus
    80003c08:	ffd7f793          	andi	a5,a5,-3
    80003c0c:	10079073          	csrw	sstatus,a5
    80003c10:	ffffe097          	auipc	ra,0xffffe
    80003c14:	708080e7          	jalr	1800(ra) # 80002318 <mycpu>
    80003c18:	07852783          	lw	a5,120(a0)
    80003c1c:	02078663          	beqz	a5,80003c48 <push_off+0x5c>
    80003c20:	ffffe097          	auipc	ra,0xffffe
    80003c24:	6f8080e7          	jalr	1784(ra) # 80002318 <mycpu>
    80003c28:	07852783          	lw	a5,120(a0)
    80003c2c:	01813083          	ld	ra,24(sp)
    80003c30:	01013403          	ld	s0,16(sp)
    80003c34:	0017879b          	addiw	a5,a5,1
    80003c38:	06f52c23          	sw	a5,120(a0)
    80003c3c:	00813483          	ld	s1,8(sp)
    80003c40:	02010113          	addi	sp,sp,32
    80003c44:	00008067          	ret
    80003c48:	0014d493          	srli	s1,s1,0x1
    80003c4c:	ffffe097          	auipc	ra,0xffffe
    80003c50:	6cc080e7          	jalr	1740(ra) # 80002318 <mycpu>
    80003c54:	0014f493          	andi	s1,s1,1
    80003c58:	06952e23          	sw	s1,124(a0)
    80003c5c:	fc5ff06f          	j	80003c20 <push_off+0x34>

0000000080003c60 <pop_off>:
    80003c60:	ff010113          	addi	sp,sp,-16
    80003c64:	00813023          	sd	s0,0(sp)
    80003c68:	00113423          	sd	ra,8(sp)
    80003c6c:	01010413          	addi	s0,sp,16
    80003c70:	ffffe097          	auipc	ra,0xffffe
    80003c74:	6a8080e7          	jalr	1704(ra) # 80002318 <mycpu>
    80003c78:	100027f3          	csrr	a5,sstatus
    80003c7c:	0027f793          	andi	a5,a5,2
    80003c80:	04079663          	bnez	a5,80003ccc <pop_off+0x6c>
    80003c84:	07852783          	lw	a5,120(a0)
    80003c88:	02f05a63          	blez	a5,80003cbc <pop_off+0x5c>
    80003c8c:	fff7871b          	addiw	a4,a5,-1
    80003c90:	06e52c23          	sw	a4,120(a0)
    80003c94:	00071c63          	bnez	a4,80003cac <pop_off+0x4c>
    80003c98:	07c52783          	lw	a5,124(a0)
    80003c9c:	00078863          	beqz	a5,80003cac <pop_off+0x4c>
    80003ca0:	100027f3          	csrr	a5,sstatus
    80003ca4:	0027e793          	ori	a5,a5,2
    80003ca8:	10079073          	csrw	sstatus,a5
    80003cac:	00813083          	ld	ra,8(sp)
    80003cb0:	00013403          	ld	s0,0(sp)
    80003cb4:	01010113          	addi	sp,sp,16
    80003cb8:	00008067          	ret
    80003cbc:	00002517          	auipc	a0,0x2
    80003cc0:	9c450513          	addi	a0,a0,-1596 # 80005680 <digits+0x48>
    80003cc4:	fffff097          	auipc	ra,0xfffff
    80003cc8:	018080e7          	jalr	24(ra) # 80002cdc <panic>
    80003ccc:	00002517          	auipc	a0,0x2
    80003cd0:	99c50513          	addi	a0,a0,-1636 # 80005668 <digits+0x30>
    80003cd4:	fffff097          	auipc	ra,0xfffff
    80003cd8:	008080e7          	jalr	8(ra) # 80002cdc <panic>

0000000080003cdc <push_on>:
    80003cdc:	fe010113          	addi	sp,sp,-32
    80003ce0:	00813823          	sd	s0,16(sp)
    80003ce4:	00113c23          	sd	ra,24(sp)
    80003ce8:	00913423          	sd	s1,8(sp)
    80003cec:	02010413          	addi	s0,sp,32
    80003cf0:	100024f3          	csrr	s1,sstatus
    80003cf4:	100027f3          	csrr	a5,sstatus
    80003cf8:	0027e793          	ori	a5,a5,2
    80003cfc:	10079073          	csrw	sstatus,a5
    80003d00:	ffffe097          	auipc	ra,0xffffe
    80003d04:	618080e7          	jalr	1560(ra) # 80002318 <mycpu>
    80003d08:	07852783          	lw	a5,120(a0)
    80003d0c:	02078663          	beqz	a5,80003d38 <push_on+0x5c>
    80003d10:	ffffe097          	auipc	ra,0xffffe
    80003d14:	608080e7          	jalr	1544(ra) # 80002318 <mycpu>
    80003d18:	07852783          	lw	a5,120(a0)
    80003d1c:	01813083          	ld	ra,24(sp)
    80003d20:	01013403          	ld	s0,16(sp)
    80003d24:	0017879b          	addiw	a5,a5,1
    80003d28:	06f52c23          	sw	a5,120(a0)
    80003d2c:	00813483          	ld	s1,8(sp)
    80003d30:	02010113          	addi	sp,sp,32
    80003d34:	00008067          	ret
    80003d38:	0014d493          	srli	s1,s1,0x1
    80003d3c:	ffffe097          	auipc	ra,0xffffe
    80003d40:	5dc080e7          	jalr	1500(ra) # 80002318 <mycpu>
    80003d44:	0014f493          	andi	s1,s1,1
    80003d48:	06952e23          	sw	s1,124(a0)
    80003d4c:	fc5ff06f          	j	80003d10 <push_on+0x34>

0000000080003d50 <pop_on>:
    80003d50:	ff010113          	addi	sp,sp,-16
    80003d54:	00813023          	sd	s0,0(sp)
    80003d58:	00113423          	sd	ra,8(sp)
    80003d5c:	01010413          	addi	s0,sp,16
    80003d60:	ffffe097          	auipc	ra,0xffffe
    80003d64:	5b8080e7          	jalr	1464(ra) # 80002318 <mycpu>
    80003d68:	100027f3          	csrr	a5,sstatus
    80003d6c:	0027f793          	andi	a5,a5,2
    80003d70:	04078463          	beqz	a5,80003db8 <pop_on+0x68>
    80003d74:	07852783          	lw	a5,120(a0)
    80003d78:	02f05863          	blez	a5,80003da8 <pop_on+0x58>
    80003d7c:	fff7879b          	addiw	a5,a5,-1
    80003d80:	06f52c23          	sw	a5,120(a0)
    80003d84:	07853783          	ld	a5,120(a0)
    80003d88:	00079863          	bnez	a5,80003d98 <pop_on+0x48>
    80003d8c:	100027f3          	csrr	a5,sstatus
    80003d90:	ffd7f793          	andi	a5,a5,-3
    80003d94:	10079073          	csrw	sstatus,a5
    80003d98:	00813083          	ld	ra,8(sp)
    80003d9c:	00013403          	ld	s0,0(sp)
    80003da0:	01010113          	addi	sp,sp,16
    80003da4:	00008067          	ret
    80003da8:	00002517          	auipc	a0,0x2
    80003dac:	90050513          	addi	a0,a0,-1792 # 800056a8 <digits+0x70>
    80003db0:	fffff097          	auipc	ra,0xfffff
    80003db4:	f2c080e7          	jalr	-212(ra) # 80002cdc <panic>
    80003db8:	00002517          	auipc	a0,0x2
    80003dbc:	8d050513          	addi	a0,a0,-1840 # 80005688 <digits+0x50>
    80003dc0:	fffff097          	auipc	ra,0xfffff
    80003dc4:	f1c080e7          	jalr	-228(ra) # 80002cdc <panic>

0000000080003dc8 <__memset>:
    80003dc8:	ff010113          	addi	sp,sp,-16
    80003dcc:	00813423          	sd	s0,8(sp)
    80003dd0:	01010413          	addi	s0,sp,16
    80003dd4:	1a060e63          	beqz	a2,80003f90 <__memset+0x1c8>
    80003dd8:	40a007b3          	neg	a5,a0
    80003ddc:	0077f793          	andi	a5,a5,7
    80003de0:	00778693          	addi	a3,a5,7
    80003de4:	00b00813          	li	a6,11
    80003de8:	0ff5f593          	andi	a1,a1,255
    80003dec:	fff6071b          	addiw	a4,a2,-1
    80003df0:	1b06e663          	bltu	a3,a6,80003f9c <__memset+0x1d4>
    80003df4:	1cd76463          	bltu	a4,a3,80003fbc <__memset+0x1f4>
    80003df8:	1a078e63          	beqz	a5,80003fb4 <__memset+0x1ec>
    80003dfc:	00b50023          	sb	a1,0(a0)
    80003e00:	00100713          	li	a4,1
    80003e04:	1ae78463          	beq	a5,a4,80003fac <__memset+0x1e4>
    80003e08:	00b500a3          	sb	a1,1(a0)
    80003e0c:	00200713          	li	a4,2
    80003e10:	1ae78a63          	beq	a5,a4,80003fc4 <__memset+0x1fc>
    80003e14:	00b50123          	sb	a1,2(a0)
    80003e18:	00300713          	li	a4,3
    80003e1c:	18e78463          	beq	a5,a4,80003fa4 <__memset+0x1dc>
    80003e20:	00b501a3          	sb	a1,3(a0)
    80003e24:	00400713          	li	a4,4
    80003e28:	1ae78263          	beq	a5,a4,80003fcc <__memset+0x204>
    80003e2c:	00b50223          	sb	a1,4(a0)
    80003e30:	00500713          	li	a4,5
    80003e34:	1ae78063          	beq	a5,a4,80003fd4 <__memset+0x20c>
    80003e38:	00b502a3          	sb	a1,5(a0)
    80003e3c:	00700713          	li	a4,7
    80003e40:	18e79e63          	bne	a5,a4,80003fdc <__memset+0x214>
    80003e44:	00b50323          	sb	a1,6(a0)
    80003e48:	00700e93          	li	t4,7
    80003e4c:	00859713          	slli	a4,a1,0x8
    80003e50:	00e5e733          	or	a4,a1,a4
    80003e54:	01059e13          	slli	t3,a1,0x10
    80003e58:	01c76e33          	or	t3,a4,t3
    80003e5c:	01859313          	slli	t1,a1,0x18
    80003e60:	006e6333          	or	t1,t3,t1
    80003e64:	02059893          	slli	a7,a1,0x20
    80003e68:	40f60e3b          	subw	t3,a2,a5
    80003e6c:	011368b3          	or	a7,t1,a7
    80003e70:	02859813          	slli	a6,a1,0x28
    80003e74:	0108e833          	or	a6,a7,a6
    80003e78:	03059693          	slli	a3,a1,0x30
    80003e7c:	003e589b          	srliw	a7,t3,0x3
    80003e80:	00d866b3          	or	a3,a6,a3
    80003e84:	03859713          	slli	a4,a1,0x38
    80003e88:	00389813          	slli	a6,a7,0x3
    80003e8c:	00f507b3          	add	a5,a0,a5
    80003e90:	00e6e733          	or	a4,a3,a4
    80003e94:	000e089b          	sext.w	a7,t3
    80003e98:	00f806b3          	add	a3,a6,a5
    80003e9c:	00e7b023          	sd	a4,0(a5)
    80003ea0:	00878793          	addi	a5,a5,8
    80003ea4:	fed79ce3          	bne	a5,a3,80003e9c <__memset+0xd4>
    80003ea8:	ff8e7793          	andi	a5,t3,-8
    80003eac:	0007871b          	sext.w	a4,a5
    80003eb0:	01d787bb          	addw	a5,a5,t4
    80003eb4:	0ce88e63          	beq	a7,a4,80003f90 <__memset+0x1c8>
    80003eb8:	00f50733          	add	a4,a0,a5
    80003ebc:	00b70023          	sb	a1,0(a4)
    80003ec0:	0017871b          	addiw	a4,a5,1
    80003ec4:	0cc77663          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003ec8:	00e50733          	add	a4,a0,a4
    80003ecc:	00b70023          	sb	a1,0(a4)
    80003ed0:	0027871b          	addiw	a4,a5,2
    80003ed4:	0ac77e63          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003ed8:	00e50733          	add	a4,a0,a4
    80003edc:	00b70023          	sb	a1,0(a4)
    80003ee0:	0037871b          	addiw	a4,a5,3
    80003ee4:	0ac77663          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003ee8:	00e50733          	add	a4,a0,a4
    80003eec:	00b70023          	sb	a1,0(a4)
    80003ef0:	0047871b          	addiw	a4,a5,4
    80003ef4:	08c77e63          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003ef8:	00e50733          	add	a4,a0,a4
    80003efc:	00b70023          	sb	a1,0(a4)
    80003f00:	0057871b          	addiw	a4,a5,5
    80003f04:	08c77663          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f08:	00e50733          	add	a4,a0,a4
    80003f0c:	00b70023          	sb	a1,0(a4)
    80003f10:	0067871b          	addiw	a4,a5,6
    80003f14:	06c77e63          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f18:	00e50733          	add	a4,a0,a4
    80003f1c:	00b70023          	sb	a1,0(a4)
    80003f20:	0077871b          	addiw	a4,a5,7
    80003f24:	06c77663          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f28:	00e50733          	add	a4,a0,a4
    80003f2c:	00b70023          	sb	a1,0(a4)
    80003f30:	0087871b          	addiw	a4,a5,8
    80003f34:	04c77e63          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f38:	00e50733          	add	a4,a0,a4
    80003f3c:	00b70023          	sb	a1,0(a4)
    80003f40:	0097871b          	addiw	a4,a5,9
    80003f44:	04c77663          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f48:	00e50733          	add	a4,a0,a4
    80003f4c:	00b70023          	sb	a1,0(a4)
    80003f50:	00a7871b          	addiw	a4,a5,10
    80003f54:	02c77e63          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f58:	00e50733          	add	a4,a0,a4
    80003f5c:	00b70023          	sb	a1,0(a4)
    80003f60:	00b7871b          	addiw	a4,a5,11
    80003f64:	02c77663          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f68:	00e50733          	add	a4,a0,a4
    80003f6c:	00b70023          	sb	a1,0(a4)
    80003f70:	00c7871b          	addiw	a4,a5,12
    80003f74:	00c77e63          	bgeu	a4,a2,80003f90 <__memset+0x1c8>
    80003f78:	00e50733          	add	a4,a0,a4
    80003f7c:	00b70023          	sb	a1,0(a4)
    80003f80:	00d7879b          	addiw	a5,a5,13
    80003f84:	00c7f663          	bgeu	a5,a2,80003f90 <__memset+0x1c8>
    80003f88:	00f507b3          	add	a5,a0,a5
    80003f8c:	00b78023          	sb	a1,0(a5)
    80003f90:	00813403          	ld	s0,8(sp)
    80003f94:	01010113          	addi	sp,sp,16
    80003f98:	00008067          	ret
    80003f9c:	00b00693          	li	a3,11
    80003fa0:	e55ff06f          	j	80003df4 <__memset+0x2c>
    80003fa4:	00300e93          	li	t4,3
    80003fa8:	ea5ff06f          	j	80003e4c <__memset+0x84>
    80003fac:	00100e93          	li	t4,1
    80003fb0:	e9dff06f          	j	80003e4c <__memset+0x84>
    80003fb4:	00000e93          	li	t4,0
    80003fb8:	e95ff06f          	j	80003e4c <__memset+0x84>
    80003fbc:	00000793          	li	a5,0
    80003fc0:	ef9ff06f          	j	80003eb8 <__memset+0xf0>
    80003fc4:	00200e93          	li	t4,2
    80003fc8:	e85ff06f          	j	80003e4c <__memset+0x84>
    80003fcc:	00400e93          	li	t4,4
    80003fd0:	e7dff06f          	j	80003e4c <__memset+0x84>
    80003fd4:	00500e93          	li	t4,5
    80003fd8:	e75ff06f          	j	80003e4c <__memset+0x84>
    80003fdc:	00600e93          	li	t4,6
    80003fe0:	e6dff06f          	j	80003e4c <__memset+0x84>

0000000080003fe4 <__memmove>:
    80003fe4:	ff010113          	addi	sp,sp,-16
    80003fe8:	00813423          	sd	s0,8(sp)
    80003fec:	01010413          	addi	s0,sp,16
    80003ff0:	0e060863          	beqz	a2,800040e0 <__memmove+0xfc>
    80003ff4:	fff6069b          	addiw	a3,a2,-1
    80003ff8:	0006881b          	sext.w	a6,a3
    80003ffc:	0ea5e863          	bltu	a1,a0,800040ec <__memmove+0x108>
    80004000:	00758713          	addi	a4,a1,7
    80004004:	00a5e7b3          	or	a5,a1,a0
    80004008:	40a70733          	sub	a4,a4,a0
    8000400c:	0077f793          	andi	a5,a5,7
    80004010:	00f73713          	sltiu	a4,a4,15
    80004014:	00174713          	xori	a4,a4,1
    80004018:	0017b793          	seqz	a5,a5
    8000401c:	00e7f7b3          	and	a5,a5,a4
    80004020:	10078863          	beqz	a5,80004130 <__memmove+0x14c>
    80004024:	00900793          	li	a5,9
    80004028:	1107f463          	bgeu	a5,a6,80004130 <__memmove+0x14c>
    8000402c:	0036581b          	srliw	a6,a2,0x3
    80004030:	fff8081b          	addiw	a6,a6,-1
    80004034:	02081813          	slli	a6,a6,0x20
    80004038:	01d85893          	srli	a7,a6,0x1d
    8000403c:	00858813          	addi	a6,a1,8
    80004040:	00058793          	mv	a5,a1
    80004044:	00050713          	mv	a4,a0
    80004048:	01088833          	add	a6,a7,a6
    8000404c:	0007b883          	ld	a7,0(a5)
    80004050:	00878793          	addi	a5,a5,8
    80004054:	00870713          	addi	a4,a4,8
    80004058:	ff173c23          	sd	a7,-8(a4)
    8000405c:	ff0798e3          	bne	a5,a6,8000404c <__memmove+0x68>
    80004060:	ff867713          	andi	a4,a2,-8
    80004064:	02071793          	slli	a5,a4,0x20
    80004068:	0207d793          	srli	a5,a5,0x20
    8000406c:	00f585b3          	add	a1,a1,a5
    80004070:	40e686bb          	subw	a3,a3,a4
    80004074:	00f507b3          	add	a5,a0,a5
    80004078:	06e60463          	beq	a2,a4,800040e0 <__memmove+0xfc>
    8000407c:	0005c703          	lbu	a4,0(a1)
    80004080:	00e78023          	sb	a4,0(a5)
    80004084:	04068e63          	beqz	a3,800040e0 <__memmove+0xfc>
    80004088:	0015c603          	lbu	a2,1(a1)
    8000408c:	00100713          	li	a4,1
    80004090:	00c780a3          	sb	a2,1(a5)
    80004094:	04e68663          	beq	a3,a4,800040e0 <__memmove+0xfc>
    80004098:	0025c603          	lbu	a2,2(a1)
    8000409c:	00200713          	li	a4,2
    800040a0:	00c78123          	sb	a2,2(a5)
    800040a4:	02e68e63          	beq	a3,a4,800040e0 <__memmove+0xfc>
    800040a8:	0035c603          	lbu	a2,3(a1)
    800040ac:	00300713          	li	a4,3
    800040b0:	00c781a3          	sb	a2,3(a5)
    800040b4:	02e68663          	beq	a3,a4,800040e0 <__memmove+0xfc>
    800040b8:	0045c603          	lbu	a2,4(a1)
    800040bc:	00400713          	li	a4,4
    800040c0:	00c78223          	sb	a2,4(a5)
    800040c4:	00e68e63          	beq	a3,a4,800040e0 <__memmove+0xfc>
    800040c8:	0055c603          	lbu	a2,5(a1)
    800040cc:	00500713          	li	a4,5
    800040d0:	00c782a3          	sb	a2,5(a5)
    800040d4:	00e68663          	beq	a3,a4,800040e0 <__memmove+0xfc>
    800040d8:	0065c703          	lbu	a4,6(a1)
    800040dc:	00e78323          	sb	a4,6(a5)
    800040e0:	00813403          	ld	s0,8(sp)
    800040e4:	01010113          	addi	sp,sp,16
    800040e8:	00008067          	ret
    800040ec:	02061713          	slli	a4,a2,0x20
    800040f0:	02075713          	srli	a4,a4,0x20
    800040f4:	00e587b3          	add	a5,a1,a4
    800040f8:	f0f574e3          	bgeu	a0,a5,80004000 <__memmove+0x1c>
    800040fc:	02069613          	slli	a2,a3,0x20
    80004100:	02065613          	srli	a2,a2,0x20
    80004104:	fff64613          	not	a2,a2
    80004108:	00e50733          	add	a4,a0,a4
    8000410c:	00c78633          	add	a2,a5,a2
    80004110:	fff7c683          	lbu	a3,-1(a5)
    80004114:	fff78793          	addi	a5,a5,-1
    80004118:	fff70713          	addi	a4,a4,-1
    8000411c:	00d70023          	sb	a3,0(a4)
    80004120:	fec798e3          	bne	a5,a2,80004110 <__memmove+0x12c>
    80004124:	00813403          	ld	s0,8(sp)
    80004128:	01010113          	addi	sp,sp,16
    8000412c:	00008067          	ret
    80004130:	02069713          	slli	a4,a3,0x20
    80004134:	02075713          	srli	a4,a4,0x20
    80004138:	00170713          	addi	a4,a4,1
    8000413c:	00e50733          	add	a4,a0,a4
    80004140:	00050793          	mv	a5,a0
    80004144:	0005c683          	lbu	a3,0(a1)
    80004148:	00178793          	addi	a5,a5,1
    8000414c:	00158593          	addi	a1,a1,1
    80004150:	fed78fa3          	sb	a3,-1(a5)
    80004154:	fee798e3          	bne	a5,a4,80004144 <__memmove+0x160>
    80004158:	f89ff06f          	j	800040e0 <__memmove+0xfc>
	...
