#include "definitions.hpp"

TransitionView exploring_keypress(ExploringView *state, SDL_Event *ev, const Notes *notes_ref, bool *leader_key_pressed) {

    if (*leader_key_pressed) {
        switch (ev->key.key) {
        break;
        case SDLK_H:
            if (*leader_key_pressed && state->active_window == EXPLORING_SELECTED_NOTES) {
                *leader_key_pressed = false;
                state->active_window = EXPLORING_NOTES_LIST;
                break;
            }
            break;
        case SDLK_L: 
            if (*leader_key_pressed && state->active_window == EXPLORING_NOTES_LIST) {
                *leader_key_pressed = false;
                state->active_window = EXPLORING_SELECTED_NOTES;
                break;
            }
        break;
            break;
        }
        *leader_key_pressed = false;
        return NO_TRANSITION;
    }

    switch (ev->key.key) {
    case SDLK_K:
        switch (state->active_window) {
        case EXPLORING_SELECTED_NOTES:
            if (!(state->highlighted_selected_note< 1)) {
                state->highlighted_selected_note --;
            } else {
                state->highlighted_selected_note = state->selected_notes_count - 1;
            }
            break;
        case EXPLORING_NOTES_LIST:
            if (!(state->highlighted_note < 1)) {
                state->highlighted_note --;
            } else {
                state->highlighted_note = notes_ref->note_count - 1;
            }
            break;
        }
        break;
    case SDLK_J:
        switch (state->active_window) {
        case EXPLORING_SELECTED_NOTES:
            if (state->highlighted_selected_note + 1 < state->selected_notes_count) {
                state->highlighted_selected_note ++;
            } else {
                state->highlighted_selected_note = 0;
            }
            break;
        case EXPLORING_NOTES_LIST:
            if (state->highlighted_note + 1 < notes_ref->note_count) {
                state->highlighted_note ++;
            } else {
                state->highlighted_note = 0;
            }
            break;
        }
        break;
    case SDLK_X:
        switch (state->active_window) {
        case EXPLORING_NOTES_LIST:
            if (!note_selected(state, state->highlighted_note))
                add_note_by_index(state, state->highlighted_note);
            else
                remove_note_by_index(state, state->highlighted_note);
            break;
        case EXPLORING_SELECTED_NOTES:
            remove_note_by_index(state, state->selected_notes[state->highlighted_selected_note]);
            break;
        }
        break;
    case SDLK_R:
        if (state->selected_notes_count > 0) {
            return TO_REVIEW;
        }
        break;
    case SDLK_ESCAPE:
        *leader_key_pressed = false;
        memset(state->selected_notes, 0, sizeof(size_t) * SELECTED_NOTES_CAPACITY);
    default:
        break;
    }
    return NO_TRANSITION;
}

void draw_explorer(const ExploringView *state, ImGuiStyle *style, const Notes *notes_ref) {

    // drawing main window with info on selected note
    ImGui::Begin("Selecting and reviewing notes", nullptr, ImGuiWindowFlags_NoMove);
    ImGui::SetWindowSize(ImVec2(scale_w(0.69), scale_h(0.96)));
    ImGui::SetWindowPos(ImVec2(scale_w(0.01), scale_h(0.02)));

    for (size_t i = 0; i < notes_ref->note_count; i++) {
        if (i == state->highlighted_note) {
            ImGui::TextColored(style->Colors[ImGuiCol_TextLink], "%s", notes_ref->notes[i]->relative_path);

        } else if (note_selected(state, i)) {
            ImGui::TextColored(style->Colors[ImGuiCol_ButtonActive], "%s", notes_ref->notes[i]->relative_path);
        } else {
            ImGui::Text("%s", notes_ref->notes[i]->relative_path);
        }
    }

    ImGui::End();

    // window for listing selected 
    ImGui::Begin("Currently selected notes", nullptr, ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos(ImVec2(scale_w(0.70), scale_h(0.02)));
    ImGui::SetWindowSize(ImVec2(scale_w(0.29), scale_h(0.96)));

    for (size_t i = 0; i < state->selected_notes_count; i++) {
        if (i == state->highlighted_selected_note) {
            ImGui::TextColored(style->Colors[ImGuiCol_TextLink], "%s", notes_ref->notes[state->selected_notes[i]]->relative_path);
        } else {
            ImGui::Text("%s", notes_ref->notes[state->selected_notes[i]]->relative_path);
        }
    }

    ImGui::End();
}

void remove_note_by_index(ExploringView *state, size_t index) {
    if (!note_selected(state, index)) return;

    for (size_t i = 0; i < state->selected_notes_count; i++) {
        if (state->selected_notes[i] == index) {
            if (state->selected_notes_count == 1) {
                state->highlighted_selected_note = 0;
                state->selected_notes[i] = 0;
                state->selected_notes_count = 0;
                return;
            }
            state->selected_notes_count --;
            for (size_t j = i; j < state->selected_notes_count; j++) {
                state->selected_notes[j] = state->selected_notes[j + 1];
            }
            if (state->highlighted_selected_note >= state->selected_notes_count)
                state->highlighted_selected_note --;
            return;
        }
    }
}

void add_note_by_index(ExploringView *state, size_t index) {
    // TODO: (syd) make it so this doesn't fail if you select more than SELECTED_NOTES_CAPACITY notes
    if (note_selected(state, index)) return;
    state->selected_notes[state->selected_notes_count++] = index;
    /*
    for (int i = 0; i < state->exploring.selected_notes_count; i++) {
        cez_log(INFO, "%d: %d", i, state->exploring.selected_notes[i]);
    }
    */
}

bool note_selected(const ExploringView *state, size_t index) {
    if (state->selected_notes_count < 1)
        // no notes selected :(
        return false;
    for (size_t i = 0; i < state->selected_notes_count; i++) {
        if (state->selected_notes[i] == index) {
            return true;
        }
    }
    return false;
}
