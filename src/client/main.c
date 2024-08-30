#include <SDL2/SDL.h>

#include "src/client/rudyscung.h"
#include "src/world/tile.h"
#include "src/util/logger.h"

int main(int argc, char** argv) {
    logger_set_log_level(LOG_LEVEL__DEBUG);

    LOG_INFO("Starting RudyScung...");

    char const* resources_path = "./res";
    if (argc > 1) {
        resources_path = argv[1];

        LOG_DEBUG("Resources path set to %s.", resources_path);
    }

    // Static initialization
    tiles_init();

    // Init RudyScung
    rudyscung_t* const rudyscung = rudyscung_new(resources_path);
    rudyscung_run(rudyscung);

    rudyscung_delete(rudyscung);

    return 0;
}