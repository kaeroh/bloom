#pragma once
#ifndef BLOOM_DEFINITIONS_H
#define BLOOM_DEFINITIONS_H

#include "imgui/imgui.h"

#include "cez.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stdbool.h>

#define SELECTED_NOTES_CAPACITY 64
#define FMT_BUFF_CAPACITY 128

typedef enum TransitionView {
    NO_TRANSITION,
    TO_EXPLORING,
    TO_REVIEW,
    TO_POPUP,
} TransitionView;

typedef enum ActiveView {
    EXPLORING,
    REVIEWING,
    POPUP,// not implemented yet
} ActiveView;

typedef struct ReviewView {
    const NoteReview *note_review;
} ReviewView;

typedef enum ExploringActiveWindow {
    EXPLORING_SELECTED_NOTES,
    EXPLORING_NOTES_LIST,
} ExploringActiveWindow;

typedef struct ExploringView {
    size_t selected_notes[SELECTED_NOTES_CAPACITY];
    size_t selected_notes_count;
    uint64_t highlighted_note;
    uint64_t highlighted_selected_note;
    ExploringActiveWindow active_window;
} ExploringView;

typedef struct State {
    SDL_Window *window;
    SDL_GPUDevice *gpu_device;

    ImGuiStyle style;

    const Graph *graph;
    float main_scale;
    bool running;
    int wwidth;
    int wheight;

    const Notes *notes_ref;

    ActiveView active_view;

    ExploringView exploring;
    ReviewView review;

    bool leader_pressed;
    char cwd[128];
} State;

void handle_keypress(SDL_Event *ev);

TransitionView exploring_keypress(ExploringView *state, SDL_Event *ev, const Notes *notes_ref, bool *leader_key_pressed);
void add_note_by_index(ExploringView *state, size_t index);
void remove_note_by_index(ExploringView *state, size_t index);
bool note_selected(const ExploringView *state, size_t index);

TransitionView reviewing_keypress(ReviewView *state, SDL_Event *ev, bool *leader_key_pressed);
void draw_review(const ReviewView *state);
void draw_explorer(const ExploringView *state, ImGuiStyle *style, const Notes *notes_ref);

int32_t scale_w(float fraction);
int32_t scale_h(float fraction);

#endif

