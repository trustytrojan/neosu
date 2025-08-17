// Copyright (c) 2025, WH, All rights reserved.
#pragma once

#include <map>
#include <set>
#include <utility>

#include "Image.h"

class AvatarManager final {
   public:
    AvatarManager() = default;
    ~AvatarManager() { this->clear(); }

    AvatarManager& operator=(const AvatarManager&) = delete;
    AvatarManager& operator=(AvatarManager&&) = delete;
    AvatarManager(const AvatarManager&) = delete;
    AvatarManager(AvatarManager&&) = delete;

    // this is run during Osu::update(), while not in unpaused gameplay
    void update();

    // called by UIAvatar to add new user id/folder avatar pairs to the loading queue (and tracking)
    void add_avatar(const std::pair<i32, std::string>& id_folder);

    // may return null if avatar is still loading
    [[nodiscard]] Image* get_avatar(const std::pair<i32, std::string>& id_folder);

   private:
    // only keep this many avatar Image resources loaded in VRAM at once
    static constexpr size_t MAX_LOADED_AVATARS{192};

    struct AvatarEntry {
        std::string file_path;
        Image* image{nullptr};  // null if not loaded in memory
        double last_access_time{0.0};
        bool is_downloading{false};
    };

    void prune_oldest_avatars();
    bool download_avatar(const std::pair<i32, std::string>& id_folder);
    void load_avatar_image(AvatarEntry& entry);
    void clear();

    // all AvatarEntries added through add_avatar remain alive forever, but the actual Image resource
    // it references will be unloaded (by priority of access time) to keep VRAM/RAM usage sustainable
    std::map<std::pair<i32, std::string>, AvatarEntry> avatars;
    std::deque<std::pair<i32, std::string>> download_queue;
    std::set<std::pair<i32, std::string>> id_blacklist;
    std::set<std::pair<i32, std::string>> newly_downloaded;
};