using System.ComponentModel;

namespace Chip8Emulator
{
    public partial class Display : Form
    {
        Chip8 chip8;
        Bitmap backBuffer;

        public event Action<byte[]> OnROMLoad;
        public event Action<int, int> OnScreenResize;
        public event Action<byte> OnKeyPressed;
        public event Action<byte> OnKeyReleased;

        public delegate void Void();
        public event Void OnExit;

        readonly Thread emulationThread;

        readonly Dictionary<Keys, byte> keymap = new()
        {
            { Keys.D1, 0x0 },
            { Keys.D2, 0x1 },
            { Keys.D3, 0x2 },
            { Keys.D4, 0x3 },
            { Keys.Q, 0x4 },
            { Keys.W, 0x5 },
            { Keys.E, 0x6 },
            { Keys.R, 0x7 },
            { Keys.A, 0x8 },
            { Keys.S, 0x9 },
            { Keys.D, 0xA },
            { Keys.F, 0xB },
            { Keys.Z, 0xC },
            { Keys.X, 0xD },
            { Keys.C, 0xE },
            { Keys.V, 0xF },
        };

        public Display()
        {
            InitializeComponent();

            chip8 = new Chip8(this);
            chip8.UpdateScreen += UpdateScreen;

            emulationThread = new(chip8.Loop);
            emulationThread.Name = "Emulation Thread";
            emulationThread.Start();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            OnExit.Invoke();
            emulationThread.Join();

            base.OnClosing(e);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);

            e.Graphics.Clear(Color.Black);
            if (backBuffer != null)
                e.Graphics.DrawImage(backBuffer, 0, menuStrip1.Height);
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);

            ClientSize = new Size(Chip8.kWidth, Chip8.kHeight + menuStrip1.Height);
            OnScreenResize.Invoke(ClientSize.Width, ClientSize.Height - menuStrip1.Height);
        }

        private void OpenROM(object sender, EventArgs e)
        {
            OpenFileDialog fileDialog = new OpenFileDialog();
            DialogResult result = fileDialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                byte[] data = File.ReadAllBytes(fileDialog.FileName);
                OnROMLoad.Invoke(data);
            }
        }

        unsafe void UpdateScreen(Bitmap bitmap)
        {
            Bitmap copy = new(Chip8.kWidth, Chip8.kHeight, Chip8.kPixelFormat);
            using (Graphics graphics = Graphics.FromImage(copy))
            {
                Rectangle imageRectangle = new(0, 0, copy.Width, copy.Height);
                graphics.DrawImage(bitmap, imageRectangle, imageRectangle, GraphicsUnit.Pixel);
            }
            backBuffer = copy;
            BeginInvoke(Invalidate);
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            if (keymap.TryGetValue(e.KeyCode, out byte key))
                OnKeyPressed.Invoke(key);
        }

        protected override void OnKeyUp(KeyEventArgs e)
        {
            base.OnKeyUp(e);
            if (keymap.TryGetValue(e.KeyCode, out byte key))
                OnKeyReleased.Invoke(key);
        }
    }
}