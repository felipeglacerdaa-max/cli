import os
import sys
sys.path.append(os.path.abspath('.'))
from download_tiktok import extract_profile_videos_from_api, extract_profile_videos_with_playwright, extract_video_urls, fetch_page, is_bot_blocked

url = 'https://www.tiktok.com/@charlidamelio'
print('URL:', url)
try:
    html = fetch_page(url)
    print('HTML len:', len(html), 'blocked:', is_bot_blocked(html))
except Exception as exc:
    print('fetch_page error:', exc)
    html = ''

if html:
    urls = extract_video_urls(html)
    print('extract_video_urls count:', len(urls))
    for u in urls[:5]:
        print('URL:', u)

api_urls = extract_profile_videos_from_api(url)
print('api count:', len(api_urls))
for u in api_urls[:5]:
    print('API:', u)

if extract_profile_videos_with_playwright:
    try:
        pw_urls = extract_profile_videos_with_playwright(url)
        print('playwright count:', len(pw_urls))
        for u in pw_urls[:5]:
            print('PLAY:', u)
    except Exception as exc:
        print('playwright error:', exc)
else:
    print('playwright not installed')
