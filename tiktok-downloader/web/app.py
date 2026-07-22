import os
import re
import sys
import threading
import urllib.parse
import urllib.request
from flask import Flask, render_template_string, request, send_from_directory, jsonify

sys.path.append(os.path.dirname(os.path.dirname(__file__)))
from download_tiktok import extract_profile_videos_from_api, extract_profile_videos_with_playwright, extract_video_urls, fetch_page, is_bot_blocked, download_file

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = os.path.join(os.path.dirname(__file__), '..', 'downloads')
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

HTML = """
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <title>TikTok Downloader</title>
    <style>
      body { font-family: Arial, sans-serif; max-width: 800px; margin: 40px auto; padding: 20px; }
      input, button { padding: 10px; font-size: 16px; }
      input { width: 70%; }
      button { width: 25%; margin-left: 10px; }
      .result { margin-top: 20px; padding: 15px; background: #f5f5f5; border-radius: 6px; }
      .warning { color: #b22222; }
      .ok { color: #1f7a1f; }
      .links { word-break: break-all; }
    </style>
  </head>
  <body>
    <h1>TikTok Downloader</h1>
    <p>Coloque a URL do perfil do TikTok e o sistema tentará encontrar vídeos para baixar.</p>
    <form id="form">
      <input type="text" name="url" placeholder="https://www.tiktok.com/@usuario" required />
      <button type="submit">Buscar</button>
    </form>
    <div id="result" class="result">Aguardando...</div>
    <script>
      document.getElementById('form').addEventListener('submit', async function (e) {
        e.preventDefault();
        const result = document.getElementById('result');
        const url = this.url.value;
        result.innerHTML = 'Buscando...';
        const response = await fetch('/api/download', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ url }) });
        const data = await response.json();
        result.innerHTML = data.ok ? '<div class="ok">Encontrados ' + data.count + ' vídeos.</div>' + '<ul>' + data.links.map(l => '<li class="links"><a href="/download/' + encodeURIComponent(l.name) + '" target="_blank">' + l.name + '</a></li>').join('') + '</ul>' : '<div class="warning">' + data.error + '</div>';
      });
    </script>
  </body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML)

@app.route('/api/download', methods=['POST'])
def api_download():
    data = request.get_json(silent=True) or {}
    url = (data.get('url') or '').strip()
    if not url:
        return jsonify({'ok': False, 'error': 'URL vazia.'})

    try:
        html = fetch_page(url)
    except Exception as exc:
        html = ''

    urls = []
    if html and not is_bot_blocked(html):
        urls = extract_video_urls(html)

    if not urls:
        api_urls = extract_profile_videos_from_api(url)
        if api_urls:
            urls = api_urls
        else:
            playwright_urls = extract_profile_videos_with_playwright(url)
            if playwright_urls:
                urls = playwright_urls

    if not urls:
        return jsonify({'ok': False, 'error': 'Nenhum vídeo encontrado. O TikTok pode estar bloqueando a página.'})

    downloads = []
    for idx, video_url in enumerate(urls[:5], start=1):
        filename = f'video_{idx:02d}.mp4'
        path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        try:
            download_file(video_url, path)
            downloads.append({'name': filename, 'path': path})
        except Exception:
            continue

    if not downloads:
        return jsonify({'ok': False, 'error': 'Os vídeos foram encontrados, mas o download falhou.'})

    return jsonify({'ok': True, 'count': len(downloads), 'links': [{'name': item['name']} for item in downloads]})

@app.route('/download/<path:filename>')
def download_file(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename, as_attachment=True)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
