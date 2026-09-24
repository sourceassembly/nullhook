info = (
    "diStorm3 by Gil Dabah, https://github.com/gdabah/distorm/\n"
    "Based on diStorm64 Python binding by Mario Vilas, http://breakingcode.wordpress.com/\n"
)

__revision__ = "$Id: distorm.py 186 2010-05-01 14:20:41Z gdabah $"

__all__ = [
    'Decode',
    'DecodeGenerator',
    'Decompose',
    'DecomposeGenerator',
    'Decode16Bits',
    'Decode32Bits',
    'Decode64Bits',
    'Mnemonics',
    'Registers',
    'RegisterMasks'
]

from ctypes import *
import os
import sys
from ._generated import Registers, Mnemonics, RegisterMasks

if sys.version_info[0] >= 3:
    xrange = range

def _load_distorm():
    if sys.version_info[0] == 3:
        try:
            import _distorm3
            return cdll.LoadLibrary(_distorm3.__spec__.origin)
        except ImportError:
            pass

    dll_ext = ('.dll' if sys.platform == 'win32' else '.so')
    libnames = ['_distorm3' + dll_ext, '_distorm3.pyd']
    for dir in sys.path:
        for name in libnames:
            _distorm_file = os.path.join(dir, name)
            if os.path.isfile(_distorm_file):
                return cdll.LoadLibrary(_distorm_file)
    raise ImportError("Error loading the diStorm dynamic library (or cannot load library into process).")

_distorm = _load_distorm()

SUPPORT_64BIT_OFFSET = False
try:
    internal_decode = _distorm.distorm_decode64
    internal_decompose = _distorm.distorm_decompose64
    internal_format = _distorm.distorm_format64
    SUPPORT_64BIT_OFFSET = True
except AttributeError:
    internal_decode = _distorm.distorm_decode32
    internal_decompose = _distorm.distorm_decompose32
    internal_format = _distorm.distorm_format32

MAX_TEXT_SIZE       = 48
MAX_INSTRUCTIONS    = 1000

DECRES_NONE         = 0
DECRES_SUCCESS      = 1
DECRES_MEMORYERR    = 2
DECRES_INPUTERR     = 3

if SUPPORT_64BIT_OFFSET:
    _OffsetType = c_ulonglong
else:
    _OffsetType = c_uint

class _WString (Structure):
    _fields_ = [
        ('length',  c_uint),
        ('p',       c_char * MAX_TEXT_SIZE),
    ]

class _CodeInfo (Structure):
    _fields_ = [
        ('codeOffset',  _OffsetType),
        ('addrMask',    _OffsetType),
        ('nextOffset',  _OffsetType),
        ('code',        c_char_p),
        ('codeLen',     c_int),
        ('dt',          c_byte),
        ('features',    c_uint),
        ]

class _DecodedInst (Structure):
    _fields_ = [
        ('offset',          _OffsetType),
        ('size',            c_uint),
        ('mnemonic',        _WString),
        ('operands',        _WString),
        ('instructionHex',  _WString)
    ]

_OperandType = c_ubyte

O_NONE = 0
O_REG  = 1
O_IMM  = 2
O_IMM1 = 3
O_IMM2 = 4
O_DISP = 5
O_SMEM = 6
O_MEM  = 7
O_PC   = 8
O_PTR  = 9

class _Operand (Structure):
    _fields_ = [
        ('type',  c_ubyte),
        ('index', c_ubyte),
        ('size',  c_uint16),
    ]

class _ex (Structure):
    _fields_ = [
        ('i1', c_uint32),
        ('i2', c_uint32),
    ]
class _ptr (Structure):
    _fields_ = [
        ('seg', c_uint16),
        ('off', c_uint32),
    ]

class _Value (Union):
    _fields_ = [
        ('sbyte', c_byte),
        ('byte', c_ubyte),
        ('sword', c_int16),
        ('word', c_uint16),
        ('sdword', c_int32),
        ('dword', c_uint32),
        ('sqword', c_int64),
        ('qword', c_uint64),
        ('addr', _OffsetType),
        ('ptr', _ptr),
        ('ex', _ex),
        ]

