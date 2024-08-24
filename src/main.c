#include "rudyscung.h"
#include "world/tile.h"

#include <SDL2/SDL.h>

int main(int argc, char** argv) {
    char const* resources_path = "../res";
    if (argc > 1) {
        resources_path = argv[1];
    }

    // Static initialization
    tiles_init();

    // Init RudyScung
    rudyscung_t* const rudyscung = rudyscung_new(resources_path);
    rudyscung_run(rudyscung);

    return 0;
}