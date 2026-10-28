# Helper for run-gui.cmd: waits for the "QEMU (Uppix)" window that WSLg opens, moves it to the
# top-left corner of the screen and brings it to the front (WSLg sometimes opens the window
# off-screen and behind other windows). The window grows when the guest changes its video
# mode; for the first 25 seconds it is placed again whenever it runs off the screen.
Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public class UppixWindow {
  public struct R { public int Left, Top, Right, Bottom; }
  public delegate bool CB(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] static extern bool EnumWindows(CB f, IntPtr l);
  [DllImport("user32.dll")] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr h);
  [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h, int k);
  [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int w, int h2, uint f);
  [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] static extern void keybd_event(byte k, byte s, uint f, UIntPtr e);
  public static IntPtr Find() {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, x) => {
      var s = new StringBuilder(256); GetWindowText(h, s, 256);
      if (IsWindowVisible(h) && s.ToString().Contains("QEMU (Uppix)")) { found = h; return false; }
      return true; }, IntPtr.Zero);
    return found;
  }
  public static void Place(IntPtr h, bool raise) {
    ShowWindow(h, 9);                                  // SW_RESTORE
    SetWindowPos(h, IntPtr.Zero, 0, 0, 0, 0, 0x0041);  // SWP_NOSIZE | SWP_SHOWWINDOW
    if (raise) {
      keybd_event(0x12, 0, 0, UIntPtr.Zero);           // Alt, so a background process may raise it
      SetForegroundWindow(h);
      keybd_event(0x12, 0, 2, UIntPtr.Zero);
    }
  }
}
"@
Add-Type -AssemblyName System.Windows.Forms
$screen = [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 120 -and $h -eq [IntPtr]::Zero; $i++) {
    $h = [UppixWindow]::Find()
    if ($h -eq [IntPtr]::Zero) { Start-Sleep -Milliseconds 500 }
}
if ($h -eq [IntPtr]::Zero) { exit 1 }
Start-Sleep -Milliseconds 500
[UppixWindow]::Place($h, $true)
$previous = ""
for ($i = 0; $i -lt 50 -and [UppixWindow]::IsWindow($h); $i++) {
    Start-Sleep -Milliseconds 500
    $r = New-Object UppixWindow+R
    [void][UppixWindow]::GetWindowRect($h, [ref]$r)
    $now = "$($r.Left),$($r.Top),$($r.Right),$($r.Bottom)"
    if ($now -ne $previous -and ($r.Left -ne 0 -or $r.Top -ne 0 -or $r.Right -gt $screen.Right -or $r.Bottom -gt $screen.Bottom)) {
        [UppixWindow]::Place($h, $now -ne $previous)
    }
    $previous = $now
}
