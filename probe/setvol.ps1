param([int]$Target = 50, [switch]$Get)
$sig = @'
using System;
using System.Runtime.InteropServices;
public class Vol {
  [DllImport("ole32.dll")] public static extern int CoInitializeEx(IntPtr p, uint mode);
  [DllImport("ole32.dll")] public static extern void CoUninitialize();
  [ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] class MMDeviceEnumerator {}
  [Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  public interface IMMDeviceEnumerator { int EnumAudioEndpoints(int d, int m, out IntPtr e); int GetDefaultAudioEndpoint(int d, int r, out IMMDevice e); }
  [Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  public interface IMMDevice { int Activate(ref Guid iid, int clsctx, IntPtr p, out IAudioEndpointVolume o); }
  [Guid("5CDF2C82-812E-4578-A3DC-B21FCD7011BB"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  public interface IAudioEndpointVolume {
    int RegisterCB(IntPtr c); int UnregisterCB(IntPtr c);
    int SetMasterVolumeLevel(float l, Guid g); int SetMasterVolumeLevelScalar(float l, Guid g);
    int GetMasterVolumeLevel(out float l); int GetMasterVolumeLevelScalar(out float l);
    int SetMute(int m, Guid g); int GetMute(out int m);
  }
  public static float Get() {
    var e = new MMDeviceEnumerator();
    IMMDevice dev; var t = e.GetType(); throw new Exception("not implemented via this path");
  }
}
'@
