namespace Chip8Emulator
{
    // TODO: Add graphics backend (D3D11, D3D12)
    // TODO: Add sound backend (XAudio)
    // TODO: Add message posting between threads
    // TODO: Fix key presses
    internal static class Program
    {
        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();

            Display display = new();

            //nint handle = form.Handle;
            Application.Run(display);
        }
    }
}