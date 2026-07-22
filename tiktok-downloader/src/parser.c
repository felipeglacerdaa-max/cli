#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_string(const char *value)
{
    size_t len = strlen(value) + 1;
    char *result = (char *)malloc(len);
    if (result) {
        memcpy(result, value, len);
    }
    return result;
}

static int add_video_url(VideoList *out, const char *url)
{
    char *normalized = NULL;
    if (!url || !*url) {
        return 0;
    }

    normalized = dup_string(url);
    if (!normalized) {
        return 0;
    }

    size_t len = strlen(normalized);
    size_t out_len = 0;
    for (size_t i = 0; i < len; ++i) {
        if (normalized[i] == '\\' && i + 1 < len && normalized[i + 1] == '/') {
            normalized[out_len++] = '/';
            ++i;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u002F", 6) == 0) {
            normalized[out_len++] = '/';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u003A", 6) == 0) {
            normalized[out_len++] = ':';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u003D", 6) == 0) {
            normalized[out_len++] = '=';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u0026", 6) == 0) {
            normalized[out_len++] = '&';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u002E", 6) == 0) {
            normalized[out_len++] = '.';
            i += 5;
        } else if (normalized[i] == '\\' && i + 1 < len && normalized[i + 1] == 'u' && isxdigit((unsigned char)normalized[i + 2]) && isxdigit((unsigned char)normalized[i + 3]) && isxdigit((unsigned char)normalized[i + 4]) && isxdigit((unsigned char)normalized[i + 5])) {
            char hex[5] = {0};
            memcpy(hex, normalized + i + 2, 4);
            long code = strtol(hex, NULL, 16);
            normalized[out_len++] = (char)code;
            i += 5;
        } else {
            normalized[out_len++] = normalized[i];
        }
    }

    while (out_len > 0 && (normalized[out_len - 1] == ' ' || normalized[out_len - 1] == '\t' || normalized[out_len - 1] == '\r' ||
                           normalized[out_len - 1] == '\n' || normalized[out_len - 1] == ',' || normalized[out_len - 1] == '.' ||
                           normalized[out_len - 1] == ';' || normalized[out_len - 1] == ':' || normalized[out_len - 1] == '"' ||
                           normalized[out_len - 1] == '\'' || normalized[out_len - 1] == ')' || normalized[out_len - 1] == ']' ||
                           normalized[out_len - 1] == '}' || normalized[out_len - 1] == '>' || normalized[out_len - 1] == '/')) {
        normalized[--out_len] = '\0';
    }

    while (out_len > 0 && (normalized[0] == ' ' || normalized[0] == '\t' || normalized[0] == '\r' || normalized[0] == '\n' ||
                           normalized[0] == '"' || normalized[0] == '\'' || normalized[0] == '(' || normalized[0] == '[' ||
                           normalized[0] == '{' || normalized[0] == '<')) {
        memmove(normalized, normalized + 1, out_len);
        normalized[--out_len] = '\0';
    }

    for (int i = 0; i < out->count; ++i) {
        if (strcmp(out->items[i], normalized) == 0) {
            free(normalized);
            return 0;
        }
    }

    if (out->count >= out->capacity) {
        int new_capacity = out->capacity == 0 ? 8 : out->capacity * 2;
        char **new_items = (char **)realloc(out->items, sizeof(char *) * new_capacity);
        if (!new_items) {
            free(normalized);
            return 0;
        }

        out->items = new_items;
        out->capacity = new_capacity;
    }

    out->items[out->count++] = normalized;
    return 1;
}

