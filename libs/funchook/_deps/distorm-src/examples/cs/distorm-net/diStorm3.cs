using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace diStorm
{
  public enum DecodeType
  {
    Decode16Bits,
    Decode32Bits,
    Decode64Bits
  }

  public class diStorm3
  {
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    public unsafe struct _CodeInfo
    {
      internal IntPtr codeOffset;
	  internal IntPtr addrMask;
      internal IntPtr nextOffset;
      internal byte* code;
      internal int codeLen;
      internal DecodeType dt;
      internal int features;
    };

    public struct _WString
    {
      public const int MAX_TEXT_SIZE = 48;
      public uint length;
      public unsafe fixed sbyte p[MAX_TEXT_SIZE];
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    public struct _DecodedInst
    {
	    public IntPtr offset;
	    public uint size;
	    public _WString mnemonic;
	    public _WString operands;
	    public _WString instructionHex;
    };

    public struct PtrStruct
    {
      private ushort seg;

      private uint off;
    };

    public struct ExStruct
    {
      private uint i1;
      private uint i2;
    };

    [StructLayout(LayoutKind.Explicit)]
    public struct _Value
    {

      [FieldOffset(0)] public sbyte sbyt;
      [FieldOffset(0)] public byte byt;
      [FieldOffset(0)] public short sword;
      [FieldOffset(0)] public ushort word;
      [FieldOffset(0)] public int sdword;
      [FieldOffset(0)] public uint dword;
      [FieldOffset(0)] public long sqword;
      [FieldOffset(0)] public ulong qword;

      [FieldOffset(0)] public IntPtr addr;
      [FieldOffset(0)] public PtrStruct ptr;
      [FieldOffset(0)] public ExStruct ex;
    };

    public struct _Operand
    {

      public OperandType type;

      public byte index;

      public ushort size;
    };

    public struct _DInst
    {
      public const int OPERANDS_NO = 4;
      private const int OPERANDS_SIZE = 4*OPERANDS_NO;

      internal _Value imm;

      internal ulong disp;

      internal IntPtr addr;

      internal ushort flags;

      internal ushort unusedPrefixesMask;

      internal ushort usedRegistersMask;

      internal ushort opcode;

      private unsafe fixed byte ops_storage[OPERANDS_SIZE];
      internal unsafe _Operand* ops
      {
        get
        {
          fixed (byte* p = ops_storage)
          {
            return (_Operand*) p;
          }
        }
      }

      internal byte opsNo;

      internal byte size;

      internal byte segment;

      internal byte ibase, scale;
      internal byte dispSize;

      internal ushort meta;

      internal ushort modifiedFlagsMask, testedFlagsMask, undefinedFlagsMask;
    };

    [DllImport("distorm3")]
    private static extern unsafe void distorm_decompose64(void* codeInfo, void* dinsts, int maxInstructions, int* usedInstructions);

    [DllImport("distorm3")]
    private static extern unsafe void distorm_decode64(IntPtr codeOffset, byte* code, int codeLen, DecodeType dt, void *result, uint maxInstructions, uint* usedInstructionsCount);

    [DllImport("distorm3")]
    private static extern unsafe void distorm_format64(void* codeInfo, void* dinst, void* output);

    public static unsafe void* Malloc(int sz)
    {
      return Marshal.AllocHGlobal(new IntPtr(sz)).ToPointer();
    }

    private static unsafe void Free(void* mem)
    {
      Marshal.FreeHGlobal(new IntPtr(mem));
    }

    private static unsafe _CodeInfo* AcquireCodeInfoStruct(CodeInfo nci, out GCHandle gch)
    {
      var ci = (_CodeInfo*) Malloc(sizeof (_CodeInfo));
      if (ci == null)
        throw new OutOfMemoryException();

      Memset(ci, 0, sizeof (_CodeInfo));

      ci->codeOffset = new IntPtr(nci._codeOffset);
      gch = GCHandle.Alloc(nci._code, GCHandleType.Pinned);

      ci->code = (byte*) gch.AddrOfPinnedObject().ToPointer();
      ci->codeLen = nci._code.Length;
      ci->dt = nci._decodeType;
      ci->features = nci._features;
      return ci;
    }

    private static unsafe DecodedInst CreateDecodedInstObj(_DecodedInst* inst)
    {
      return new DecodedInst {
        Offset = inst->offset,
        Size = inst->size,
        Mnemonic = new String(inst->mnemonic.p),
        Operands = new String(inst->operands.p),
        Hex = new string(inst->instructionHex.p)
      };
    }

    private static unsafe void Memset(void *p, int v, int sz)
    {
    }

    public static unsafe void Decompose(CodeInfo nci, DecomposedResult ndr)
    {
	    _CodeInfo* ci = null;
      _DInst* insts = null;
      var gch = new GCHandle();
      var usedInstructionsCount = 0;

      try
      {
        if ((ci = AcquireCodeInfoStruct(nci, out gch)) == null)
          throw new OutOfMemoryException();

        var maxInstructions = ndr.MaxInstructions;

        if ((insts = (_DInst*) Malloc(maxInstructions*sizeof (_DInst))) == null)
          throw new OutOfMemoryException();

        distorm_decompose64(ci, insts, maxInstructions, &usedInstructionsCount);

        var dinsts = new DecomposedInst[usedInstructionsCount];

        for (var i = 0; i < usedInstructionsCount; i++) {
          var di = new DecomposedInst {
            Address = insts[i].addr,
            Flags = insts[i].flags,
            Size = insts[i].size,
            _segment = insts[i].segment,
            Base = insts[i].ibase,
            Scale = insts[i].scale,
            Opcode = (Opcode) insts[i].opcode,
            UnusedPrefixesMask = insts[i].unusedPrefixesMask,
            Meta = insts[i].meta,
            RegistersMask = insts[i].usedRegistersMask,
            ModifiedFlagsMask = insts[i].modifiedFlagsMask,
            TestedFlagsMask = insts[i].testedFlagsMask,
            UndefinedFlagsMask = insts[i].undefinedFlagsMask
          };

          var immVariant = new DecomposedInst.ImmVariant {
            Imm = insts[i].imm.qword,
            Size = 0
          };

          var operandsNo = 0;
          for (operandsNo = 0; operandsNo < _DInst.OPERANDS_NO; operandsNo++)
          {
            if (insts[i].ops[operandsNo].type == OperandType.None)
              break;
          }

          var ops = new Operand[operandsNo];

          for (var j = 0; j < operandsNo; j++)
          {
            if (insts[i].ops[j].type == OperandType.Imm) {

              immVariant.Size = insts[i].ops[j].size;
            }

            var op = new Operand {
              Type = insts[i].ops[j].type,
              Index = insts[i].ops[j].index,
              Size = insts[i].ops[j].size
            };

            ops[j] = op;
          }
          di.Operands = ops;

          di.Imm = immVariant;

          var disp = new DecomposedInst.DispVariant {
            Displacement = insts[i].disp,
            Size = insts[i].dispSize
          };

          di.Disp = disp;
          dinsts[i] = di;
        }

        ndr.Instructions = dinsts;
      }
      finally
      {
        if (gch.IsAllocated)
          gch.Free();
        if (ci != null)
          Free(ci);
        if (insts != null)
          Free(insts);
      }
    }

    public static unsafe void Decode(CodeInfo nci, DecodedResult dr)
    {
      _CodeInfo* ci = null;
      _DecodedInst* insts = null;
      var gch = new GCHandle();
      uint usedInstructionsCount = 0;

      try
      {
        if ((ci = AcquireCodeInfoStruct(nci, out gch)) == null)
          throw new OutOfMemoryException();

        var maxInstructions = dr.MaxInstructions;

        if ((insts = (_DecodedInst*) Malloc(maxInstructions*sizeof (_DecodedInst))) == null)
          throw new OutOfMemoryException();

        distorm_decode64(ci->codeOffset, ci->code, ci->codeLen, ci->dt, insts, (uint) maxInstructions,
                         &usedInstructionsCount);

        var dinsts = new DecodedInst[usedInstructionsCount];

        for (var i = 0; i < usedInstructionsCount; i++)
          dinsts[i] = CreateDecodedInstObj(&insts[i]);
        dr.Instructions = dinsts;
      }
      finally {

        if (gch.IsAllocated)
          gch.Free();
        if (ci != null)
          Free(ci);
        if (insts != null)
          Free(insts);
      }
    }

    public static unsafe DecodedInst Format(CodeInfo nci, DecomposedInst ndi)
    {
      var input = new _DInst();
      _CodeInfo *ci = null;
      var gch = new GCHandle();
      DecodedInst di;

      try
      {
        ci = AcquireCodeInfoStruct(nci, out gch);
        if (ci == null)
          throw new OutOfMemoryException();

        input.addr = ndi.Address;
        input.flags = ndi.Flags;
        input.size = (byte) ndi.Size;
        input.segment = (byte) ndi._segment;
        input.ibase = (byte) ndi.Base;
        input.scale = (byte) ndi.Scale;
        input.opcode = (ushort) ndi.Opcode;

        input.meta = (ushort) ndi.Meta;

        int opsCount = ndi.Operands.Length;
        for (var i = 0; i < opsCount; i++) {
          var op = ndi.Operands[i];
          if (op == null) continue;
          input.ops[i].index = (byte) op.Index;
          input.ops[i].type = op.Type;
          input.ops[i].size = (ushort) op.Size;
        }

        if (ndi.Imm != null)
          input.imm.qword = ndi.Imm.Imm;

        if (ndi.Disp != null)
        {
          input.disp = ndi.Disp.Displacement;
          input.dispSize = (byte) ndi.Disp.Size;
        }

        _DecodedInst output;
        distorm_format64(ci, &input, &output);

        di = CreateDecodedInstObj(&output);
      }
      finally
      {
        if (gch.IsAllocated)
          gch.Free();
        if (ci != null)
          Free(ci);
      }
      return di;
    }
  }
}
