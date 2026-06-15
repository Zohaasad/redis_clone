#include "expiry.h"
#include <chrono>

#include <thread>
#include <atomic>

int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

bool check_expiry(Dict* d, const char* key, size_t klen) {
    Obj* obj = dict_get(d, key, klen);
    if (!obj) return false;
    if (obj->expire_at_ms == 0) return false;
    if (now_ms() < obj->expire_at_ms) return false;
    dict_del(d, key, klen);
    return true;
}



static std::thread sweep_thread;
static std::atomic<bool> sweep_running(false);

struct SweepCtx {
    std::vector<std::string> keys;
    int max;
    int count;
};

static bool sweep_cb(const char* key, size_t klen, Obj* val, void* ud) {
    SweepCtx* ctx = (SweepCtx*)ud;
    if (ctx->count >= ctx->max) return false;
    if (val->expire_at_ms != 0) {
        ctx->keys.push_back(std::string(key, klen));
        ctx->count++;
    }
    return true;
}

static void sweep_loop(Dict** dict_ptr) {
    while (sweep_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!*dict_ptr) continue;
        SweepCtx ctx;
        ctx.max   = 20;
        ctx.count = 0;
        dict_each(*dict_ptr, sweep_cb, &ctx);
        for (auto& key : ctx.keys) {
            check_expiry(*dict_ptr, key.data(), key.size());
        }
    }
}

void start_expiry_sweep(Dict** dict_ptr) {
    sweep_running = true;
    sweep_thread  = std::thread(sweep_loop, dict_ptr);
    printf("[minired] active expiry sweep started\n");
}

void stop_expiry_sweep() {
    sweep_running = false;
    if (sweep_thread.joinable()) sweep_thread.join();
}