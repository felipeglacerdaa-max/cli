#include "downloader.h"

#include <curl/curl.h>
#include <stdio.h>

int download_file(const char *url, const char *filename)
{
    CURL *curl = curl_easy_init();
    FILE *fp = NULL;

    if (!curl) {
        return 0;
    }

    fp = fopen(filename, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}
