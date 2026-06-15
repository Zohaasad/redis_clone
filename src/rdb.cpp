#include "rdb.h"
#include "object.h"
#include "sds.h"
#include "list.h"
#include "htable.h"
#include "zset.h"
#include "expiry.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>


static uint64_t crc64_table[256];
static bool     crc64_initialized = false;

static void crc64_init() {
    if (crc64_initialized) return;
    for (int i = 0; i < 256; i++) {
        uint64_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xad93d23594c935a9ULL;
            else         crc >>= 1;
        }
        crc64_table[i] = crc;
    }
    crc64_initialized = true;
}

static uint64_t crc64_update(uint64_t crc, const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++)
        crc = crc64_table[(crc ^ p[i]) & 0xff] ^ (crc >> 8);
    return crc;
}


struct Writer {
    FILE*    f;
    uint64_t crc;

    Writer(FILE* f) : f(f), crc(0) { crc64_init(); }

    bool write(const void* data, size_t len) {
        if (fwrite(data, 1, len, f) != len) return false;
        crc = crc64_update(crc, data, len);
        return true;
    }

    bool write_u8(uint8_t v)   { return write(&v, 1); }
    bool write_u32(uint32_t v) { return write(&v, 4); }
    bool write_u64(uint64_t v) { return write(&v, 8); }
    bool write_i64(int64_t v)  { return write(&v, 8); }
    bool write_double(double v){ return write(&v, 8); }

    bool write_blob(const char* data, uint32_t len) {
        return write_u32(len) && write(data, len);
    }
};


struct Reader {
    FILE*    f;
    uint64_t crc;
    bool     ok;

    Reader(FILE* f) : f(f), crc(0), ok(true) { crc64_init(); }

    bool read(void* data, size_t len) {
        if (!ok) return false;
        if (fread(data, 1, len, f) != len) { ok = false; return false; }
        crc = crc64_update(crc, data, len);
        return true;
    }

    bool read_u8(uint8_t& v)   { return read(&v, 1); }
    bool read_u32(uint32_t& v) { return read(&v, 4); }
    bool read_u64(uint64_t& v) { return read(&v, 8); }
    bool read_i64(int64_t& v)  { return read(&v, 8); }
    bool read_double(double& v){ return read(&v, 8); }

    
    char* read_blob(uint32_t& out_len) {
        uint32_t len;
        if (!read_u32(len)) return nullptr;
        char* buf = new char[len + 1];
        if (!read(buf, len)) { delete[] buf; return nullptr; }
        buf[len]  = '\0';
        out_len   = len;
        return buf;
    }
};


struct HashCollect {
    sds* keys;
    sds* vals;
    int  max;
    int  count;
};

static bool hash_collect_cb(const char* key, size_t klen, Obj* val, void* ud) {
    HashCollect* ctx = (HashCollect*)ud;
    if (ctx->count >= ctx->max) return false;
    ctx->keys[ctx->count] = sds_new(key, klen);
    ctx->vals[ctx->count] = sds_dup(val->str);
    ctx->count++;
    return true;
}

static bool write_entry(Writer& w, const char* key, size_t klen, Obj* obj) {
   
    w.write_u8((uint8_t)obj->type);

    w.write_i64(obj->expire_at_ms);

    w.write_blob(key, (uint32_t)klen);

    switch (obj->type) {

    case OBJ_STRING:
        w.write_blob(obj->str, (uint32_t)sds_len(obj->str));
        break;

    case OBJ_LIST: {
        List* l = obj->list;
        w.write_u32((uint32_t)list_len(l));
        ListNode* n = l->head;
        while (n) {
            w.write_blob(n->value, (uint32_t)sds_len(n->value));
            n = n->next;
        }
        break;
    }

    case OBJ_HASH: {
        sds keys[4096], vals[4096];
        HashCollect ctx = { keys, vals, 4096, 0 };
        dict_each(obj->hash->d, hash_collect_cb, &ctx);
        w.write_u32((uint32_t)ctx.count);
        for (int i = 0; i < ctx.count; i++) {
            w.write_blob(keys[i], (uint32_t)sds_len(keys[i]));
            w.write_blob(vals[i], (uint32_t)sds_len(vals[i]));
            sds_free(keys[i]);
            sds_free(vals[i]);
        }
        break;
    }

    case OBJ_ZSET: {
        ZSet* z = obj->zset;
        uint32_t card = (uint32_t)zset_card(z);
        w.write_u32(card);
        char*  members[4096];
        double scores[4096];
        int count = zset_range(z, 0, (long)card - 1, members, scores, 4096);
        for (int i = 0; i < count; i++) {
            w.write_double(scores[i]);
            w.write_blob(members[i], (uint32_t)strlen(members[i]));
        }
        break;
    }
    }
    return true;
}


struct SaveCtx {
    Writer* w;
    bool    ok;
};

static bool save_cb(const char* key, size_t klen, Obj* val, void* ud) {
    SaveCtx* ctx = (SaveCtx*)ud;
    if (!write_entry(*ctx->w, key, klen, val)) {
        ctx->ok = false;
        return false;
    }
    return true;
}