class _DInst (Structure):
    _fields_ = [
        ('imm', _Value),
        ('disp', c_uint64),
        ('addr',  _OffsetType),
        ('flags',  c_uint16),
        ('unusedPrefixesMask', c_uint16),
        ('usedRegistersMask', c_uint32),
        ('opcode', c_uint16),
        ('ops', _Operand*4),
        ('opsNo', c_ubyte),
        ('size', c_ubyte),
        ('segment', c_ubyte),
        ('base', c_ubyte),
        ('scale', c_ubyte),
        ('dispSize', c_ubyte),
        ('meta', c_uint16),
        ('modifiedFlagsMask', c_uint16),
        ('testedFlagsMask', c_uint16),
        ('undefinedFlagsMask', c_uint16)
        ]

Decode16Bits    = 0
Decode32Bits    = 1
Decode64Bits    = 2
OffsetTypeSize  = sizeof(_OffsetType)

R_NONE = 0xFF

FLAGS = [

"FLAG_LOCK",

"FLAG_REPNZ",

"FLAG_REP",

"FLAG_HINT_TAKEN",

"FLAG_HINT_NOT_TAKEN",

"FLAG_IMM_SIGNED",

"FLAG_DST_WR",

"FLAG_RIP_RELATIVE"
]

D_CF = 1
D_PF = 4
D_AF = 0x10
D_ZF = 0x40
D_SF = 0x80
D_IF = 0x200
D_DF = 0x400
D_OF = 0x800

FLAG_NOT_DECODABLE = 0xFFFF

DF_NONE = 0
DF_MAXIMUM_ADDR16 = 1
DF_MAXIMUM_ADDR32 = 2
DF_RETURN_FC_ONLY = 4

DF_STOP_ON_CALL = 0x8
DF_STOP_ON_RET  = 0x10
DF_STOP_ON_SYS  = 0x20
DF_STOP_ON_UNC_BRANCH  = 0x40
DF_STOP_ON_CND_BRANCH  = 0x80
DF_STOP_ON_INT  = 0x100
DF_STOP_ON_CMOV  = 0x200
DF_STOP_ON_HLT  = 0x400
DF_STOP_ON_PRIVILEGED = 0x800
DF_STOP_ON_UNDECODEABLE = 0x1000
DF_SINGLE_BYTE_STEP = 0x2000
DF_FILL_EFLAGS = 0x4000
DF_USE_ADDR_MASK = 0x8000

DF_STOP_ON_FLOW_CONTROL = (DF_STOP_ON_CALL | DF_STOP_ON_RET | DF_STOP_ON_SYS | \
    DF_STOP_ON_UNC_BRANCH | DF_STOP_ON_CND_BRANCH | DF_STOP_ON_INT | DF_STOP_ON_CMOV | \
    DF_STOP_ON_HLT)

def DecodeGenerator(codeOffset, code, dt):
    """
    @type  codeOffset: long
    @param codeOffset: Memory address where the code is located.
        This is B{not} an offset into the code!
        It's the actual memory address where it was read from.

    @type  code: str
    @param code: Code to disassemble.

    @type  dt: int
    @param dt: Disassembly type. Can be one of the following:

         * L{Decode16Bits}: 80286 decoding

         * L{Decode32Bits}: IA-32 decoding

         * L{Decode64Bits}: AMD64 decoding

    @rtype:  generator of tuple( long, int, str, str )
    @return: Generator of tuples. Each tuple represents an assembly instruction
        and contains:
         - Memory address of instruction.
         - Size of instruction in bytes.
         - Disassembly line of instruction.
         - Hexadecimal dump of instruction.

    @raise ValueError: Invalid arguments.
    """

    if not code:
        return

    if not codeOffset:
        codeOffset = 0

    if dt not in (Decode16Bits, Decode32Bits, Decode64Bits):
        raise ValueError("Invalid decode type value: %r" % (dt,))

    codeLen         = len(code)
    code_buf        = create_string_buffer(code)
    p_code          = byref(code_buf)
    result          = (_DecodedInst * MAX_INSTRUCTIONS)()
    p_result        = byref(result)
    instruction_off = 0

    toUnicode = lambda s: s
    spaceCh = b" "
    if sys.version_info[0] >= 3:
        if sys.version_info[1] > 0:
            toUnicode = lambda s: s.decode()
        else:
            spaceCh = " "

    while codeLen > 0:

        usedInstructionsCount = c_uint(0)
        status = internal_decode(_OffsetType(codeOffset), p_code, codeLen, dt, p_result, MAX_INSTRUCTIONS, byref(usedInstructionsCount))

        if status == DECRES_INPUTERR:
            raise ValueError("Invalid arguments passed to distorm_decode()")

        used = usedInstructionsCount.value
        if not used:
            break

        for index in xrange(used):
            di   = result[index]
            asm  = di.mnemonic.p
            if len(di.operands.p):
                asm += spaceCh + di.operands.p
            pydi = (di.offset, di.size, toUnicode(asm), toUnicode(di.instructionHex.p))
            instruction_off += di.size
            yield pydi

        di         = result[used - 1]
        delta      = di.offset - codeOffset + result[used - 1].size
        if delta <= 0:
            break
        codeOffset = codeOffset + delta
        p_code     = byref(code_buf, instruction_off)
        codeLen    = codeLen - delta

