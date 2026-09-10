# Measure a stream server's SUSTAINED production rate, independent of any consumer:
# connect, read continuously (socket buffer), discard first 10s, measure bytes/s over the rest.
import socket, ssl, urllib.parse, sys, time, re

def rate(url, window_s=80, warmup_s=10):
    p = urllib.parse.urlparse(url)
    port = p.port or (443 if p.scheme == 'https' else 80)
    path = p.path or '/'
    if p.query: path += '?' + p.query
    s = socket.create_connection((p.hostname, port), timeout=15)
    if p.scheme == 'https':
        s = ssl.create_default_context().wrap_socket(s, server_hostname=p.hostname)
    req = f"GET {path} HTTP/1.1\r\nHost: {p.hostname}\r\nUser-Agent: WinampMPEG/5.6\r\nIcy-MetaData: 1\r\nConnection: close\r\n\r\n"
    s.sendall(req.encode())
    buf = b''
    while b'\r\n\r\n' not in buf:
        c = s.recv(4096)
        if not c: break
        buf += c
    hdr_end = buf.find(b'\r\n\r\n') + 4
    headers = buf[:hdr_end].decode('latin1','replace')
    br = re.search(r'icy-br:\s*(\d+)', headers, re.I)
    nominal = int(br.group(1)) * 1000 if br else 0
    t0 = time.time(); total = len(buf) - hdr_end
    # warmup
    while time.time() - t0 < warmup_s:
        c = s.recv(65536)
        if not c: break
        total += len(c)
    t1 = time.time(); w0 = total
    ended = False
    while time.time() - t1 < window_s:
        s.settimeout(max(1.0, window_s - (time.time() - t1)))
        try:
            c = s.recv(65536)
        except socket.timeout:
            break
        if not c:
            ended = True; break
        total += len(c)
    dt = time.time() - t1
    got = (total - w0) / dt
    s.close()
    x = got / (nominal/8.0) * 100 if nominal else 0
    print(f"{url}\n  icy-br={nominal//1000}k measured={got:.0f} B/s over {dt:.0f}s => {got/nominal*100:.1f}% of nominal {'[CLOSED EARLY]' if ended else ''}")

for url in sys.argv[1:]:
    try: rate(url)
    except Exception as e: print(f"{url}\n  ERROR: {e}")
