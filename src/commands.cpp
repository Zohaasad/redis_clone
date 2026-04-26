#include "commands.h"
#include "client.h"
#include "sds.h"
#include "object.h"
#include <algorithm>
#include <cctype>
#include <cstring>
Dict* g_dict = nullptr;

void commands_init() {
    g_dict = new Dict();
}