static char *normalize_candidate(const char *value)
{
    if (!value || !*value) {
        return NULL;
    }

    size_t len = strlen(value);
    char *normalized = (char *)malloc(len + 1);
    if (!normalized) {
        return NULL;
    }

    size_t out = 0;
    for (size_t i = 0; i < len; ++i) {
        if (value[i] == '\\' && i + 1 < len && value[i + 1] == '/') {
            normalized[out++] = '/';
            ++i;
        } else if (value[i] == '\\' && i + 6 <= len && strncmp(value + i, "\\u002F", 6) == 0) {
            normalized[out++] = '/';
            i += 5;
        } else if (value[i] == '\\' && i + 6 <= len && strncmp(value + i, "\\u003A", 6) == 0) {
            normalized[out++] = ':';
            i += 5;
        } else if (value[i] == '\\' && i + 6 <= len && strncmp(value + i, "\\u003D", 6) == 0) {
            normalized[out++] = '=';
            i += 5;
        } else if (value[i] == '\\' && i + 6 <= len && strncmp(value + i, "\\u0026", 6) == 0) {
            normalized[out++] = '&';
            i += 5;
        } else if (value[i] == '\\' && i + 6 <= len && strncmp(value + i, "\\u002E", 6) == 0) {
            normalized[out++] = '.';
            i += 5;
        } else {
            normalized[out++] = value[i];
        }
    }

    while (out > 0 && (normalized[out - 1] == ' ' || normalized[out - 1] == '\t' || normalized[out - 1] == '\r' ||
                       normalized[out - 1] == '\n' || normalized[out - 1] == ',' || normalized[out - 1] == '.' ||
                       normalized[out - 1] == ';' || normalized[out - 1] == ':' || normalized[out - 1] == '"' ||
                       normalized[out - 1] == '\'' || normalized[out - 1] == ')' || normalized[out - 1] == ']' ||
                       normalized[out - 1] == '}' || normalized[out - 1] == '>' || normalized[out - 1] == '/')) {
        normalized[--out] = '\0';
    }

    while (out > 0 && (normalized[0] == ' ' || normalized[0] == '\t' || normalized[0] == '\r' || normalized[0] == '\n' ||
                       normalized[0] == '"' || normalized[0] == '\'' || normalized[0] == '(' || normalized[0] == '[' ||
                       normalized[0] == '{' || normalized[0] == '<')) {
        memmove(normalized, normalized + 1, out);
        normalized[--out] = '\0';
    }

    return normalized;
}

static int looks_like_video_url(const char *value)
{
    if (!value || !*value) {
        return 0;
    }

    char *normalized = dup_string(value);
    if (!normalized) {
        return 0;
    }

    size_t len = strlen(normalized);
    size_t out_len = 0;
    for (size_t i = 0; i < len; ++i) {
        if (normalized[i] == '\\' && i + 1 < len && normalized[i + 1] == '/') {
            normalized[out_len++] = '/';
            ++i;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u002F", 6) == 0) {
            normalized[out_len++] = '/';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u003A", 6) == 0) {
            normalized[out_len++] = ':';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u003D", 6) == 0) {
            normalized[out_len++] = '=';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u0026", 6) == 0) {
            normalized[out_len++] = '&';
            i += 5;
        } else if (normalized[i] == '\\' && i + 6 <= len && strncmp(normalized + i, "\\u002E", 6) == 0) {
            normalized[out_len++] = '.';
            i += 5;
        } else if (normalized[i] == '\\' && i + 1 < len && normalized[i + 1] == 'u' && isxdigit((unsigned char)normalized[i + 2]) && isxdigit((unsigned char)normalized[i + 3]) && isxdigit((unsigned char)normalized[i + 4]) && isxdigit((unsigned char)normalized[i + 5])) {
            char hex[5] = {0};
            memcpy(hex, normalized + i + 2, 4);
            long code = strtol(hex, NULL, 16);
            normalized[out_len++] = (char)code;
            i += 5;
        } else {
            normalized[out_len++] = normalized[i];
        }
    }

    while (out_len > 0 && (normalized[out_len - 1] == ' ' || normalized[out_len - 1] == '\t' || normalized[out_len - 1] == '\r' ||
                           normalized[out_len - 1] == '\n' || normalized[out_len - 1] == ',' || normalized[out_len - 1] == '.' ||
                           normalized[out_len - 1] == ';' || normalized[out_len - 1] == ':' || normalized[out_len - 1] == '"' ||
                           normalized[out_len - 1] == '\'' || normalized[out_len - 1] == ')' || normalized[out_len - 1] == ']' ||
                           normalized[out_len - 1] == '}' || normalized[out_len - 1] == '>' || normalized[out_len - 1] == '/')) {
        normalized[--out_len] = '\0';
    }

    while (out_len > 0 && (normalized[0] == ' ' || normalized[0] == '\t' || normalized[0] == '\r' || normalized[0] == '\n' ||
                           normalized[0] == '"' || normalized[0] == '\'' || normalized[0] == '(' || normalized[0] == '[' ||
                           normalized[0] == '{' || normalized[0] == '<')) {
        memmove(normalized, normalized + 1, out_len);
        normalized[--out_len] = '\0';
    }

    int result = 0;
    if (strstr(normalized, ".mp4") != NULL || strstr(normalized, ".m3u8") != NULL || strstr(normalized, "tiktokcdn") != NULL ||
        strstr(normalized, "/video/") != NULL || strstr(normalized, "video/tos") != NULL ||
        strstr(normalized, "www.tiktok.com/") != NULL || strstr(normalized, "tiktok.com/") != NULL ||
        strstr(normalized, "http://") != NULL || strstr(normalized, "https://") != NULL) {
        result = 1;
    }

    free(normalized);
    return result;
}

