import ctypes, ctypes.wintypes as w, sys

CLSCTX_ALL = 23
eRender, eConsole = 0, 1
class GUID(ctypes.Structure):
    _fields_ = [("Data1", ctypes.c_ulong), ("Data2", ctypes.c_ushort), ("Data3", ctypes.c_ushort), ("Data4", ctypes.c_ubyte*8)]
def g(s):
    h = s.strip("{}").split("-")
    raw = h[3] + h[4]
    d4 = [int(raw[i:i+2],16) for i in range(0,16,2)]
    return GUID(int(h[0],16), int(h[1],16), int(h[2],16), (ctypes.c_ubyte*8)(*[int(h[3][i:i+2],16) for i in (0,2)], *d4))
CLSID_MMDeviceEnumerator = g("{BCDE0395-E52F-467C-8E3D-C4579291692E}")
IID_IMMDeviceEnumerator  = g("{A95664D2-9614-4F35-A746-DE8DB63617E6}")
IID_IAudioEndpointVolume = g("{5CDF2C82-812E-4578-A3DC-B21FCD7011BB}")

ole32 = ctypes.oledll.ole32
ole32.CoInitializeEx(None, 0)
enum = ctypes.POINTER(ctypes.c_void_p)()
# vtable order for IMMDeviceEnumerator: (NotImpl1 is vtable[0]... careful: EnumAudioEndpoints is method 1)
# We build raw COM calls via ctypes.cast on vtables.
class COM(ctypes.c_void_p): pass

def vtbl(obj, index):
    return (ctypes.cast(obj, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents.contents.value) + index*ctypes.sizeof(ctypes.c_void_p)

def call(ptr, restype, *args):
    f = ctypes.WINFUNCTYPE(restype, ctypes.c_void_p, *([type(a) for a in args]))(ptr)
    return f(ctypes.cast(args[0] if False else obj, ctypes.c_void_p), *args) if False else None

# Simpler: use ctypes with explicit prototypes
obj = ctypes.c_void_p()
hr = ole32.CoCreateInstance(ctypes.byref(CLSID_MMDeviceEnumerator), None, CLSCTX_ALL, ctypes.byref(IID_IMMDeviceEnumerator), ctypes.byref(obj))
assert hr == 0, hex(hr)
# IMMDeviceEnumerator vtable: 0=EnumAudioEndpoints, 1=GetDefaultAudioEndpoint, ...
pVtbl = ctypes.cast(obj, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
GetDefault = ctypes.WINFUNCTYPE(ctypes.HRESULT, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_void_p))(pVtbl[1])
dev = ctypes.c_void_p()
hr = GetDefault(obj, eRender, eConsole, ctypes.byref(dev)); assert hr == 0, hex(hr)
pVtbl2 = ctypes.cast(dev, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
Activate = ctypes.WINFUNCTYPE(ctypes.HRESULT, ctypes.c_void_p, ctypes.POINTER(GUID), ctypes.c_ulong, ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p))(pVtbl2[3])
epv = ctypes.c_void_p()
hr = Activate(dev, ctypes.byref(IID_IAudioEndpointVolume), CLSCTX_ALL, None, ctypes.byref(epv)); assert hr == 0, hex(hr)
pVtbl3 = ctypes.cast(epv, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
# IAudioEndpointVolume: 0 RegisterCB, 1 UnregisterCB, 2 SetMasterVolumeLevel, 3 SetMasterVolumeLevelScalar,
# 4 GetMasterVolumeLevel, 5 GetMasterVolumeLevelScalar, 6 SetMute, 7 GetMute
SetMute = ctypes.WINFUNCTYPE(ctypes.HRESULT, ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p)(pVtbl3[6])
GetMute = ctypes.WINFUNCTYPE(ctypes.HRESULT, ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.c_void_p)(pVtbl3[7])
GetVol = ctypes.WINFUNCTYPE(ctypes.HRESULT, ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_void_p)(pVtbl3[5])
cmd = sys.argv[1] if len(sys.argv) > 1 else "get"
m = ctypes.c_int(0); v = ctypes.c_float(0)
GetMute(epv, ctypes.byref(m), None)
GetVol(epv, ctypes.byref(v), None)
if cmd == "mute": SetMute(epv, 1, None); print("muted")
elif cmd == "unmute": SetMute(epv, 0, None); print("unmuted")
elif cmd == "get": print(f"vol={v.value*100:.0f}% mute={m.value}")
ole32.CoUninitialize()
