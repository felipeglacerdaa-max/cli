#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "curl_utils.h"
#include "downloader.h"
#include "parser.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0777)
#endif

static void print_banner(void)
{
    printf("\n==================================================\n");
    printf("  TikTok Downloader by Gnomo\n");
    printf("  Discord: gnomo\n");
    printf("==================================================\n");
}

static void print_usage(const char *prog)
{
    printf("Uso:\n");
    printf("  %s <url> [quantidade]\n", prog);
    printf("  %s --limit <quantidade> <url>\n", prog);
    printf("\nExemplo:\n");
    printf("  %s https://www.tiktok.com/@usuario 10\n", prog);
}

static int ensure_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

#ifdef _WIN32
    return MKDIR(path) == 0;
#else
    return MKDIR(path) == 0;
#endif
}

static int parse_limit(const char *value, int *limit)
{
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed <= 0) {
        return 0;
    }

    *limit = (int)parsed;
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 1;
    }

    const char *target_url = argv[1];
    int limit = 5;

    if (argc >= 3) {
        if (strcmp(argv[2], "--limit") == 0 || strcmp(argv[2], "-n") == 0) {
            if (argc < 4 || !parse_limit(argv[3], &limit)) {
                fprintf(stderr, "Erro: limite inválido.\n");
                return 1;
            }
        } else if (!parse_limit(argv[2], &limit)) {
            fprintf(stderr, "Erro: quantidade inválida.\n");
            return 1;
        }
    }

    print_banner();
    printf("URL alvo: %s\n", target_url);
    printf("Quantidade: %d\n", limit);

    if (!ensure_directory("downloads")) {
        fprintf(stderr, "Erro: não foi possível criar a pasta downloads.\n");
        return 1;
    }

    printf("\nObtendo página...\n");
    char *page = fetch_page_to_string(target_url);
    if (!page) {
        fprintf(stderr, "Erro: não foi possível obter a página.\n");
        return 1;
    }

    VideoList videos;
    init_video_list(&videos);
    int found = parse_html(page, &videos);
    free(page);

    if (found <= 0) {
        printf("Nenhum vídeo encontrado. Tente outro link ou confira se a página expõe mídia diretamente.\n");
        free_video_list(&videos);
        return 1;
    }

    printf("\nEncontrados %d vídeos.\n", found);

    int count = found < limit ? found : limit;
    for (int i = 0; i < count; ++i) {
        char filename[256];
        snprintf(filename, sizeof(filename), "downloads/video_%02d.mp4", i + 1);
        printf("[%d/%d] Baixando %s\n", i + 1, count, filename);

        if (!download_file(videos.items[i], filename)) {
            fprintf(stderr, "Falha ao baixar: %s\n", videos.items[i]);
            continue;
        }

        printf("[OK] %s\n", filename);
    }

    printf("\nConcluído.\n");
    free_video_list(&videos);
    return 0;
}
