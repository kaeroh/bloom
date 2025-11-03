#include "cez.hpp"
#include "definitions.hpp"

void display_note_for_review(
        const Note *note, 
        uint64_t flash_index, 
        bool show_front
        ) {
        assert(note != NULL);
        switch (note->flashes[flash_index].type) {
                case CEZ_CARD:
                        ImGui::Text("%s", note->flashes[flash_index].data.card.front);
                        if (!show_front) {
                                ImGui::Text("%s", note->flashes[flash_index].data.card.back);
                        }
                        break;
                case CEZ_CLOZE:
                        if (show_front)
                                ImGui::Text("%s", note->flashes[flash_index].data.cloze.review_text);
                        else
                                ImGui::Text("%s", note->flashes[flash_index].data.cloze.view_text);
                        break;
                default:
                        cez_log(WARNING, "unreachable in src/main.c: %d, %d", __LINE__, note->flashes[flash_index].type);
                        break;
        }
}

void draw_review(const ReviewView *state) {
        // TODO: better branching for this function? It's pretty sloppy.
        char fmt_buff[FMT_BUFF_CAPACITY];

        if (cez_review_complete()) {
                strcpy(fmt_buff, "Review complete");
        } else {
                snprintf(fmt_buff, FMT_BUFF_CAPACITY, "Reviewing: %s", state->note_review->notes[state->note_review->current_note]->relative_path);
        }
        ImGui::Begin(fmt_buff, nullptr, ImGuiWindowFlags_NoMove);
        ImGui::SetWindowSize(ImVec2(scale_w(0.69), scale_h(0.96)));
        ImGui::SetWindowPos(ImVec2(scale_w(0.01), scale_h(0.02)));

        if (!cez_review_complete())
                display_note_for_review(
                                state->note_review->notes[state->note_review->current_note], 
                                state->note_review->current_flash, 
                                state->note_review->showing_front);

        ImGui::End();

        ImGui::Begin("Stats for current review");
        ImGui::SetWindowSize(ImVec2(scale_w(0.29), scale_h(0.96)));
        ImGui::SetWindowPos(ImVec2(scale_w(0.70), scale_h(0.02)));

        if (cez_review_complete()) {
                snprintf(fmt_buff, FMT_BUFF_CAPACITY, "%lu/%lu notes completed", state->note_review->current_note, state->note_review->note_count);
                ImGui::Text("%s", fmt_buff);
                snprintf(fmt_buff, FMT_BUFF_CAPACITY, "%lu/%lu flashes completed in note", cez_get_current_shown_note()->flash_count, cez_get_current_shown_note()->flash_count);
                ImGui::Text("%s", fmt_buff);
        } else {
                snprintf(fmt_buff, FMT_BUFF_CAPACITY, "%lu/%lu notes completed", state->note_review->current_note + 1, state->note_review->note_count);
                ImGui::Text("%s", fmt_buff);
                snprintf(fmt_buff, FMT_BUFF_CAPACITY, "%lu/%lu flashes completed in note", state->note_review->current_flash, cez_get_current_shown_note()->flash_count);
                ImGui::Text("%s", fmt_buff);
        }


        snprintf(fmt_buff, FMT_BUFF_CAPACITY, "%d correct", state->note_review->correct);
        ImGui::Text("%s", fmt_buff);

        snprintf(fmt_buff, FMT_BUFF_CAPACITY, "%d incorrect", state->note_review->incorrect);
        ImGui::Text("%s", fmt_buff);

        ImGui::End();
}

TransitionView reviewing_keypress(ReviewView *state, SDL_Event *ev, bool *leader_key_pressed) {
    switch (ev->key.key) {
    case SDLK_K:
        break;
    case SDLK_J:
        cez_show_back();
        break;
    case SDLK_H:
        if (!state->note_review->showing_front) {
            cez_review_incorrect_answer();
        }
        break;
    case SDLK_L:
        if (!state->note_review->showing_front) {
            cez_review_correct_answer();
        }
        break;
    case SDLK_ESCAPE:
        *leader_key_pressed = false;
        return TO_EXPLORING;
        break;
    default:
        break;
    }
    return NO_TRANSITION;
}
