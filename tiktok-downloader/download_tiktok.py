import argparse
import http.cookiejar
import json
import os
import re
import sys
import urllib.request

from curl_cffi import requests

try:
    from playwright.sync_api import sync_playwright
except Exception:  # pragma: no cover - optional dependency
    sync_playwright = None


class Colors:
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    RESET = "\033[0m"


def clear_screen():
    if os.name == "nt":
        os.system("cls")
    else:
        os.system("clear")


def banner():
    print(f"{Colors.CYAN}{Colors.BOLD}")
    print("==================================================")
    print("  TikTok Downloader by Gnomo")
    print("  Discord: gnomo")
    print("==================================================")
    print(f"{Colors.RESET}")


def show_main_menu(default_limit):
    print(f"{Colors.YELLOW}{Colors.BOLD}Painel principal{Colors.RESET}")
    print("\nEscolha uma opção:")
    print("  [1] Baixar vídeos")
    print("  [2] Definir quantidade padrão")
    print("  [3] Sair")
    print(f"\nQuantidade padrão atual: {default_limit}")


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def build_browser_headers():
    return {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8",
        "Accept-Language": "pt-BR,pt;q=0.9,en-US;q=0.8,en;q=0.7",
        "Accept-Encoding": "gzip, deflate, br",
        "Connection": "keep-alive",
        "Upgrade-Insecure-Requests": "1",
        "Sec-Fetch-Site": "none",
        "Sec-Fetch-Mode": "navigate",
        "Sec-Fetch-User": "?1",
        "Sec-Fetch-Dest": "document",
        "DNT": "1",
        "Referer": "https://www.tiktok.com/",
    }


def is_bot_blocked(html):
    if not html:
        return True
    lower = html.lower()
    markers = [
        "please wait",
        "challenge",
        "captcha",
        "robot",
        "verify your identity",
        "access denied",
        "suspicious activity",
    ]
    return any(marker in lower for marker in markers)


def fetch_page(url, retries=2):
    cookie_jar = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cookie_jar))
    opener.addheaders = [(key, value) for key, value in build_browser_headers().items()]

    try:
        opener.open(urllib.request.Request("https://www.tiktok.com/", headers=build_browser_headers()), timeout=20)
    except Exception:
        pass

    for attempt in range(retries):
        req = urllib.request.Request(url, headers=build_browser_headers())
        try:
            with opener.open(req, timeout=25) as resp:
                body = resp.read().decode("utf-8", errors="ignore")
                return body
        except Exception:
            if attempt == retries - 1:
                raise

    raise RuntimeError("Falha ao buscar a página")


def fetch_page_with_curl_cffi(url):
    resp = requests.get(
        url,
        headers=build_browser_headers(),
        impersonate="chrome124",
        timeout=25,
        allow_redirects=True,
    )
    return resp.text


def extract_urls_from_value(value):
    urls = []
    if not value:
        return urls

    if isinstance(value, str):
        urls.append(value)
        return urls

    if isinstance(value, dict):
        if "urlList" in value and isinstance(value["urlList"], list):
            for item in value["urlList"]:
                urls.extend(extract_urls_from_value(item))
        else:
            for key in ("uri", "url", "downloadAddr", "playAddr", "downloadUrl"):
                if key in value:
                    urls.extend(extract_urls_from_value(value[key]))
        return urls

    if isinstance(value, list):
        for item in value:
            urls.extend(extract_urls_from_value(item))
        return urls

    return urls


def looks_like_video_url(candidate):
    value = normalize_candidate(candidate).lower()
    return any(token in value for token in [".mp4", ".m3u8", "tiktokcdn", "video/tos"])


def gather_video_urls_from_item(item, seen):
    urls = []
    video = item.get("video") or {}
    for key in ("playAddr", "downloadAddr", "downloadUrl", "videoUrl", "downloadUrl", "downloadAddr"):
        for url in extract_urls_from_value(video.get(key)):
            if looks_like_video_url(url) and url not in seen:
                seen.add(url)
                urls.append(normalize_candidate(url))

    for url in extract_urls_from_value(item.get("shareUrl")):
        if looks_like_video_url(url) and url not in seen:
            seen.add(url)
            urls.append(normalize_candidate(url))

    return urls


