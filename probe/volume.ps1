param([string]$Action, [int]$Percent = 0)
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
[Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IMMDevice { int Activate(ref Guid iid, int clsctx, IntPtr p, out IntPtr o); }
[Guid("5CDF2C82-812E-4578-A3DC-B21FCD7011BB"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IAudioEndpointVolume {
  int RegisterControlChangeCB(IntPtr c); int UnregisterControlChangeCB(IntPtr c);
  int SetMasterVolumeLevel(float l, Guid g); int SetMasterVolumeLevelScalar(float l, Guid g);
  int GetMasterVolumeLevel(out float l); int GetMasterVolumeLevelScalar(out float l);
  int SetMute(int m, Guid g); int GetMute(out int m);
}
[Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IMMDeviceEnumerator { int NotImpl1(); int GetDefaultAudioEndpoint(int d, int r, out IMMDevice e); }
[ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] class MMDeviceEnumeratorCom {}
public static class Vol {
  public static IAudioEndpointVolume Ep() {
    var en = (IMMDeviceEnumerator)(object)new MMDeviceEnumeratorCom();
    IMMDevice dev; en.GetDefaultAudioEndpoint(0, 1, out dev);
    Guid g = new Guid("5CDF2C82-812E-4578-A3DC-B21FCD7011BB");
    IntPtr p; dev.Activate(ref g, 23, IntPtr.Zero, out p);
    return (IAudioEndpointVolume)System.Runtime.InteropServices.Marshal.GetObjectForIUnknown(p);
  }
  public static float Get() { float v; Ep().GetMasterVolumeLevelScalar(out v); return v; }
  public static void Set(float v) { Ep().SetMasterVolumeLevelScalar(v, Guid.Empty); }
  public static void Mute(int m) { Ep().SetMute(m, Guid.Empty); }
}
"@
switch ($Action) {
  "get"   { $m = 0; [Vol]::Ep().GetMute([ref]$m); Write-Output ("vol=" + [math]::Round([Vol]::Get()*100) + "% mute=" + $m) }
  "set"   { [Vol]::Set([math]::Round($Percent/100.0,2)); Write-Output ("set to " + $Percent + "%") }
  "mute"  { [Vol]::Mute(1); Write-Output "muted" }
  "unmute"{ [Vol]::Mute(0); Write-Output "unmuted" }
}
