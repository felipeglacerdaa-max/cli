#include "curl_utils.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MemoryChunk {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
    struct MemoryChunk *chunk = (struct MemoryChunk *)userp;

    char *ptr = (char *)realloc(chunk->memory, chunk->size + real_size + 1);
    if (!ptr) {
        return 0;
    }

    chunk->memory = ptr;
    memcpy(&(chunk->memory[chunk->size]), contents, real_size);
    chunk->size += real_size;
    chunk->memory[chunk->size] = '\0';

    return real_size;
}

char *fetch_page_to_string(const char *url)
{
    CURL *curl = curl_easy_init();
    struct MemoryChunk chunk;
    char *result = NULL;

    if (!curl) {
        return NULL;
    }

    chunk.memory = (char *)malloc(1);
    chunk.size = 0;
    if (!chunk.memory) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8");
    headers = curl_slist_append(headers, "Accept-Language: pt-BR,pt;q=0.9,en-US;q=0.8,en;q=0.7");
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
    headers = curl_slist_append(headers, "DNT: 1");
    headers = curl_slist_append(headers, "Referer: https://www.tiktok.com/");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");
    curl_easy_setopt(curl, CURLOPT_REFERER, "https://www.tiktok.com/");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(chunk.memory);
        return NULL;
    }

    result = chunk.memory;
    return result;
}
