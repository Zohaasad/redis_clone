#pragma once

#include <vector>
#include <string>
#include "dict.h"

extern Dict*       g_dict;
extern std::string g_rdb_path;

struct Client;

void commands_init();
void dispatch_command(Client* client, std::vector<std::string>& args);