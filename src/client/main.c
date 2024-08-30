#include <SDL2/SDL.h>

#include "src/client/client.h"
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

    // static initialization
    tiles_init();

    // init rudyscung
    client_t* const client = client_new(resources_path);
    client_run(client);

    client_delete(client);

    return 0;
}