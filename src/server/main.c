#include "src/server/server.h"
#include "src/util/logger.h"
#include "src/world/tile.h"

int main(int argc, char** argv) {
    logger_set_log_level(LOG_LEVEL__DEBUG);

    LOG_INFO("Starting RudyScung server...");

    // static initialization
    tiles_init();

    // init rudyscung server
    server_t* const server = server_new();
    server_run(server);

    server_delete(server);
    
    return 0;
}