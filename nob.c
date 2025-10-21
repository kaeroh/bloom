#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "build/"

static const char *SRC[] = {
    "./src/cez.cc",
    "./src/main.cc",
    "./src/review_view.cc",
    "./src/exploring_view.cc",
};

static const char *IMGUI_SRC[] = {
    "./external/imgui/imgui_widgets.cpp",
    "./external/imgui/imgui_draw.cpp",
    "./external/imgui/imgui_tables.cpp",
    "./external/imgui/imgui.cpp",
    "./external/imgui/imgui_impl_sdlgpu3.cpp",
    "./external/imgui/imgui_impl_sdl3.cpp",
};

static const size_t IMGUI_SRC_LEN = sizeof(IMGUI_SRC)/sizeof(char*);
static const size_t SRC_LEN = sizeof(SRC)/sizeof(char*);

static Nob_Cmd cmd = {0};

void std_flags(Nob_Cmd *cmd) {
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-g", "-I./external");
}

void add_the_sauce(Nob_Cmd *cmd) {
    for (int i = 0; i < SRC_LEN; i++) {
        nob_cmd_append(cmd, SRC[i]);
    }
}

void add_imgui_src(Nob_Cmd *cmd) {
    for (int i = 0; i < IMGUI_SRC_LEN; i++) {
        nob_cmd_append(cmd, IMGUI_SRC[i]);
    }
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    // build static
    nob_cmd_append(&cmd, "g++");
    std_flags(&cmd);

    nob_cmd_append(&cmd, "-o", "./build/bloom");
    add_the_sauce(&cmd);
    add_imgui_src(&cmd);
    nob_cmd_append(&cmd, "-lSDL3", "-lm");
    nob_cmd_run_sync_and_reset(&cmd);

/*
nob_cmd_append(&cmd, "-c", "-fPIC");
nob_cmd_append(&cmd, LIB_SRC[0]);
nob_cmd_append(&cmd, "-o", "./build/cez.o");
nob_cmd_run_sync_and_reset(&cmd);

nob_cmd_append(&cmd, "ar", "rcs", "./build/libcezstatic.a", "./build/cez.o");
nob_cmd_run_sync_and_reset(&cmd);

// build dynamic
nob_cmd_append(&cmd, "cc", "-shared", "-o", "./build/libcez.so", "./build/cez.o");
nob_cmd_run_sync_and_reset(&cmd);
*/
}