bool rdb_save(Dict* d, const std::string& path) {
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) {
        perror("rdb_save: fopen");
        return false;
    }

    Writer w(f);

   
    w.write(RDB_MAGIC, 4);
    uint32_t ver = RDB_VERSION;
    w.write_u32(ver);
    uint64_t key_count = (uint64_t)dict_size(d);
    w.write_u64(key_count);


    SaveCtx ctx = { &w, true };
    dict_each(d, save_cb, &ctx);

    if (!ctx.ok) {
        fclose(f);
        remove(tmp.c_str());
        return false;
    }

   
    uint64_t final_crc = w.crc;
    fwrite(&final_crc, 8, 1, f);

    fflush(f);
    fclose(f);

   
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        perror("rdb_save: rename");
        return false;
    }

    printf("[minired] snapshot saved to %s (%llu keys)\n",
           path.c_str(), (unsigned long long)key_count);
    return true;
}

bool rdb_load(Dict* d, const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false; 

    Reader r(f);

    char magic[4];
    if (!r.read(magic, 4) || memcmp(magic, RDB_MAGIC, 4) != 0) {
        fprintf(stderr, "[minired] rdb: bad magic\n");
        fclose(f);
        return false;
    }

  
    uint32_t ver;
    if (!r.read_u32(ver) || ver != RDB_VERSION) {
        fprintf(stderr, "[minired] rdb: unsupported version %u\n", ver);
        fclose(f);
        return false;
    }

   
    uint64_t key_count;
    if (!r.read_u64(key_count)) {
        fclose(f);
        return false;
    }

    
    for (uint64_t i = 0; i < key_count; i++) {
        uint8_t type;
        if (!r.read_u8(type)) { fclose(f); return false; }

        int64_t expire_at_ms;
        if (!r.read_i64(expire_at_ms)) { fclose(f); return false; }


        uint32_t klen;
        char* key = r.read_blob(klen);
        if (!key) { fclose(f); return false; }

        Obj* obj = nullptr;

        switch (type) {

        case RDB_TYPE_STRING: {
            uint32_t vlen;
            char* val = r.read_blob(vlen);
            if (!val) { delete[] key; fclose(f); return false; }
            sds s = sds_new(val, vlen);
            delete[] val;
            obj = new Obj(s);
            break;
        }

        case RDB_TYPE_LIST: {
            uint32_t count;
            if (!r.read_u32(count)) { delete[] key; fclose(f); return false; }
            List* l = new List();
            for (uint32_t j = 0; j < count; j++) {
                uint32_t elen;
                char* elem = r.read_blob(elen);
                if (!elem) { delete[] key; delete l; fclose(f); return false; }
                list_rpush(l, sds_new(elem, elen));
                delete[] elem;
            }
            obj = new Obj(l);
            break;
        }

        case RDB_TYPE_HASH: {
            uint32_t count;
            if (!r.read_u32(count)) { delete[] key; fclose(f); return false; }
            HTable* h = new HTable();
            for (uint32_t j = 0; j < count; j++) {
                uint32_t flen, vlen;
                char* field = r.read_blob(flen);
                if (!field) { delete[] key; delete h; fclose(f); return false; }
                char* val = r.read_blob(vlen);
                if (!val) { delete[] field; delete[] key; delete h; fclose(f); return false; }
                htable_set(h, field, flen, val, vlen);
                delete[] field;
                delete[] val;
            }
            obj = new Obj(h);
            break;
        }

        case RDB_TYPE_ZSET: {
            uint32_t count;
            if (!r.read_u32(count)) { delete[] key; fclose(f); return false; }
            ZSet* z = new ZSet();
            for (uint32_t j = 0; j < count; j++) {
                double score;
                if (!r.read_double(score)) { delete[] key; delete z; fclose(f); return false; }
                uint32_t mlen;
                char* member = r.read_blob(mlen);
                if (!member) { delete[] key; delete z; fclose(f); return false; }
                zset_add(z, member, mlen, score);
                delete[] member;
            }
            obj = new Obj(z);
            break;
        }

        default:
            fprintf(stderr, "[minired] rdb: unknown type %u\n", type);
            delete[] key;
            fclose(f);
            return false;
        }

        obj->expire_at_ms = expire_at_ms;

       
        if (expire_at_ms != 0 && now_ms() > expire_at_ms) {
            delete obj;
        } else {
            dict_set(d, key, klen, obj);
        }
        delete[] key;
    }

  
    uint64_t stored_crc;
    uint64_t computed_crc = r.crc;
    if (fread(&stored_crc, 8, 1, f) != 1 || stored_crc != computed_crc) {
        fprintf(stderr, "[minired] rdb: CRC mismatch — file is corrupt\n");
        fclose(f);
        return false;
    }

    fclose(f);
    printf("[minired] snapshot loaded from %s (%llu keys)\n",
           path.c_str(), (unsigned long long)key_count);
    return true;
}
