using System.Diagnostics;
using System.Drawing.Imaging;

// TODO: fix memory spikes
// TODO: fix key events
namespace Chip8Emulator
{
    public class Chip8
    {
        const uint kRegistersCount = 0x10;
        const uint kMemorySize = 0x1000;
        const uint kStackSize = 0x10;
        public const uint kKeysCount = 0x10;
        const ushort kProgramStart = 0x200;

        public const PixelFormat kPixelFormat = PixelFormat.Format24bppRgb;
        const int kPixelSize = 3;

        public const int kScaleFactor = 8;
        public const int kWidth = 64 * kScaleFactor;
        public const int kHeight = 32 * kScaleFactor;

        const uint kClockHZ = 60;
        const uint kClockRateMS = (uint)(1.0 / kClockHZ * 1000 + 0.5);

        readonly byte[] font = new byte[] {
            0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
            0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
            0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
            0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
            0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
            0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
            0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
            0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
            0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
            0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
            0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
            0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
            0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
            0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
            0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
            0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
        };

        private readonly struct Argument
        {
            readonly string name;
            readonly ushort mask;
            readonly ushort shift;

            public Argument(string name, ushort mask, ushort shift)
            {
                this.name = name;
                this.mask = mask;
                this.shift = shift;
            }

            public readonly string Name => name;
            public readonly ushort Mask => mask;
            public readonly ushort Shift => shift;
        }

        delegate void Routine(Chip8 chip8, ushort opcode, in Instruction instr);
        private readonly struct Instruction
        {
            readonly bool undefined = true;
            public bool Undefined { get { return undefined; } }

            readonly string name;
            readonly ushort mask;
            readonly ushort pattern;
            readonly Argument[] args;
            readonly Routine routine;

            public Instruction(string name, ushort mask, ushort pattern, Argument[] args, Routine routine)
            {
                undefined = false;

                this.name = name;
                this.mask = mask;
                this.pattern = pattern;
                this.args = args;

                this.routine = routine;
            }

            public readonly void Execute(Chip8 chip8, ushort opcode)
            {
                routine(chip8, opcode, this);
            }

            public readonly string Name => name;
            public readonly ushort Mask => mask;
            public readonly ushort Pattern => pattern;
            public readonly Argument Arg(int index) => args[index];
        }

