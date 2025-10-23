#pragma once
#ifndef CEZ_H_
#define CEZ_H_

#include "arena.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum cez_NodeType {
    CEZ_CARD,
    CEZ_CLOZE,
} cez_NodeType;

typedef struct cez_String {
    const char *str;
    const uint32_t len;
} cez_String;

typedef struct Card {
    char *front;
    char *back;
} Card;

typedef struct Cloze {
    char *review_text;
    char *view_text;
} Cloze;

typedef struct Flash Flash;

typedef struct Note {
    char *view_text;
    const char *review_text;
    const char *relative_path;
    size_t flash_count;
    size_t flash_capacity;
    Flash *flashes;
    bool occupied;
    uint64_t id;
    Arena arena;
} Note;

typedef struct Flash {
    cez_NodeType type;
    union {
        Card card;
        Cloze cloze;
    } data;
} Flash;

// TODO: (syd) support edges
typedef struct Edge {
    const Note *start;
    const Note *end;
} Edge ;

typedef enum LogLevel {
	INFO,
	WARNING,
	ERROR,
} LogLevel;

typedef struct Notes {
    const Note **notes;
    size_t note_count;
} Notes;

typedef struct NoteReview {
    uint64_t current_note;
    uint64_t current_flash;
    int32_t correct;
    int32_t incorrect;
    Note **notes;
    uint64_t note_count;
    bool showing_front;
} NoteReview;

typedef struct Graph Graph;

typedef struct Review Review;

bool cez_init(const char *dir);
const Graph *cez_load_graph(const char *dir);
void cez_shutdown();
// returns NULL if no note was found
const Note *get_note(const char *relative_path);
const Notes *get_notes();
const NoteReview *cez_load_review_from_array(size_t *array, size_t count, bool shuffle);
const Review *cez_load_review_from_node(const Note *parent);
const Review *cez_load_review_from_dir(const char *relative_path);
void cez_show_back();
void cez_review_correct_answer();
void cez_review_incorrect_answer();
bool cez_review_complete();
void cez_log(LogLevel level, const char *fmt, ...);

#endif // CEZ_H_
