#include "cez.hpp"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#include <xxh3.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define START_CAPACITY 128
#define START_FLASH_CAPACITY 8
#define READ_BUFF_CAP 1024

// TODO: (syd) make the organization of this file not shit
// TODO: (syd) maybe don't have the API caller store the actual pointer to the
// data, keep it here so you can shutdown...

static Arena graph_arena = {0};
// buffer for creating absolute path from relative
static char absolute_buffer[512];
static Notes notes = {0};

// data handling

typedef struct KV {
    char *key;
    uint64_t value;
} KV;

typedef struct HashMap {
    KV *items;
    size_t count;
    size_t capacity;
} HashMap;

// hidden crap

typedef struct Review {
    Flash *nodes;
    size_t current;
} Review;

typedef struct Graph {
    Edge *edges;
    size_t edge_capacity;
    size_t edge_count;
    Note **notes;
    size_t note_capacity;
    size_t note_count;
    const char *base_dir;
    HashMap map;
} Graph;

#define return_defer(value) do { result = (value); goto defer; } while(0)

static Graph graph = {};
static NoteReview note_review = {0};

// - - - hidden - - -

bool read_file(const char *abs_path, Graph *graph);
bool recursive_file_load(const char *abs_path, Graph *graph);

char *make_str(Arena *arena, const char *str);
const char *get_absolute(const char *relative_path, const Graph *graph);
const char *get_relative(const char *abs_path, const Graph *graph);
void realloc_flashes(Note *note);
void delete_note(Graph *graph, Note *note);
void delete_note_by_id(Graph *graph, uint64_t id);
bool is_markdown(const char *file_name);
uint64_t make_id(const char *relative_path);
const Note *get_note(const char *relative_path);
// ensure both strings have the same strlen
void make_review_text(char *review_text, char *view_text);

// - - - top level API implementation - - -

const Graph *cez_load_graph(const char *dir) {
    graph.base_dir = make_str(&graph_arena, dir);
    graph.notes = (Note **)arena_alloc(&graph_arena, sizeof(Note) * START_CAPACITY);
    graph.note_count = 0;
    graph.note_capacity = START_CAPACITY;
    graph.edges = (Edge *)arena_alloc(&graph_arena, sizeof(Edge) * START_CAPACITY);
    graph.edge_capacity = START_CAPACITY;

    note_review.notes = NULL;
    /*
    if (!note_review.notes) {
        cez_log(ERROR, "Failed to alloc note_review.notes: %d", __LINE__);
        return NULL;
    }
    */

    recursive_file_load(dir, &graph);

    return &graph;
};

const Note *get_note(const char *relative_path) {
    for (size_t i = 0; i < graph.note_count; i++) {
        if (strcmp(relative_path, graph.notes[i]->relative_path) == 0) {
            return (const Note *)&graph.notes[i];
        }
    }
    return NULL;
}

void shuffle_notes(Note **notes, uint64_t len) {
    if (len < 1) return;
    Note *tmp = NULL;
    uint64_t b = 0;
    for (uint64_t i = len-1; i > 1; i--) {
        b = rand()%(i-1);
        tmp = notes[b];
        cez_log(INFO, "Swapping: %lu, %lu", b, i);

        notes[b] = notes[i];
        notes[i] = tmp;
    }
}

void shuffle_flashes(Flash *flashes, uint64_t len) {
    if (len < 1) return;
    Flash tmp = {};
    uint64_t b = 0;
    cez_log(INFO, "shuffling: %lu", len);
    for (uint64_t i = len-1; i > 1; i--) {
        b = rand()%(i-1);
        tmp = flashes[b];
        cez_log(INFO, "Swapping: %lu, %lu", b, i);

        flashes[b] = flashes[i];
        flashes[i] = tmp;
    }
}

const NoteReview *cez_load_review_from_array(size_t *array, size_t count, bool shuffle) {
    if (count < 1) {
        return NULL;
    }
    void *alloc = realloc(note_review.notes, sizeof(Note*) * count);
    if (!alloc) {
        cez_log(ERROR, "failed memory allocation in cez_load_review_from_array");
        return NULL;
    }
    note_review.notes = (Note**)alloc;
    for (size_t i = 0; i < count; i++) {
        memcpy(&note_review.notes[i], &graph.notes[array[i]], sizeof(Note*));
    }

    if (shuffle) {
        for (uint64_t i = 0; i < count; i++) {
            cez_log(INFO, "SHUFFLING");
            shuffle_flashes(note_review.notes[i]->flashes, note_review.notes[i]->flash_count);
        }

        shuffle_notes(note_review.notes, count);
    }

    note_review.note_count = count;
    note_review.current_note = 0;
    note_review.current_flash = 0;
    note_review.correct = 0;
    note_review.incorrect = 0;
    note_review.showing_front = true;
    while (note_review.notes[note_review.current_note]->flash_count < 1 && !cez_review_complete()) {
        note_review.current_note ++;
    }
    return (const NoteReview*)&note_review;
}

