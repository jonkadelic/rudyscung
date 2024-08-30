#pragma once

typedef struct server server_t;

server_t* const server_new();

void server_delete(server_t* const self);

void server_run(server_t* const self);
