#include "Collections.h"

#include "BanchoFile.h"
#include "BanchoProtocol.h"
#include "ConVar.h"
#include "Database.h"
#include "Engine.h"

bool collections_loaded = false;
std::vector<Collection*> collections;

void Collection::delete_collection() {
    for(auto map : this->maps) {
        this->remove_map(map);
    }
}

void Collection::add_map(MD5Hash map_hash) {
    {
        auto it = std::find(this->deleted_maps.begin(), this->deleted_maps.end(), map_hash);
        if(it != this->deleted_maps.end()) {
            this->deleted_maps.erase(it);
        }
    }

    {
        auto it = std::find(this->neosu_maps.begin(), this->neosu_maps.end(), map_hash);
        if(it == this->neosu_maps.end()) {
            this->neosu_maps.push_back(map_hash);
        }
    }

    {
        auto it = std::find(this->maps.begin(), this->maps.end(), map_hash);
        if(it == this->maps.end()) {
            this->maps.push_back(map_hash);
        }
    }
}

void Collection::remove_map(MD5Hash map_hash) {
    {
        auto it = std::find(this->maps.begin(), this->maps.end(), map_hash);
        if(it != this->maps.end()) {
            this->maps.erase(it);
        }
    }

    {
        auto it = std::find(this->neosu_maps.begin(), this->neosu_maps.end(), map_hash);
        if(it != this->neosu_maps.end()) {
            this->neosu_maps.erase(it);
        }
    }

    {
        auto it = std::find(this->peppy_maps.begin(), this->peppy_maps.end(), map_hash);
        if(it != this->peppy_maps.end()) {
            this->deleted_maps.push_back(map_hash);
        }
    }
}

void Collection::rename_to(std::string new_name) {
    if(new_name.length() < 1) new_name = "Untitled collection";
    if(this->name == new_name) return;

    auto new_collection = get_or_create_collection(new_name);

    for(auto map : this->maps) {
        this->remove_map(map);
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
    const double startTime = Timing::getTimeReal();

    unload_collections();

    auto osu_database_version = cv::database_version.getInt();

    std::string peppy_collections_path = cv::osu_folder.getString();
    peppy_collections_path.append("collection.db");

    BanchoFile::Reader peppy_collections(peppy_collections_path.c_str());
    if(peppy_collections.total_size > 0) {
        u32 version = peppy_collections.read<u32>();
        u32 nb_collections = peppy_collections.read<u32>();

        if(version > osu_database_version) {
            debugLog("osu!stable collection.db version more recent than neosu, loading might fail.\n");
        }

        for(int c = 0; c < nb_collections; c++) {
            auto name = peppy_collections.read_string();
            u32 nb_maps = peppy_collections.read<u32>();

            auto collection = get_or_create_collection(name);
            collection->maps.reserve(nb_maps);
            collection->peppy_maps.reserve(nb_maps);

            for(int m = 0; m < nb_maps; m++) {
                auto map_hash = peppy_collections.read_hash();
                collection->maps.push_back(map_hash);
                collection->peppy_maps.push_back(map_hash);
            }
        }
    }

    BanchoFile::Reader neosu_collections("collections.db");
    if(neosu_collections.total_size > 0) {
        u32 version = neosu_collections.read<u32>();
        u32 nb_collections = neosu_collections.read<u32>();

        if(version > COLLECTIONS_DB_VERSION) {
            debugLog("neosu collections.db version is too recent! Cannot load it without stuff breaking.\n");
            unload_collections();
            return false;
        } else if(version < COLLECTIONS_DB_VERSION) {
            // Reading from older database version: backup just in case
            auto backup_path = UString::format("collections.db.%d", version);
            BanchoFile::copy("collections.db", backup_path.toUtf8());
        }

        for(int c = 0; c < nb_collections; c++) {
            auto name = neosu_collections.read_string();
            auto collection = get_or_create_collection(name);

            u32 nb_deleted_maps = 0;
            if(version >= 20240429) {
                nb_deleted_maps = neosu_collections.read<u32>();
            }

            collection->deleted_maps.reserve(nb_deleted_maps);
            for(int d = 0; d < nb_deleted_maps; d++) {
                auto map_hash = neosu_collections.read_hash();

                auto it = std::find(collection->maps.begin(), collection->maps.end(), map_hash);
                if(it != collection->maps.end()) {
                    collection->maps.erase(it);
                }

                collection->deleted_maps.push_back(map_hash);
            }

            u32 nb_maps = neosu_collections.read<u32>();
            collection->maps.reserve(collection->maps.size() + nb_maps);
            collection->neosu_maps.reserve(nb_maps);

            for(int m = 0; m < nb_maps; m++) {
                auto map_hash = neosu_collections.read_hash();

                auto it = std::find(collection->maps.begin(), collection->maps.end(), map_hash);
                if(it == collection->maps.end()) {
                    collection->maps.push_back(map_hash);
                }

                collection->neosu_maps.push_back(map_hash);
            }
        }
    }

    u32 nb_peppy = 0;
    u32 nb_neosu = 0;
    u32 nb_total = 0;
    for(auto collection : collections) {
        nb_peppy += collection->peppy_maps.size();
        nb_neosu += collection->neosu_maps.size();
        nb_total += collection->maps.size();
    }

    debugLog("peppy+neosu collections: loading took %f seconds (%d peppy, %d neosu, %d maps total)\n",
             (Timing::getTimeReal() - startTime), nb_peppy, nb_neosu, nb_total);
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
    debugLog("Osu: Saving collections ...\n");
    if(!collections_loaded) {
        debugLog("Cannot save collections since they weren't loaded properly first!\n");
        return false;
    }

    const double startTime = Timing::getTimeReal();

    BanchoFile::Writer db("collections.db");
    db.write<u32>(COLLECTIONS_DB_VERSION);

    u32 nb_collections = collections.size();
    db.write<u32>(nb_collections);

    for(auto collection : collections) {
        db.write_string(collection->name.c_str());

        u32 nb_deleted = collection->deleted_maps.size();
        db.write<u32>(nb_deleted);
        for(auto map : collection->deleted_maps) {
            db.write_string(map.hash);
        }

        u32 nb_neosu = collection->neosu_maps.size();
        db.write<u32>(nb_neosu);
        for(auto map : collection->neosu_maps) {
            db.write_string(map.hash);
        }
    }

    debugLog("collections.db: saving took %f seconds\n", (Timing::getTimeReal() - startTime));
    return true;
}