void cez_review_next_flash() {
    if (cez_review_complete()) return;
    if (note_review.showing_front) {
        note_review.showing_front = false;
        return;
    }  

    if (!(note_review.current_flash + 1 < note_review.notes[note_review.current_note]->flash_count)) {
        note_review.current_note ++;
        note_review.current_flash = 0;
        note_review.showing_front = true;
    } else {
        note_review.current_flash ++;
        note_review.showing_front = true;
    }

    while (!cez_review_complete() && note_review.notes[note_review.current_note]->flash_count < 1) {
        note_review.current_note ++;
        note_review.current_flash = 0;
        note_review.showing_front = true;
    }
}

void cez_show_back() {
    if (cez_review_complete()) return;
    if (note_review.showing_front) {
        note_review.showing_front = false;
        return;
    }
}

void cez_review_correct_answer() {
    if (note_review.showing_front) {
        return;
    }
    note_review.correct ++;
    cez_review_next_flash();
}
void cez_review_incorrect_answer() {
    if (note_review.showing_front) {
        return;
    }

    note_review.incorrect ++;
    cez_review_next_flash();
}

bool cez_review_complete() {
    return note_review.current_note >= note_review.note_count;
}

const Notes *get_notes() {
    notes.note_count = graph.note_count;
    notes.notes = (const Note**)graph.notes;
    return (const Notes *)&notes;
}

void cez_shutdown() {
    
}

// - - - underlying hidden functions - - -

void nprint(const char *str, int n) {
    for (int i = 0; i < n; i++) {
        fprintf(stderr, "%c", str[i]);
    }
    fprintf(stderr, "\n");
}

