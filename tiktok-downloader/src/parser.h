#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char **items;
    int count;
    int capacity;
} VideoList;

void init_video_list(VideoList *list);
void free_video_list(VideoList *list);
int parse_html(const char *html, VideoList *out);

#endif
