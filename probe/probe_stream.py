import sys, socket, ssl, urllib.parse, re

def parse_mp3_frames(data):
    # returns (version, layer, sample_rate, bitrates_seen, frames)
    bitrates_v1_l3 = [0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0]
    v2_rates = [22050,24000,16000]; v1_rates = [44100,48000,32000]
    stats = {}
    frames = 0
    sample_rates = set(); versions = set(); layers = set()
    i = 0; n = len(data)
    while i < n-4 and frames < 300:
        if data[i] == 0xFF and (data[i+1] & 0xE0) == 0xE0:
            b1 = data[i+1]; b2 = data[i+2]; b3 = data[i+3]
            version = (b1 >> 3) & 0x03   # 0=MPEG2.5, 2=MPEG2, 3=MPEG1
            layer = (b1 >> 1) & 0x03     # 1=Layer III
            br_idx = (b2 >> 4) & 0x0F
            sr_idx = (b2 >> 2) & 0x03
            if version in (0,2,3) and layer == 1 and br_idx not in (0,15) and sr_idx != 3:
                version_str = {0:'MPEG2.5',2:'MPEG2',3:'MPEG1'}[version]
                if version == 3: rate = v1_rates[sr_idx]; br = bitrates_v1_l3[br_idx]
                else:
                    br = [0,8,16,24,32,40,48,56,64,80,96,112,128,144,160][br_idx]
                    rate = v2_rates[sr_idx]
                versions.add(version_str); layers.add('Layer III'); sample_rates.add(rate)
                stats[br] = stats.get(br, 0) + 1
                frames += 1
                # frame length: Layer III MPEG1: 144*bitrate*1000/samplerate + padding
                pad = (b2 >> 1) & 0x01
                flen = (144 * br * 1000 // rate) + pad if version == 3 else (72 * br * 1000 // rate) + pad
                i += max(flen, 1)
                continue
        i += 1
    return versions, layers, sample_rates, stats, frames

def probe(url):
    p = urllib.parse.urlparse(url)
    port = p.port or (443 if p.scheme == 'https' else 80)
    path = p.path or '/'
    if p.query: path += '?' + p.query
    s = socket.create_connection((p.hostname, port), timeout=10)
    if p.scheme == 'https':
        s = ssl.create_default_context().wrap_socket(s, server_hostname=p.hostname)
    req = f"GET {path} HTTP/1.1\r\nHost: {p.hostname}\r\nUser-Agent: WinampMPEG/5.6\r\nIcy-MetaData: 1\r\nConnection: close\r\n\r\n"
    s.sendall(req.encode())
    # read headers
    buf = b''
    while b'\r\n\r\n' not in buf:
        chunk = s.recv(4096)
        if not chunk: break
        buf += chunk
    hdr_end = buf.find(b'\r\n\r\n') + 4
    headers = buf[:hdr_end].decode('latin1', 'replace')
    metaint = 0
    m = re.search(r'icy-metaint:\s*(\d+)', headers, re.I)
    if m: metaint = int(m.group(1))
    icy_name = re.search(r'icy-name:\s*(.*)', headers, re.I)
    ctype = re.search(r'content-type:\s*(.*)', headers, re.I)
    hls = 'm3u' in headers.lower() or 'x-icy' not in headers.lower() and metaint == 0
    # read audio
    audio = buf[hdr_end:]
    title = ''
    if metaint > 0:
        while len(audio) < metaint + 4096:
            chunk = s.recv(65536)
            if not chunk: break
            audio += chunk
        pos = metaint
        if len(audio) > pos:
            mlen = audio[pos] * 16
            meta = audio[pos+1:pos+1+mlen]
            tm = re.search(rb"StreamTitle='([^']*)'", meta)
            if tm: title = tm.group(1).decode('utf-8','replace')
    else:
        while len(audio) < 200000:
            chunk = s.recv(65536)
            if not chunk: break
            audio += chunk
    s.close()
    versions, layers, rates, brs, frames = parse_mp3_frames(audio)
    print(f"URL: {url}")
    print(f"  content-type: {ctype.group(1).strip() if ctype else '?'} | icy-metaint: {metaint} | icy-name: {icy_name.group(1).strip() if icy_name else '?'}")
    print(f"  format: {'/'.join(sorted(versions))} {'/'.join(sorted(layers))} | rates: {sorted(rates)} | frames parsed: {frames}")
    print(f"  bitrate histogram: {dict(sorted(brs.items()))} => {'CBR' if len(brs)==1 else 'VBR/mixed'}")
    print(f"  first StreamTitle: {title!r}")
    print()

for url in sys.argv[1:]:
    try:
        probe(url)
    except Exception as e:
        print(f"URL: {url}\n  ERROR: {e}\n")
