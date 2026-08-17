#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "scoring.h"
#include "config.h"

static int current_score = 0;
static ScoreEntry scores[MAX_SCORES];
static int score_count = 0;

void scoring_init(void) {
    scoring_load();
}

void scoring_reset(void) {
    current_score = 0;
}

int scoring_get(void) {
    return current_score;
}

void scoring_increment(void) {
    current_score++;
}

void scoring_save(void) {
    FILE *f = fopen(SCORES_PATH, "w");
    if (!f) return;

    fprintf(f, "{\"scores\":[");
    int count = score_count < MAX_SCORES ? score_count : MAX_SCORES;
    for (int i = 0; i < count; i++) {
        if (i > 0) fprintf(f, ",");
        fprintf(f, "{\"initials\":\"%s\",\"score\":%d}", scores[i].initials, scores[i].score);
    }
    fprintf(f, "]}");
    fclose(f);
}

int scoring_load(void) {
    FILE *f = fopen(SCORES_PATH, "r");
    if (!f) {
        score_count = 0;
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        score_count = 0;
        return 0;
    }

    char *buf = (char *)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    score_count = 0;
    char *p = buf;
    while ((p = strstr(p, "\"initials\":")) != NULL && score_count < MAX_SCORES) {
        p += 11;
        while (*p == '"') p++;
        int len = 0;
        while (p[len] != '"' && len < 3) len++;
        memcpy(scores[score_count].initials, p, len);
        scores[score_count].initials[len] = '\0';
        p += len;

        p = strstr(p, "\"score\":");
        if (p) {
            p += 8;
            scores[score_count].score = atoi(p);
            score_count++;
        }
    }

    free(buf);
    return score_count;
}

int scoring_is_high_score(int score) {
    if (score <= 0) return 0;
    if (score_count < MAX_SCORES) return 1;
    return score > scores[score_count - 1].score;
}

int scoring_get_rank(int score) {
    for (int i = 0; i < score_count; i++) {
        if (score > scores[i].score) return i + 1;
    }
    return score_count + 1;
}