static void extract_http_links(const char *html, VideoList *out)
{
    const char *cursor = html;
    while (cursor && *cursor) {
        const char *start = NULL;
        if (strncmp(cursor, "http://", 7) == 0) {
            start = cursor;
        } else if (strncmp(cursor, "https://", 8) == 0) {
            start = cursor;
        } else if (strncmp(cursor, "http\\/\\/", 9) == 0) {
            start = cursor;
        } else if (strncmp(cursor, "https\\/\\/", 10) == 0) {
            start = cursor;
        } else if (strncmp(cursor, "http\\u003a\\u002f\\u002f", 24) == 0) {
            start = cursor;
        } else if (strncmp(cursor, "https\\u003a\\u002f\\u002f", 25) == 0) {
            start = cursor;
        }

        if (!start) {
            ++cursor;
            continue;
        }

        const char *end = start;
        while (*end && *end != '"' && *end != '\'' && *end != ' ' && *end != '<' && *end != '\n' && *end != '\r' &&
               *end != ',' && *end != ';' && *end != ')' && *end != ']' && *end != '}') {
            ++end;
        }

        if (end > start) {
            size_t len = (size_t)(end - start);
            char *candidate = (char *)malloc(len + 1);
            if (candidate) {
                memcpy(candidate, start, len);
                candidate[len] = '\0';
                if (looks_like_video_url(candidate)) {
                    add_video_url(out, candidate);
                }
                free(candidate);
            }
        }

        cursor = end;
    }
}

static void extract_quoted_strings(const char *html, VideoList *out)
{
    const char *cursor = html;
    while (cursor && *cursor) {
        if (*cursor == '"' || *cursor == '\'') {
            char quote = *cursor;
            const char *start = cursor + 1;
            const char *end = start;

            while (*end) {
                if (*end == '\\' && end[1] != '\0') {
                    end += 2;
                    continue;
                }

                if (*end == quote) {
                    break;
                }

                ++end;
            }

            if (*end == quote && end > start) {
                size_t len = (size_t)(end - start);
                char *value = (char *)malloc(len + 1);
                if (value) {
                    memcpy(value, start, len);
                    value[len] = '\0';
                    if (looks_like_video_url(value)) {
                        add_video_url(out, value);
                    }
                    free(value);
                }
            }

            cursor = end ? end + 1 : cursor + 1;
        } else {
            ++cursor;
        }
    }
}

static void extract_property_values(const char *html, const char *property, VideoList *out)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", property);
    const char *cursor = html;

    while ((cursor = strstr(cursor, pattern)) != NULL) {
        cursor += strlen(pattern);
        const char *end = cursor;
        while (*end && *end != '"') {
            ++end;
        }

        if (end > cursor) {
            size_t len = (size_t)(end - cursor);
            char *value = (char *)malloc(len + 1);
            if (value) {
                memcpy(value, cursor, len);
                value[len] = '\0';
                if (looks_like_video_url(value)) {
                    add_video_url(out, value);
                }
                free(value);
            }
        }

        cursor = end;
    }
}

void init_video_list(VideoList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void free_video_list(VideoList *list)
{
    if (!list) {
        return;
    }

    for (int i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int parse_html(const char *html, VideoList *out)
{
    if (!html || !out) {
        return 0;
    }

    if (!out->items && out->capacity == 0) {
        init_video_list(out);
    }

    extract_property_values(html, "playAddr", out);
    extract_property_values(html, "downloadAddr", out);
    extract_property_values(html, "videoUrl", out);
    extract_property_values(html, "video_url", out);
    extract_quoted_strings(html, out);
    extract_http_links(html, out);

    return out->count;
}