void cez_log(LogLevel level, const char *fmt, ...) {
	// TODO: (syd) Minimal log level missing for now
    switch (level) {
    case INFO:
        fprintf(stderr, "[INFO] ");
        break;
    case WARNING:
        fprintf(stderr, "[WARN] ");
        break;
    case ERROR:
        fprintf(stderr, "[ERR.] ");
        break;
    default:
        // TODO: (syd) unreachable stuff?
        fprintf(stderr, "UNREACHABLE: %d", __LINE__);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// makes a new note from a file
// the only way of creating a note?
bool read_file(const char *abs_path, Graph *graph) {
	cez_log(INFO, "reading file: %s", abs_path);
    bool result = true;
    FILE *in = NULL;
    //char new_note->view_text[new_note->view_text_CAP];
    size_t chars_read = 0;
    size_t file_size = 0;

    graph->notes[graph->note_count] = (Note*)arena_alloc(&graph_arena, sizeof(Note));
    Note *new_note = graph->notes[graph->note_count];
    graph->note_count ++;

    // 0 = nothing found/card completed
    // 1 = found opening of front face
    // 2 = found full front face
    // 3 = found opening of back face
    int32_t card_search_stage = 0;
    size_t card_start_index = 0;
    size_t new_card_index = 0;

    int32_t cloze_search_stage = 0;
    uint64_t new_cloze_index = 0;
    size_t last_newline_index = 0;

    in = fopen(abs_path, "r");
    if (!in) {
        cez_log(ERROR, "Failed to open file: %s", abs_path);
        return_defer(false);
    }

    // load view_text into new_note
    fseek(in, 0, SEEK_END);
    file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    cez_log(INFO, "alloc size: %lu", file_size);
    new_note->view_text = (char*)arena_alloc(&new_note->arena, file_size+1);
    chars_read = fread(new_note->view_text, sizeof(char), file_size, in);
    new_note->view_text[file_size] = '\0';
    cez_log(INFO, "READING: %s", new_note->view_text);


    new_note->relative_path = arena_strdup(&new_note->arena, get_relative(abs_path, graph));
    new_note->flashes = (Flash*)arena_alloc(&new_note->arena, sizeof(Flash) * START_FLASH_CAPACITY);
    new_note->flash_capacity = START_FLASH_CAPACITY;
    new_note->occupied = true;


    // TODO: (syd) check for back of card not being filled to avoid incomplete cards
    for (size_t index = 0; index < chars_read; index++) {

        if (new_note->view_text[index] == '\n' && cloze_search_stage != 2) {
            // not allowing for clozes to be between lines
            if (cloze_search_stage == 1) {
                cloze_search_stage = 0;
            }

            last_newline_index = index;
        }

        switch(card_search_stage) {
        case 0: // finding start of front
            if (new_note->view_text[index] == ';'
             && new_note->view_text[index + 1] == ';') {

                card_search_stage = 1;

                card_start_index = index + 2;
            }
            break;
        case 1: // finding end of front
            if (
                    new_note->view_text[index] == ';'
                    && new_note->view_text[index + 1] == ';') {

                card_search_stage = 2;

                realloc_flashes(new_note);

                size_t end_index = index;
                size_t str_size = sizeof(char) * (end_index - card_start_index);

                new_card_index = new_note->flash_count;
                new_note->flash_count ++;
                new_note->flashes[new_card_index].type = CEZ_CARD;
                new_note->flashes[new_card_index].data.card.front = (char*)arena_alloc(&new_note->arena, str_size+1);

                memcpy(new_note->flashes[new_card_index].data.card.front, &new_note->view_text[card_start_index], str_size);
                new_note->flashes[new_card_index].data.card.front[str_size] = '\0';
            }
            break;
        case 2: // finding start of back: "::"
            if (
                    new_note->view_text[index] == ':'
                    && new_note->view_text[index + 1] == ':') {

                card_search_stage = 3;
                card_start_index = index + 2;
            }
            break;
        case 3: // finding end of back: "::"
            if (
                    new_note->view_text[index] == ':'
                    && new_note->view_text[index + 1] == ':') {

                card_search_stage = 0;

                size_t end_index = index;
                size_t str_size = sizeof(char) * (end_index - card_start_index);

                new_note->flashes[new_card_index].data.card.back = (char*)arena_alloc(&new_note->arena, str_size+1);

                memcpy(new_note->flashes[new_card_index].data.card.back, &new_note->view_text[card_start_index], str_size);
                new_note->flashes[new_card_index].data.card.back[str_size] = '\0';
                cez_log(INFO, "new card %d: \n%s\n%s", new_card_index, 
                        new_note->flashes[new_card_index].data.card.front, 
                        new_note->flashes[new_card_index].data.card.back
                       );
            }
            break;
        default:
            cez_log(ERROR, "unreachable section: %d", __LINE__);
            break;
        }

        switch (cloze_search_stage) {
        case 0: // finding start of cloze ({{)
            if (new_note->view_text[index] == '{' && new_note->view_text[index+1] == '{') {
                cloze_search_stage = 1;
            }
            break;
        case 1: // finding end of cloze (}})
            if (new_note->view_text[index] == '}' && new_note->view_text[index+1] == '}') {
                cloze_search_stage = 2;
                cez_log(INFO, "moving to last stage of cloze reading");
            }
            break;
        case 2: // finding newline to finish cloze
            if (new_note->view_text[index] == '\n') {
                cloze_search_stage = 0;

                realloc_flashes(new_note);

                size_t end_index = index;

                new_cloze_index = new_note->flash_count;
                new_note->flashes[new_cloze_index].type = CEZ_CLOZE;
                size_t len = sizeof(char) * (end_index - last_newline_index);
                new_note->flashes[new_cloze_index].data.cloze.review_text = (char*)arena_alloc(&new_note->arena, len+1);
                new_note->flashes[new_cloze_index].data.cloze.view_text = (char*)arena_alloc(&new_note->arena, len+1);
                new_note->flash_count ++;

                memcpy(new_note->flashes[new_cloze_index].data.cloze.view_text, &new_note->view_text[last_newline_index], len);
                memcpy(new_note->flashes[new_cloze_index].data.cloze.review_text, &new_note->view_text[last_newline_index], len);
                new_note->flashes[new_cloze_index].data.cloze.view_text[len] = '\0';
                new_note->flashes[new_cloze_index].data.cloze.review_text[len] = '\0';
                make_review_text(
                        new_note->flashes[new_cloze_index].data.cloze.review_text, 
                        new_note->flashes[new_cloze_index].data.cloze.view_text
                        );
                cez_log(INFO, "new cloze %d: \n%s\n%s", new_cloze_index, 
                        new_note->flashes[new_cloze_index].data.cloze.review_text, 
                        new_note->flashes[new_cloze_index].data.cloze.view_text
                       );
                last_newline_index = index;
            }
            break;
        default:
            fprintf(stderr, "unreachable line: %d\n", __LINE__);
            break;
        }
    }
    // check for incomplete flashes
    if (!(card_search_stage == 0)) {
        cez_log(INFO, "incomplete card in: %s", abs_path);
        new_note->flash_count --;
    }
    if (cloze_search_stage == 2) {
        new_note->flash_count --;
    }
defer:
    return result;
}

void make_review_text(char *review_text, char *view_text) {
    bool in_cloze = false;
    for (size_t i = 0; i < strlen(review_text)-1; i++) {
        if (view_text[i] == '{' && view_text[i+1] == '{') {
            view_text[i] = ' ';
            view_text[i+1] = ' ';
            in_cloze = true;
            review_text[i] = '_';
            review_text[i+1] = '_';
            i += 2;
        }
        if (view_text[i] == '}' && view_text[i+1] == '}') {
            view_text[i] = ' ';
            view_text[i+1] = ' ';
            review_text[i] = '_';
            review_text[i+1] = '_';
            in_cloze = false;
        }
        if (in_cloze) {
            review_text[i] = '_';
        }
    }
}

bool recursive_file_load(const char *abs_path, Graph *graph) {
    bool result = true;
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    struct stat stat_buff;
    char entry_buff[READ_BUFF_CAP];

    dir = opendir(abs_path);
    if (!dir) {
        cez_log(ERROR, "failed to open dir: %s", abs_path);
        return_defer(false);
    }

    while ((ent = readdir(dir))) {
        strcpy(entry_buff, abs_path);
        entry_buff[strlen(entry_buff)+1] = '\0';
        entry_buff[strlen(entry_buff)] = '/';
        strcat(entry_buff, ent->d_name);
        if (stat(entry_buff, &stat_buff) == -1) {
            perror("stat");
            cez_log(ERROR, "stat fail");
            exit(1);
        }

		if (strcmp(ent->d_name, "..") == 0 || strcmp(ent->d_name, ".") == 0) {
			continue;
		}

        if ((stat_buff.st_mode & S_IFMT) == S_IFREG) {
            if (!is_markdown(ent->d_name)) {
                continue;
            }
            if (!read_file(entry_buff, graph)) {
                cez_log(ERROR, "failed to read file: %s", entry_buff);
            }
        } else if ((stat_buff.st_mode & S_IFMT) == S_IFDIR) {
            strcpy(entry_buff, abs_path);
            entry_buff[strlen(entry_buff)+1] = '\0';
            entry_buff[strlen(entry_buff)] = '/';
            strcat(entry_buff, ent->d_name);
            cez_log(INFO, "recursive search of: %s", entry_buff);
            recursive_file_load(entry_buff, graph);
        }
    }

defer:
    return result;
}

char *make_str(Arena *arena, const char *str) {
    char *result = (char*)malloc(sizeof(char) * strlen(str));
    strcpy(result, str);
    return result;
}

const char *get_absolute(const char *relative_path, const Graph *graph) {
    strcpy(absolute_buffer, graph->base_dir);
    strcat(absolute_buffer, relative_path);
    return absolute_buffer;
}

const char *get_relative(const char *abs_path, const Graph *graph) {
    return &abs_path[strlen(graph->base_dir) + 1];
}

bool is_markdown(const char *file_name) {
    if (strcmp(".md", &file_name[strlen(file_name) - 3]) == 0) {
        return true;
    } else {
        return false;
    }
}

void realloc_flashes(Note *note) {
    if (note->flash_count >= note->flash_capacity) {
        note->flashes = (Flash*)arena_realloc(&note->arena, note->flashes, sizeof(Flash) * note->flash_capacity, sizeof(Flash) * note->flash_capacity * 2);
        note->flash_capacity *= 2;
    }
}

uint64_t make_id(const char *relative_path) {
    uint64_t id = XXH3_64bits(relative_path, sizeof(char) * strlen(relative_path)) % graph.note_capacity;
    while (graph.notes[id]->occupied) {
        id ++;
        id %= graph.note_capacity;
    }
    return id;
}