def extract_profile_videos_from_api(url):
    match = re.search(r"https?://www\.tiktok\.com/@([^/]+)", url)
    if not match:
        return []

    username = match.group(1)
    api_urls = [
        f"https://www.tiktok.com/api/user/detail/?uniqueId={username}&__a=1",
        f"https://www.tiktok.com/api/user/detail/?uniqueId={username}&__a=1&__d=dis",
    ]

    for api_url in api_urls:
        try:
            body = fetch_page_with_curl_cffi(api_url)
            if not body or body.startswith("<"):
                continue

            data = json.loads(body)
            user = data.get("userInfo", {}).get("user") or {}
            sec_uid = user.get("secUid")
            if not sec_uid:
                continue

            cursor = 0
            video_urls = []
            seen = set()
            for _ in range(4):
                item_list_url = (
                    f"https://www.tiktok.com/api/post/item_list/?secUid={sec_uid}&count=30&cursor={cursor}&aid=1988&__a=1"
                )
                item_body = fetch_page_with_curl_cffi(item_list_url)
                if not item_body or item_body.startswith("<"):
                    break

                item_data = json.loads(item_body)
                items = item_data.get("itemList") or item_data.get("aweme_list") or []
                if not items:
                    break

                for item in items:
                    video_urls.extend(gather_video_urls_from_item(item, seen))

                cursor = item_data.get("cursor") or item_data.get("maxCursor") or 0
                if not cursor:
                    break

            if video_urls:
                return video_urls
        except Exception:
            continue

    return []


def extract_profile_videos_with_playwright(url, max_videos=40):
    if sync_playwright is None:
        return []

    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            context = browser.new_context(
                viewport={"width": 1440, "height": 900},
                user_agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
            )
            page = context.new_page()
            page.goto(url, wait_until="networkidle", timeout=60000)
            for _ in range(10):
                page.mouse.wheel(0, 2400)
                page.wait_for_timeout(1200)

            profile_html = page.content()
            direct_urls = extract_video_urls(profile_html)
            page_urls = page.evaluate(
                "() => Array.from(document.querySelectorAll('a')).map(a => a.href).filter(h => h && h.includes('/video/'))"
            )

            seen = set(direct_urls)
            for video_page in page_urls:
                if len(direct_urls) >= max_videos:
                    break
                if video_page in seen:
                    continue
                try:
                    page.goto(video_page, wait_until="networkidle", timeout=60000)
                    page.wait_for_timeout(3000)
                    video_srcs = page.evaluate(
                        "() => Array.from(document.querySelectorAll('video')).map(v => v.src || v.getAttribute('src')).filter(Boolean)"
                    )
                    for src in video_srcs:
                        if src and looks_like_video_url(src) and src not in seen:
                            seen.add(src)
                            direct_urls.append(src)
                        if len(direct_urls) >= max_videos:
                            break
                except Exception:
                    continue

            browser.close()
            return direct_urls
    except Exception:
        return []


def get_profile_video_urls(url, min_count=8):
    seen = set()
    urls = []

    try:
        html = fetch_page(url)
        if html and not is_bot_blocked(html):
            for candidate in extract_video_urls(html):
                if candidate not in seen:
                    seen.add(candidate)
                    urls.append(candidate)
    except Exception:
        pass

    api_urls = extract_profile_videos_from_api(url)
    for candidate in api_urls:
        if candidate not in seen:
            seen.add(candidate)
            urls.append(candidate)

    if len(urls) < min_count:
        playwright_urls = extract_profile_videos_with_playwright(url, max_videos=max(20, min_count * 2))
        for candidate in playwright_urls:
            if candidate not in seen:
                seen.add(candidate)
                urls.append(candidate)

    return urls


def normalize_candidate(candidate):
    if not candidate:
        return ""

    value = candidate.strip()
    value = value.strip(" \t\r\n,.;:)'\"")
    value = value.replace("\\/", "/")
    value = value.replace("\\u002F", "/")
    value = value.replace("\\u003A", ":")
    value = value.replace("\\u002E", ".")
    value = value.replace("\\u0026", "&")
    value = value.replace("\\u003D", "=")
    value = value.replace("\\u003C", "<")
    value = value.replace("\\u003E", ">")
    value = value.rstrip(" \t\r\n,.;:)'\"")
    if value.startswith("http\\/\\/"):
        value = value.replace("http\\/\\/", "http://", 1)
    if value.startswith("https\\/\\/"):
        value = value.replace("https\\/\\/", "https://", 1)
    return value


def looks_like_video_url(candidate):
    value = normalize_candidate(candidate).lower()
    return any(token in value for token in [".mp4", ".m3u8", "tiktokcdn", "/video/", "video/tos"])


