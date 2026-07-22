#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/parser.h"

int main(void)
{
    const char *html =
        "<script>window.__INITIAL_STATE__=\"{\"videoUrl\":\"https://v16-web.tiktokcdn.com/video/tos/abc.mp4\","
        "\"mobileUrl\":\"https://www.tiktok.com/@user/video/1234567890\","
        "\"escaped\":\"https\\/\\/v16-web.tiktokcdn.com\\/video\\/tos\\/xyz.m3u8\"}</script>";

    VideoList videos;
    init_video_list(&videos);

    int found = parse_html(html, &videos);
    assert(found >= 3);

    int matched = 0;
    for (int i = 0; i < videos.count; ++i) {
        if (strstr(videos.items[i], "tiktokcdn.com") != NULL || strstr(videos.items[i], "/video/") != NULL) {
            matched++;
        }
    }

    assert(matched >= 3);

    free_video_list(&videos);
    puts("parser test passed");
    return 0;
}