def Decode(offset, code, type = Decode32Bits):
    """
    @type  offset: long
    @param offset: Memory address where the code is located.
        This is B{not} an offset into the code!
        It's the actual memory address where it was read from.

    @type  code: str
    @param code: Code to disassemble.

    @type  type: int
    @param type: Disassembly type. Can be one of the following:

         * L{Decode16Bits}: 80286 decoding

         * L{Decode32Bits}: IA-32 decoding

         * L{Decode64Bits}: AMD64 decoding

    @rtype:  list of tuple( long, int, str, str )
    @return: List of tuples. Each tuple represents an assembly instruction
        and contains:
         - Memory address of instruction.
         - Size of instruction in bytes.
         - Disassembly line of instruction.
         - Hexadecimal dump of instruction.

    @raise ValueError: Invalid arguments.
    """
    return list(DecodeGenerator(offset, code, type))

OPERAND_NONE = ""
OPERAND_IMMEDIATE = "Immediate"
OPERAND_REGISTER = "Register"

OPERAND_ABSOLUTE_ADDRESS = "AbsoluteMemoryAddress"
OPERAND_MEMORY = "AbsoluteMemory"
OPERAND_FAR_MEMORY = "FarMemory"

InstructionSetClasses = [
"ISC_UNKNOWN",

"ISC_INTEGER",

"ISC_FPU",

"ISC_P6",

"ISC_MMX",

"ISC_SSE",

"ISC_SSE2",

"ISC_SSE3",

"ISC_SSSE3",

"ISC_SSE4_1",

"ISC_SSE4_2",

"ISC_SSE4_A",

"ISC_3DNOW",

"ISC_3DNOWEXT",

"ISC_VMX",

"ISC_SVM",

"ISC_AVX",

"ISC_FMA",

"ISC_AES",

"ISC_CLMUL",
]

FlowControlFlags = [

"FC_NONE",

"FC_CALL",

"FC_RET",

"FC_SYS",

"FC_UNC_BRANCH",

"FC_CND_BRANCH",

"FC_INT",

"FC_CMOV",

"FC_HLT",
]

class FlowControl:
    """ The flow control instruction will be flagged in the lo byte of the 'meta' field in _InstInfo of diStorm.
    They are used to distinguish between flow control instructions (such as: ret, call, jmp, jz, etc) to normal ones. """
    (CALL,
    RET,
    SYS,
    UNC_BRANCH,
    CND_BRANCH,
    INT,
    CMOV,
    HLT) = range(1, 9)

def _getOpSize(flags):
    return ((flags >> 7) & 3)

def _getISC(metaflags):
    realvalue = ((metaflags >> 8) & 0x1f)
    try:
        return InstructionSetClasses[realvalue]
    except IndexError:
        print ("Bad ISC flags in meta member: {}".format(realvalue))
        raise

def _getFC(metaflags):
    realvalue = (metaflags & 0xf)
    try:
        return FlowControlFlags[realvalue]
    except IndexError:
        print ("Bad FlowControl flags in meta member: {}".format(realvalue))
        raise

def _getMnem(opcode):
    return Mnemonics.get(opcode, "UNDEFINED")

def _unsignedToSigned64(val):
    return int(val if val < 0x8000000000000000 else (val - 0x10000000000000000))

def _unsignedToSigned32(val):
    return int(val if val < 0x80000000 else (val - 0x10000000))

if SUPPORT_64BIT_OFFSET:
    _unsignedToSigned = _unsignedToSigned64
else:
    _unsignedToSigned = _unsignedToSigned32

