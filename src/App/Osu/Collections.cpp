#include "Collections.h"

#include "BanchoProtocol.h"
#include "ConVar.h"
#include "Database.h"
#include "Engine.h"

bool collections_loaded = false;
std::vector<Collection*> collections;

void Collection::delete_collection() {
    for(auto map : maps) {
        remove_map(map);
    }
}

void Collection::add_map(MD5Hash map_hash) {
    {
        auto it = std::find(deleted_maps.begin(), deleted_maps.end(), map_hash);
        if(it != deleted_maps.end()) {
            deleted_maps.erase(it);
        }
    }

    {
        auto it = std::find(neosu_maps.begin(), neosu_maps.end(), map_hash);
        if(it == neosu_maps.end()) {
            neosu_maps.push_back(map_hash);
        }
    }

    {
        auto it = std::find(maps.begin(), maps.end(), map_hash);
        if(it == maps.end()) {
            maps.push_back(map_hash);
        }
    }
}

void Collection::remove_map(MD5Hash map_hash) {
    {
        auto it = std::find(maps.begin(), maps.end(), map_hash);
        if(it != maps.end()) {
            maps.erase(it);
        }
    }

    {
        auto it = std::find(neosu_maps.begin(), neosu_maps.end(), map_hash);
        if(it != neosu_maps.end()) {
            neosu_maps.erase(it);
        }
    }

    {
        auto it = std::find(peppy_maps.begin(), peppy_maps.end(), map_hash);
        if(it != peppy_maps.end()) {
            deleted_maps.push_back(map_hash);
        }
    }
}

void Collection::rename_to(std::string new_name) {
    if(new_name.length() < 1) new_name = "Untitled collection";
    if(name == new_name) return;

    auto new_collection = get_or_create_collection(new_name);

    for(auto map : maps) {
        remove_map(map);
        new_collection->add_map(map);
    }
}

Collection* get_or_create_collection(std::string name) {
    if(name.length() < 1) name = "Untitled collection";

    for(auto collection : collections) {
        if(collection->name == name) {
            return collection;
        }
    }

    auto collection = new Collection();
    collection->name = name;
    collections.push_back(collection);

    return collection;
}

bool load_collections() {
    const double startTime = engine->getTimeReal();

    unload_collections();

    auto osu_folder = convar->getConVarByName("osu_folder")->getString();
    auto osu_database_version = convar->getConVarByName("osu_database_version")->getInt();

    std::string peppy_collections_path = osu_folder.toUtf8();
    peppy_collections_path.append("collection.db");
    Packet peppy_collections = load_db(peppy_collections_path);
    if(peppy_collections.size > 0) {
        u32 version = read<u32>(&peppy_collections);
        u32 nb_collections = read<u32>(&peppy_collections);

        if(version > osu_database_version) {
            debugLog("osu!stable collection.db version more recent than neosu, loading might fail.\n");
        }

        for(int c = 0; c < nb_collections; c++) {
            auto name = read_stdstring(&peppy_collections);
            u32 nb_maps = read<u32>(&peppy_collections);

            auto collection = get_or_create_collection(name);
            collection->maps.reserve(nb_maps);
            collection->peppy_maps.reserve(nb_maps);

            for(int m = 0; m < nb_maps; m++) {
                auto map_hash = read_hash(&peppy_collections);
                collection->maps.push_back(map_hash);
                collection->peppy_maps.push_back(map_hash);
            }
        }
    }
    free(peppy_collections.memory);

    auto neosu_collections = load_db("collections.db");
    if(neosu_collections.size > 0) {
        u32 version = read<u32>(&neosu_collections);
        u32 nb_collections = read<u32>(&neosu_collections);

        if(version > COLLECTIONS_DB_VERSION) {
            debugLog("neosu collections.db version is too recent! Cannot load it without stuff breaking.\n");
            free(neosu_collections.memory);
            unload_collections();
            return false;
        }

        for(int c = 0; c < nb_collections; c++) {
            auto name = read_stdstring(&neosu_collections);
            auto collection = get_or_create_collection(name);

            u32 nb_deleted_maps = 0;
            if(version >= 20240429) {
                nb_deleted_maps = read<u32>(&neosu_collections);
            }

            collection->deleted_maps.reserve(nb_deleted_maps);
            for(int d = 0; d < nb_deleted_maps; d++) {
                auto map_hash = read_hash(&neosu_collections);

                auto it = std::find(collection->maps.begin(), collection->maps.end(), map_hash);
                if(it != collection->maps.end()) {
                    collection->maps.erase(it);
                }

                collection->deleted_maps.push_back(map_hash);
            }

            u32 nb_maps = read<u32>(&neosu_collections);
            collection->maps.reserve(collection->maps.size() + nb_maps);
            collection->neosu_maps.reserve(nb_maps);

            for(int m = 0; m < nb_maps; m++) {
                auto map_hash = read_hash(&neosu_collections);

                auto it = std::find(collection->maps.begin(), collection->maps.end(), map_hash);
                if(it == collection->maps.end()) {
                    collection->maps.push_back(map_hash);
                }

                collection->neosu_maps.push_back(map_hash);
            }
        }
    }
    free(neosu_collections.memory);

    u32 nb_peppy = 0;
    u32 nb_neosu = 0;
    u32 nb_total = 0;
    for(auto collection : collections) {
        nb_peppy += collection->peppy_maps.size();
        nb_neosu += collection->neosu_maps.size();
        nb_total += collection->maps.size();
    }

    debugLog("peppy+neosu collections: loading took %f seconds (%d peppy, %d neosu, %d maps total)\n",
             (engine->getTimeReal() - startTime), nb_peppy, nb_neosu, nb_total);
    collections_loaded = true;
    return true;
}

void unload_collections() {
    collections_loaded = false;

    for(auto collection : collections) {
        delete collection;
    }
    collections.clear();
}

bool save_collections() {
    if(!collections_loaded) {
        debugLog("Cannot save collections since they weren't loaded properly first!\n");
        return false;
    }

    const double startTime = engine->getTimeReal();

    Packet db;
    write<u32>(&db, COLLECTIONS_DB_VERSION);

    u32 nb_collections = collections.size();
    write<u32>(&db, nb_collections);

    for(auto collection : collections) {
        write_string(&db, collection->name.c_str());

        u32 nb_deleted = collection->deleted_maps.size();
        write<u32>(&db, nb_deleted);
        for(auto map : collection->deleted_maps) {
            write_string(&db, map.hash);
        }

        u32 nb_neosu = collection->neosu_maps.size();
        write<u32>(&db, nb_neosu);
        for(auto map : collection->neosu_maps) {
            write_string(&db, map.hash);
        }
    }

    if(!save_db(&db, "collections.db")) {
        debugLog("Couldn't write collections.db!\n");
        return false;
    }

    debugLog("collections.db: saving took %f seconds\n", (engine->getTimeReal() - startTime));
    return true;
}