        readonly Instruction[] opcodeToInstructionMappings = new Instruction[]
        {
            new("CLS",         0xFFFF, 0x00E0, Array.Empty<Argument>(),                                                             Clear),
            new("RET",         0xFFFF, 0x00EE, Array.Empty<Argument>(),                                                             Return),
            new("JMP_NNN",     0xF000, 0x1000, new Argument[] { new("NNN", 0x0FFF, 0) },                                            Jump_NNN),
            new("CALL_NNN",    0xF000, 0x2000, new Argument[] { new("NNN", 0x0FFF, 0) },                                            Call_NNN),
            new("SE_VX_NN",    0xF000, 0x3000, new Argument[] { new("VX",  0x0F00, 8), new("NN", 0x00FF, 0) },                      SkipEqual_VX_NN),
            new("SNE_VX_NN",   0xF000, 0x4000, new Argument[] { new("VX",  0x0F00, 8), new("NN", 0x00FF, 0) },                      SkipNotEqual_VX_NN),
            new("SE_VX_VY",    0xF000, 0x5000, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      SkipEqual_VX_VY),
            new("LD_VX_NN",    0xF000, 0x6000, new Argument[] { new("VX",  0x0F00, 8), new("NN", 0x00FF, 0) },                      Load_VX_NN),
            new("ADD_VX_NN",   0xF000, 0x7000, new Argument[] { new("VX",  0x0F00, 8), new("NN", 0x00FF, 0) },                      Add_VX_NN),
            new("LD_VX_VY",    0xF00F, 0x8000, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      Load_VX_VY),
            new("OR_VX_VY",    0xF00F, 0x8001, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      Or_VX_VY),
            new("AND_VX_VY",   0xF00F, 0x8002, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      And_VX_VY),
            new("XOR_VX_VY",   0xF00F, 0x8003, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      Xor_VX_VY),
            new("ADD_VX_VY",   0xF00F, 0x8004, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      Add_VX_VY),
            new("SUB_VX_VY",   0xF00F, 0x8005, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      SubtractBorrow_VX_VY),
            new("SHR_VX_{VY}", 0xF00F, 0x8006, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      ShiftRight_VX_VY),
            new("SUBN_VX_VY",  0xF00F, 0x8007, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      SubtractNoBorrow_VX_VY),
            new("SHL_VX_{VY}", 0xF00F, 0x800E, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      ShiftLeft_VX_VY),
            new("SNE_VX_VY",   0xF00F, 0x9000, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4) },                      SkipNotEqual_VX_VY),
            new("LD_I_NNN",    0xF000, 0xA000, new Argument[] { new("NNN", 0x0FFF, 0) },                                            Load_I_NNN),
            new("JMP_V0_NNN",  0xF000, 0xB000, new Argument[] { new("NNN", 0x0FFF, 0) },                                            Jump_V0_NNN),
            new("RND_VX_NN",   0xF000, 0xC000, new Argument[] { new("VX",  0x0F00, 8), new("NN", 0x00FF, 0) },                      Random_VX_NN),
            new("DRW_VX_VY_N", 0xF000, 0xD000, new Argument[] { new("VX",  0x0F00, 8), new("VY", 0x00F0, 4), new("N", 0x000F, 0) }, Draw_VX_VY_N),
            new("SKP_VX",      0xF0FF, 0xE09E, new Argument[] { new("VX",  0x0F00, 8) },                                            SkipKeyPressed_VX),
            new("SKNP_VX",     0xF0FF, 0xE0A1, new Argument[] { new("VX",  0x0F00, 8) },                                            SkipKeyNotPressed_VX),
            new("LD_VX_DT",    0xF0FF, 0xF007, new Argument[] { new("VX",  0x0F00, 8) },                                            Load_VX_DT),
            new("LD_VX_K",     0xF0FF, 0xF00A, new Argument[] { new("VX",  0x0F00, 8) },                                            Load_VX_K),
            new("LD_DT_VX",    0xF0FF, 0xF015, new Argument[] { new("VX",  0x0F00, 8) },                                            Load_DT_VX),
            new("LD_ST_VX",    0xF0FF, 0xF018, new Argument[] { new("VX",  0x0F00, 8) },                                            Load_ST_VX),
            new("ADD_I_VX",    0xF0FF, 0xF01E, new Argument[] { new("VX",  0x0F00, 8) },                                            Add_I_VX),
            new("LD_F_VX",     0xF0FF, 0xF029, new Argument[] { new("VX",  0x0F00, 8) },                                            LoadFont_VX),
            new("LD_B_VX",     0xF0FF, 0xF033, new Argument[] { new("VX",  0x0F00, 8) },                                            LoadBinaryCodedDecimal_VX),
            new("LD_[I]_VX",   0xF0FF, 0xF055, new Argument[] { new("VX",  0x0F00, 8) },                                            StoreRegistersToMemory),
            new("LD_VX_[I]",   0xF0FF, 0xF065, new Argument[] { new("VX",  0x0F00, 8) },                                            LoadRegistersFromMemory),
        };

        readonly byte[] memory = new byte[kMemorySize];
        readonly byte[] registers = new byte[kRegistersCount];
        readonly ushort[] stack = new ushort[kStackSize];
        public long[] keys = new long[kKeysCount];

        byte carryFlag = 0;
        byte DT = 0;
        byte ST = 0;
        ushort I = 0;
        ushort PC = kProgramStart;
        byte SP = 0;

        Bitmap frameBuffer;
        object frameBufferLock = new();

        readonly Display display;
        public event Action<Bitmap> UpdateScreen;

        long startTS = 0;
        static readonly Random random = new();

        enum EmulationState
        {
            Stopped = 0,
            Running,
            Exit,
        }

        long emulationState = (long)EmulationState.Stopped;
        object emulationLock = new();

        public Chip8(Display display)
        {
            this.display = display;

            display.OnROMLoad += LoadROM;
            display.OnScreenResize += ResizeDisplay;
            display.OnKeyPressed += OnKeyPressed;
            display.OnKeyReleased += OnKeyReleased;
            display.OnExit += Exit;
        }

        void Exit()
        {
            Interlocked.Exchange(ref emulationState, (long)EmulationState.Exit);
        }

        void LoadROM(byte[] data)
        {
            Interlocked.Exchange(ref emulationState, (long)EmulationState.Stopped);

            lock (emulationLock)
            {
                Array.Clear(memory, 0, memory.Length);
                Array.Clear(registers, 0, registers.Length);
                carryFlag = 0;
                DT = 0;
                ST = 0;
                I = 0;
                PC = kProgramStart;
                Array.Clear(stack, 0, stack.Length);
                SP = 0;

                Array.Clear(keys, 0, keys.Length);

                Array.Copy(font, memory, font.Length);
                Array.Copy(data, 0x0, memory, kProgramStart, data.Length);

                ClearScreen();
            }

            Interlocked.Exchange(ref emulationState, (long)EmulationState.Running);
        }

        public void ResizeDisplay(int width, int height)
        {
            lock (frameBufferLock) frameBuffer = new Bitmap(width, height, kPixelFormat);

            ClearScreen();
        }

        public void OnKeyPressed(byte key)
        {
            Interlocked.Exchange(ref keys[key], 1);
        }
        public void OnKeyReleased(byte key)
        {
            Interlocked.Exchange(ref keys[key], 0);
        }

        public void Loop()
        {
            do
            {
                long endTS = Stopwatch.GetTimestamp();
                if (Interlocked.Read(ref emulationState) == (long)EmulationState.Running)
                {
                    lock (emulationLock)
                    {
                        ushort opcode = Fetch();
                        Instruction instr = Decode(opcode);
                        Execute(opcode, instr);
                    }
                }

                TimeSpan elapsed = Stopwatch.GetElapsedTime(startTS, endTS);
                if (elapsed.TotalMilliseconds >= 1000.0 / 60.0)
                {
                    if (DT > 0)
                        DT--;

                    if (ST > 0)
                        ST--;

                    startTS = endTS;
                }

            } while (Interlocked.Read(ref emulationState) != (long)EmulationState.Exit);
        }

        ushort Fetch()
        {
            return (ushort)((memory[PC] << 8) | memory[PC + 1]);
        }

        Instruction Decode(ushort opcode)
        {
            Instruction instr = Array.Find(opcodeToInstructionMappings, instr => (opcode & instr.Mask) == instr.Pattern);
            if (instr.Undefined)
                throw new ArgumentOutOfRangeException(string.Format("Unknown opcode: {0:X}", opcode));
            return instr;
        }

        void Execute(ushort opcode, in Instruction instr)
        {
            instr.Execute(this, opcode);
        }

        static void DecodeArgs(in Instruction instr, ushort opcode, out ushort out0)
        {
            Argument arg0 = instr.Arg(0);
            out0 = (ushort)((opcode & arg0.Mask) >> arg0.Shift);
        }

        static void DecodeArgs(in Instruction instr, ushort opcode, out byte out0)
        {
            Argument arg0 = instr.Arg(0);
            out0 = (byte)((opcode & arg0.Mask) >> arg0.Shift);
        }

        static void DecodeArgs(in Instruction instr, ushort opcode, out byte out0, out byte out1)
        {
            Argument arg0 = instr.Arg(0);
            Argument arg1 = instr.Arg(1);

            out0 = (byte)((opcode & arg0.Mask) >> arg0.Shift);
            out1 = (byte)((opcode & arg1.Mask) >> arg1.Shift);
        }

        static void DecodeArgs(in Instruction instr, ushort opcode, out byte out0, out byte out1, out byte out2)
        {
            Argument arg0 = instr.Arg(0);
            Argument arg1 = instr.Arg(1);
            Argument arg2 = instr.Arg(2);

            out0 = (byte)((opcode & arg0.Mask) >> arg0.Shift);
            out1 = (byte)((opcode & arg1.Mask) >> arg1.Shift);
            out2 = (byte)((opcode & arg2.Mask) >> arg2.Shift);
        }

        unsafe void ClearScreen()
        {
            lock (frameBufferLock)
            {
                Rectangle viewport = new(0, 0, frameBuffer.Width, frameBuffer.Height);
                BitmapData data = frameBuffer.LockBits(viewport, ImageLockMode.WriteOnly, kPixelFormat);
                byte* pData = (byte*)data.Scan0;
                for (int i = 0; i < data.Height; i++)
                {
                    for (int j = 0; j < data.Stride; j++)
                    {
                        pData[j + data.Stride * i] = 0x0;
                    }
                }
                frameBuffer.UnlockBits(data);
            }

            UpdateScreen.Invoke(frameBuffer);
        }

        static void Clear(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            chip8.ClearScreen();
            chip8.PC += 2;
        }

        static void Return(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            chip8.PC = chip8.stack[--chip8.SP];
        }

        static void Jump_NNN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out ushort NNN);
            chip8.PC = NNN;
        }

        static void Call_NNN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            chip8.stack[chip8.SP++] = (ushort)(chip8.PC + 2);
            if (chip8.SP >= kStackSize)
                throw new StackOverflowException();

            DecodeArgs(instr, opcode, out ushort NNN);
            chip8.PC = NNN;
        }

        static void SkipEqual_VX_NN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte NN);
            chip8.PC += (ushort)(chip8.registers[VX] == NN ? 4 : 2);
        }

        static void SkipNotEqual_VX_NN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte NN);
            chip8.PC += (ushort)(chip8.registers[VX] != NN ? 4 : 2);
        }

        static void SkipEqual_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.PC += (ushort)(chip8.registers[VX] == chip8.registers[VY] ? 4 : 2);
        }

        static void Load_VX_NN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte NN);
            chip8.registers[VX] = NN;
            chip8.PC += 2;
        }

        static void Add_VX_NN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte NN);
            chip8.registers[VX] += NN;
            chip8.PC += 2;
        }

        static void Load_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.registers[VX] = chip8.registers[VY];
            chip8.PC += 2;
        }

        static void Or_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.registers[VX] |= chip8.registers[VY];
            chip8.PC += 2;
        }

        static void And_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.registers[VX] &= chip8.registers[VY];
            chip8.PC += 2;
        }

        static void Xor_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.registers[VX] ^= chip8.registers[VY];
            chip8.PC += 2;
        }

        static void Add_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.carryFlag = (byte)((0xFF - chip8.registers[VX]) < chip8.registers[VY] ? 1 : 0);
            chip8.registers[VX] += chip8.registers[VY];
            chip8.PC += 2;
        }

        static void SubtractBorrow_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.carryFlag = (byte)(chip8.registers[VX] > chip8.registers[VY] ? 1 : 0);
            chip8.registers[VX] -= chip8.registers[VY];
            chip8.PC += 2;
        }

        static void ShiftRight_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.carryFlag = (byte)(chip8.registers[VX] & 0x1);
            chip8.registers[VX] >>= 1;
            chip8.PC += 2;
        }

        static void SubtractNoBorrow_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.carryFlag = (byte)(chip8.registers[VX] < chip8.registers[VY] ? 1 : 0);
            chip8.registers[VX] = (byte)(chip8.registers[VY] - chip8.registers[VX]);
            chip8.PC += 2;
        }

        static void ShiftLeft_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.carryFlag = (byte)(chip8.registers[VX] & 0x80);
            chip8.registers[VX] <<= 1;
            chip8.PC += 2;
        }

        static void SkipNotEqual_VX_VY(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY);
            chip8.PC += (ushort)(chip8.registers[VX] != chip8.registers[VY] ? 4 : 2);
        }

        static void Load_I_NNN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out ushort NNN);
            chip8.I = NNN;
            chip8.PC += 2;
        }

        static void Jump_V0_NNN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out ushort NNN);
            chip8.PC = (ushort)(chip8.registers[0] + NNN);
        }

        static void Random_VX_NN(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte NN);
            chip8.registers[VX] = (byte)(random.Next(0x0, 0xFF) & NN);
            chip8.PC += 2;
        }

        // TODO: Adapt for different types of bitmaps
        unsafe void Draw(byte x, byte y, byte n)
        {
            lock (frameBufferLock)
            {
                Rectangle viewport = new(0, 0, frameBuffer.Width, frameBuffer.Height);
                BitmapData data = frameBuffer.LockBits(viewport, ImageLockMode.ReadWrite, kPixelFormat);
                byte* pData = (byte*)data.Scan0;

                int dx = (x * kScaleFactor) & (kWidth - 1);
                int dy = (y * kScaleFactor) & (kHeight - 1);

                for (int byteIndex = 0; byteIndex < n; byteIndex++, dy += kScaleFactor)
                {
                    if (dy >= frameBuffer.Height)
                        break;

                    byte mem = memory[I + byteIndex];
                    for (int bitIndex = 7; bitIndex >= 0; bitIndex--)
                    {
                        byte bit = (byte)((mem >> bitIndex) & 0x1);
                        int pixelShift = ((dx * kPixelSize) + ((7 - bitIndex) * kScaleFactor)) + data.Stride * dy;

                        if (bit == 0x1)
                        {
                            for (int i = 0; i < kScaleFactor; i++)
                            {
                                for (int j = 0; j < kScaleFactor; j++)
                                {
                                    int shift = j + data.Stride * i;
                                    int index = pixelShift + shift;
                                    ref byte pixel0 = ref pData[index];
                                    ref byte pixel1 = ref pData[index + 1];
                                    ref byte pixel2 = ref pData[index + 2];

                                    uint* pixel = (uint*)&pData[index];
                                    const uint onMask = 0xffffff;
                                    const uint offMask = 0xff000000;

                                    if ((*pixel & onMask) == onMask)
                                    {
                                        carryFlag = 0x1;
                                        *pixel &= offMask;
                                    }
                                    else if ((*pixel & offMask) == 0x0)
                                    {
                                        *pixel |= onMask;
                                    }
                                }
                            }
                        }
                    }
                }

                frameBuffer.UnlockBits(data);
            }

            UpdateScreen.Invoke(frameBuffer);
        }

        static void Draw_VX_VY_N(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX, out byte VY, out byte N);
            chip8.Draw(chip8.registers[VX], chip8.registers[VY], N);
            chip8.PC += 2;
        }

        static void SkipKeyPressed_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.PC += (ushort)(Interlocked.Read(ref chip8.keys[chip8.registers[VX]]) == 1 ? 4 : 2);
        }

        static void SkipKeyNotPressed_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.PC += (ushort)(Interlocked.Read(ref chip8.keys[chip8.registers[VX]]) == 0 ? 4 : 2);
        }

        static void Load_VX_DT(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.registers[VX] = chip8.DT;
            chip8.PC += 2;
        }

        static void Load_VX_K(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            // TODO: fixme
            DecodeArgs(instr, opcode, out byte VX);
            while (true)
            {
                for (int i = 0; i < kKeysCount; i++)
                {
                    if (Interlocked.CompareExchange(ref chip8.keys[i], 0, 1) == 1)
                    {
                        chip8.registers[VX] = (byte)i;
                        goto key_pressed;
                    }
                }
            }
        key_pressed:
            chip8.PC += 2;
        }

        static void Load_DT_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.DT = chip8.registers[VX];
            chip8.PC += 2;
        }

        static void Load_ST_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.ST = chip8.registers[VX];
            chip8.PC += 2;
        }

        static void Add_I_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.I += chip8.registers[VX];
            chip8.PC += 2;
        }

        static void LoadFont_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            chip8.I = (ushort)(chip8.registers[VX] * 0x5);
            chip8.PC += 2;
        }

        static void LoadBinaryCodedDecimal_VX(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            byte value = chip8.registers[VX];
            byte h = (byte)(value / 100);
            byte t = (byte)((value - h * 100) / 10);
            byte o = (byte)(value - h * 100 - t * 10);

            chip8.memory[chip8.I] = h;
            chip8.memory[chip8.I + 1] = t;
            chip8.memory[chip8.I + 2] = o;
            chip8.PC += 2;
        }

        static void StoreRegistersToMemory(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            for (int i = 0; i <= VX; i++)
                chip8.memory[chip8.I + i] = chip8.registers[i];
            chip8.PC += 2;
        }

        static void LoadRegistersFromMemory(Chip8 chip8, ushort opcode, in Instruction instr)
        {
            DecodeArgs(instr, opcode, out byte VX);
            for (int i = 0; i <= VX; i++)
                chip8.registers[i] = chip8.memory[chip8.I + i];
            chip8.PC += 2;
        }
    }
}