class Operand (object):
    def __init__(self, type, *args):
        self.type = type
        self.index = None
        self.name = ""
        self.size = 0
        self.value = 0
        self.disp = 0
        self.dispSize = 0
        self.base = 0
        self.segment = 0
        if type == OPERAND_IMMEDIATE:
            self.value = int(args[0])
            self.size = args[1]
        elif type == OPERAND_REGISTER:
            self.index = args[0]
            self.size = args[1]
            self.name = Registers[self.index]
        elif type == OPERAND_MEMORY:
            self.base = args[0] if args[0] != R_NONE else None
            self.index = args[1]
            self.size = args[2]
            self.scale = args[3] if args[3] > 1 else 1
            self.disp = int(args[4])
            self.dispSize = args[5]
            self.segment = args[6]
        elif type == OPERAND_ABSOLUTE_ADDRESS:
            self.size = args[0]
            self.disp = int(args[1])
            self.dispSize = args[2]
            self.segment = args[3]
        elif type == OPERAND_FAR_MEMORY:
            self.size = args[2]
            self.seg = args[0]
            self.off = args[1]

    def _toText(self):
        if self.type == OPERAND_IMMEDIATE:
            if self.value >= 0:
                return "0x%x" % self.value
            else:
                return "-0x%x" % abs(self.value)
        elif self.type == OPERAND_REGISTER:
            return self.name
        elif self.type == OPERAND_ABSOLUTE_ADDRESS:
            return '[0x%x]' % self.disp
        elif self.type == OPERAND_FAR_MEMORY:
            return '%s:%s' % (hex(self.seg), hex(self.off))
        elif (self.type == OPERAND_MEMORY):
            result = "["
            if self.base != None:
                result += Registers[self.base] + "+"
            if self.index != None:
                result += Registers[self.index]
                if self.scale > 1:
                    result += "*%d" % self.scale
            if self.disp >= 0:
                result += "+0x%x" % self.disp
            else:
                result += "-0x%x" % abs(self.disp)
            return result + "]"
    def __str__(self):
        return self._toText()

class Instruction (object):
    def __init__(self, di, instructionBytes, dt):
        "Expects a filled _DInst structure, and the corresponding byte code of the whole instruction"

        flags = di.flags
        self.instructionBytes = instructionBytes
        self.opcode = di.opcode
        self.operands = []
        self.flags = []
        self.rawFlags = di.flags
        self.meta = 0
        self.privileged = False
        self.instructionClass = _getISC(0)
        self.flowControl = _getFC(0)
        self.address = di.addr
        self.size = di.size
        self.dt = dt
        self.valid = False
        if di.segment != R_NONE:
            self.segment = di.segment & 0x7f
            self.isSegmentDefault = (di.segment & 0x80) == 0x80
        else:
            self.segment = R_NONE
            self.isSegmentDefault = False
        self.unusedPrefixesMask = di.unusedPrefixesMask
        self.usedRegistersMask = di.usedRegistersMask

        self.registers = []
        maskIndex = 1
        v = self.usedRegistersMask
        while (v):
            if (v & maskIndex):
                self.registers.append(RegisterMasks[maskIndex])
                v ^= maskIndex
            maskIndex <<= 1

        if flags == FLAG_NOT_DECODABLE:
            self.mnemonic = 'DB 0x%02x' % (di.imm.byte)
            self.flags = ['FLAG_NOT_DECODABLE']
            return

        self.valid = True
        self.mnemonic = _getMnem(self.opcode)

        for index, flag in enumerate(FLAGS):
            if (flags & (1 << index)) != 0:
                self.flags.append(flag)

        for operand in di.ops:
            if operand.type != O_NONE:
                self.operands.append(self._extractOperand(di, operand))

        metas = di.meta
        self.meta = di.meta
        self.privileged = (metas & 0x8000) == 0x8000
        self.instructionClass = _getISC(metas)
        self.flowControl = _getFC(metas)

        self.modifiedFlags = di.modifiedFlagsMask
        self.undefinedFlags = di.undefinedFlagsMask
        self.testedFlags = di.testedFlagsMask

    def _extractOperand(self, di, operand):

        if operand.type == O_IMM:
            if ("FLAG_IMM_SIGNED" in self.flags):

                constant = _unsignedToSigned(di.imm.sqword)
            else:

                constant = di.imm.qword
            return Operand(OPERAND_IMMEDIATE, constant, operand.size)
        elif operand.type == O_IMM1:
            return Operand(OPERAND_IMMEDIATE, di.imm.ex.i1, operand.size)
        elif operand.type == O_IMM2:
            return Operand(OPERAND_IMMEDIATE, di.imm.ex.i2, operand.size)
        elif operand.type == O_REG:
            return Operand(OPERAND_REGISTER, operand.index, operand.size)
        elif operand.type == O_MEM:
            return Operand(OPERAND_MEMORY, di.base, operand.index, operand.size, di.scale, _unsignedToSigned(di.disp), di.dispSize, self.segment)
        elif operand.type == O_SMEM:
            return Operand(OPERAND_MEMORY, None, operand.index, operand.size, di.scale, _unsignedToSigned(di.disp), di.dispSize, self.segment)
        elif operand.type == O_DISP:
            return Operand(OPERAND_ABSOLUTE_ADDRESS, operand.size, di.disp, di.dispSize, self.segment)
        elif operand.type == O_PC:
            return Operand(OPERAND_IMMEDIATE, _unsignedToSigned(di.imm.addr) + self.address + self.size, operand.size)
        elif operand.type == O_PTR:
            return Operand(OPERAND_FAR_MEMORY, di.imm.ptr.seg, di.imm.ptr.off, operand.size)
        else:
            raise ValueError("Unknown operand type encountered: %d!" % operand.type)

    def _toText(self):

        return Decode(self.address, self.instructionBytes, self.dt)[0][2]

    def __str__(self):
        return self._toText()

