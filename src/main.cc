#include "definitions.hpp"

#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdlgpu3.h"
#include "imgui/imgui_impl_sdl3.h"

#include <unistd.h>
#include <math.h>


// TODO: (syd) make sure structs are packed well and make the structure make sense


static State state = {0};
static ImVec4 CLEAR_COLOUR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

int32_t scale_w(float fraction) {
    return (int32_t)floorf(state.wwidth * fraction);
}

int32_t scale_h(float fraction) {
    return (int32_t)floorf(state.wheight * fraction);
}

int main() {
    char font_dir[128];
    state.active_view = EXPLORING;
    state.exploring.active_window = EXPLORING_NOTES_LIST;
    
    
    getcwd(state.cwd, 128);
    state.graph = cez_load_graph(state.cwd);
    state.notes_ref = get_notes();

    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) {
        cez_log(ERROR, "SDL_Init(): %s", SDL_GetError());
        return 1;
    }

    state.window = SDL_CreateWindow("Bloom", 640, 480, SDL_WINDOW_RESIZABLE);
    if (!state.window) {
        cez_log(ERROR, "SDL3 failed window creation: %s", SDL_GetError());
        return 1;
    }

    SDL_ShowWindow(state.window);

    state.main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    state.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB, true, nullptr);
    if (state.gpu_device == nullptr) {
        cez_log(ERROR, "SDL_CreateGPUDevice: %d", SDL_GetError());
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(state.gpu_device, state.window)) {
        cez_log(ERROR, "SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
    }
    SDL_SetGPUSwapchainParameters(state.gpu_device, state.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    state.style = ImGui::GetStyle();

    state.style.ScaleAllSizes(state.main_scale);
    state.style.FontScaleDpi = state.main_scale;

    ImGui_ImplSDL3_InitForSDLGPU(state.window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};

    init_info.Device = state.gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(state.gpu_device, state.window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

    ImGui_ImplSDLGPU3_Init(&init_info);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    state.style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    snprintf(font_dir, 128, "%s/.local/share/bloom/NotoSerif-Regular.ttf", getenv("HOME"));
    io.Fonts->AddFontFromFileTTF(font_dir);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    state.running = true;
    while (state.running) {
        SDL_GetWindowSize(state.window, &state.wwidth, &state.wheight);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {

            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT) {
                state.running = false;
            }

            handle_keypress(&ev);
        }

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        switch (state.active_view) {
        case EXPLORING:
            draw_explorer(&state.exploring, &state.style, state.notes_ref);
            break;
        case REVIEWING:
            draw_review(&state.review);
            break;
        default:
            break;
        }

        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();

        SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(state.gpu_device);
        SDL_GPUTexture *swapchain_texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, state.window, &swapchain_texture, nullptr, nullptr);

        if (swapchain_texture != nullptr) {
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor { CLEAR_COLOUR.x, CLEAR_COLOUR.y, CLEAR_COLOUR.z, CLEAR_COLOUR.w };
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;
            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass, nullptr);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_WaitForGPUIdle(state.gpu_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(state.gpu_device, state.window);
    SDL_DestroyGPUDevice(state.gpu_device);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
    return 0;
}

void handle_keypress(SDL_Event *ev) {
    TransitionView transition = NO_TRANSITION;

    if (!(ev->type == SDL_EVENT_KEY_DOWN)) {
        return;
    }

    if (ev->key.key == SDLK_W && ev->key.mod == SDL_KMOD_LCTRL) {
        state.leader_pressed = true;
        return;
    }

    switch (state.active_view) {
    case POPUP:
        // have to make this it's own function since popups system will be a little more elaborate
        /*
        
        switch (ev->key.keysym.sym) {
        case SDLK_k:
            state.highlighted_review_type --;
            break;
        case SDLK_j:
            state.highlighted_review_type ++;
            break;
        case SDLK_ESCAPE:
            state.active_view = EXPLORING;
            break;
        default:
            break;
        }

*/
        break;
    case EXPLORING:
        transition = exploring_keypress(&state.exploring, ev, state.notes_ref, &state.leader_pressed);
        break;
    case REVIEWING:
        transition = reviewing_keypress(&state.review, ev, &state.leader_pressed);
    default:
        break;
    }

    // transition to another view if needed
    switch (state.active_view) {
    case EXPLORING:
        switch (transition) {
        case TO_REVIEW:
            state.review.note_review = cez_load_review_from_array(state.exploring.selected_notes, state.exploring.selected_notes_count, true);
            state.active_view = REVIEWING;
            break;
        case TO_EXPLORING:
            break;
        case TO_POPUP:
            cez_log(WARNING, "unreachable, unimplemented code: %d", __LINE__);
            break;
        default:
          break;
        }
        break;
    case REVIEWING:
        switch (transition) {
        case TO_REVIEW:
            break;
        case TO_EXPLORING:
            state.active_view = EXPLORING;
            break;
        case TO_POPUP:
            cez_log(WARNING, "unreachable, unimplemented code: %d", __LINE__);
        default:
            break;
        }
        break;
    case POPUP:
        cez_log(WARNING, "unreachable, unimplemented code: %d", __LINE__);
        break;
    }

}

