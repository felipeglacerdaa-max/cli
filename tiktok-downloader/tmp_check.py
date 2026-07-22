import urllib.request
url = 'https://www.tiktok.com/@velvet.rae.music'
req = urllib.request.Request(
    url,
    headers={
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
        'Accept-Language': 'pt-BR,pt;q=0.9,en-US;q=0.8,en;q=0.7',
        'Referer': 'https://www.tiktok.com/',
    },
)
with urllib.request.urlopen(req, timeout=20) as resp:
    body = resp.read(10000).decode('utf-8', errors='ignore')
    print('status', resp.status)
    print(body[:4000])