def DecomposeGenerator(codeOffset, code, dt, features = 0):
    """
    @type  codeOffset: long
    @param codeOffset: Memory address where the code is located.
        This is B{not} an offset into the code!
        It's the actual memory address where it was read from.

    @type  code: str, in Py3 bytes
    @param code: Code to disassemble.

    @type  dt: int
    @param dt: Disassembly type. Can be one of the following:

         * L{Decode16Bits}: 80286 decoding

         * L{Decode32Bits}: IA-32 decoding

         * L{Decode64Bits}: AMD64 decoding

    @type  features: int
    @param features: A flow control stopping criterion, eg. DF_STOP_ON_CALL.
                     or other features, eg. DF_RETURN_FC_ONLY.

    @rtype:  generator of TODO
    @return: Generator of TODO

    @raise ValueError: Invalid arguments.
    """

    if not code:
        return

    if not codeOffset:
        codeOffset = 0

    if dt not in (Decode16Bits, Decode32Bits, Decode64Bits):
        raise ValueError("Invalid decode type value: %r" % (dt,))

    codeLen         = len(code)
    code_buf        = create_string_buffer(code)
    p_code          = byref(code_buf)
    result          = (_DInst * MAX_INSTRUCTIONS)()
    startCodeOffset = codeOffset

    while codeLen > 0:

        usedInstructionsCount = c_uint(0)
        codeInfo = _CodeInfo(_OffsetType(codeOffset), _OffsetType(0), _OffsetType(0), cast(p_code, c_char_p), codeLen, dt, features)
        status = internal_decompose(byref(codeInfo), byref(result), MAX_INSTRUCTIONS, byref(usedInstructionsCount))
        if status == DECRES_INPUTERR:
            raise ValueError("Invalid arguments passed to distorm_decode()")

        used = usedInstructionsCount.value
        if not used:
            break

        for index in range(used):
            di = result[index]
            yield Instruction(di, code[di.addr - startCodeOffset : di.addr - startCodeOffset + di.size], dt)

        lastInst = result[used - 1]
        delta = lastInst.addr + lastInst.size - codeOffset
        codeOffset = codeOffset + delta
        p_code     = byref(code_buf, codeOffset - startCodeOffset)
        codeLen    = codeLen - delta

        if (features & (DF_STOP_ON_FLOW_CONTROL | DF_STOP_ON_PRIVILEGED | DF_STOP_ON_UNDECODEABLE)) != 0:
            break

def Decompose(offset, code, type = Decode32Bits, features = 0):
    """
    @type  offset: long
    @param offset: Memory address where the code is located.
        This is B{not} an offset into the code!
        It's the actual memory address where it was read from.

    @type  code: str, in Py3 bytes
    @param code: Code to disassemble.

    @type  type: int
    @param type: Disassembly type. Can be one of the following:

         * L{Decode16Bits}: 80286 decoding

         * L{Decode32Bits}: IA-32 decoding

         * L{Decode64Bits}: AMD64 decoding

    @type  features: int
    @param features: A flow control stopping criterion, eg. DF_STOP_ON_CALL.
                     or other features, eg. DF_RETURN_FC_ONLY.

    @rtype:  TODO
    @return: TODO
    @raise ValueError: Invalid arguments.
    """
    return list(DecomposeGenerator(offset, code, type, features))
