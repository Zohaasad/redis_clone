#pragma once

#include <vector>
#include <string>
#include "dict.h"
using namespace std;
extern Dict* g_dict;
struct Client;
void commands_init();

void dispatch_command(Client* client, vector<string>& args);