def extract_video_urls(html):
    urls = []
    seen = set()

    patterns = [
        r"https?://[^\s\"'<>]+\.mp4[^\s\"'<>]*",
        r"https?://[^\s\"'<>]+\.m3u8[^\s\"'<>]*",
        r"https?://[^\s\"'<>]*tiktokcdn[^\s\"'<>]*",
        r"https?://[^\s\"'<>]*video/tos[^\s\"'<>]*",
        r"https?:[\\/]{2}[^\s\"'<>]+",
    ]

    for pattern in patterns:
        for match in re.finditer(pattern, html, re.IGNORECASE):
            candidate = normalize_candidate(match.group(0))
            if looks_like_video_url(candidate) and candidate not in seen:
                seen.add(candidate)
                urls.append(candidate)

    for match in re.finditer(r"[\"'](https?:[\\/]{2}[^\"']+)[\"']", html):
        candidate = normalize_candidate(match.group(1))
        if looks_like_video_url(candidate) and candidate not in seen:
            seen.add(candidate)
            urls.append(candidate)

    if not urls:
        for match in re.finditer(r"https?:[\\/]{2}[^\s\"'<>]+", html):
            candidate = normalize_candidate(match.group(0))
            if looks_like_video_url(candidate) and candidate not in seen:
                seen.add(candidate)
                urls.append(candidate)

    return urls


def download_file(url, filename):
    req = urllib.request.Request(url, headers=build_browser_headers())
    with urllib.request.urlopen(req, timeout=60) as resp:
        content_type = resp.getheader("Content-Type", "")
        data = resp.read()

    if not is_media_response(content_type, data, len(data)):
        raise RuntimeError(f"Resposta não é um vídeo válido: {content_type}")

    with open(filename, "wb") as fh:
        fh.write(data)
    return True


def run_download_flow(url, quantity):
    ensure_dir("downloads")
    print(f"\n{Colors.CYAN}Iniciando download...{Colors.RESET}")
    print(f"{Colors.YELLOW}URL:{Colors.RESET} {url}")
    print(f"{Colors.YELLOW}Quantidade:{Colors.RESET} {quantity}")

    try:
        html = fetch_page(url)
    except Exception as exc:
        print(f"{Colors.RED}Erro ao obter a página:{Colors.RESET} {exc}")
        html = ""

    urls = get_profile_video_urls(url)
    if not urls:
        print(f"{Colors.RED}A página do TikTok respondeu com proteção anti-bot ou não expôs vídeos na resposta HTML.{Colors.RESET}")
        return False
    print(f"{Colors.CYAN}Encontrados {len(urls)} URLs possíveis.{Colors.RESET}")

    count = min(quantity, len(urls)) if urls else 0
    if count == 0:
        print(f"{Colors.RED}Nenhum vídeo encontrado.{Colors.RESET}")
        return False

    for idx in range(count):
        video_url = urls[idx]
        filename = os.path.join("downloads", f"video_{idx + 1:02d}.mp4")
        print(f"{Colors.GREEN}[{idx + 1}/{count}] Baixando...{Colors.RESET}")
        try:
            download_file(video_url, filename)
            print(f"{Colors.GREEN}[OK] {filename}{Colors.RESET}")
        except Exception as exc:
            print(f"{Colors.RED}[FALHA] {exc}{Colors.RESET}")

    print(f"{Colors.CYAN}\nDownload concluído. Voltando ao painel principal...{Colors.RESET}")
    return True


def main():
    parser = argparse.ArgumentParser(description="Baixa vídeos de uma página do TikTok")
    parser.add_argument("url", nargs="?", help="URL da página do TikTok")
    parser.add_argument("quantidade", nargs="?", type=int, help="Quantidade de vídeos para baixar")
    args = parser.parse_args()

    default_limit = args.quantidade if args.quantidade is not None else 5

    while True:
        clear_screen()
        banner()
        show_main_menu(default_limit)

        choice = input("\nDigite a opção: ").strip()

        if choice == "1":
            target_url = args.url
            if not target_url:
                target_url = input("Informe a URL da página do TikTok: ").strip()
            quantity = default_limit
            if not args.quantidade:
                quantity_input = input(f"Quantidade de vídeos [{default_limit}]: ").strip()
                if quantity_input:
                    try:
                        quantity = int(quantity_input)
                    except ValueError:
                        print(f"{Colors.RED}Valor inválido. Usando o padrão.{Colors.RESET}")
                        quantity = default_limit
            run_download_flow(target_url, quantity)
            input("\nPressione Enter para continuar...")

        elif choice == "2":
            new_limit = input(f"Nova quantidade padrão [{default_limit}]: ").strip()
            if new_limit:
                try:
                    default_limit = int(new_limit)
                    print(f"{Colors.GREEN}Quantidade padrão atualizada.{Colors.RESET}")
                except ValueError:
                    print(f"{Colors.RED}Valor inválido.{Colors.RESET}")
            input("\nPressione Enter para continuar...")

        elif choice == "3":
            print(f"{Colors.CYAN}Encerrando...{Colors.RESET}")
            break

        else:
            print(f"{Colors.RED}Opção inválida.{Colors.RESET}")
            input("\nPressione Enter para continuar...")


if __name__ == "__main__":
    main()
