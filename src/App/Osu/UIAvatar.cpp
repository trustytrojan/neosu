#include "UIAvatar.h"

#include <stdlib.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <sstream>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Downloader.h"
#include "Engine.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "UIUserContextMenu.h"

// Returns true when avatar is fully downloaded
bool download_avatar(u32 user_id) {
    if(user_id == 0) return false;

    // XXX: clear blacklist when changing endpoint
    static std::vector<u32> blacklist;
    for(auto bl : blacklist) {
        if(user_id == bl) {
            return false;
        }
    }

    auto server_dir = UString::format(MCENGINE_DATA_DIR "avatars/%s", bancho->endpoint.toUtf8());
    if(!env->directoryExists(server_dir.toUtf8())) {
        env->createDirectory(server_dir.toUtf8());
    }

    float progress = -1.f;
    std::vector<u8> data;
    auto scheme = cv_use_https.getBool() ? "https://" : "http://";
    auto img_url = UString::format("%sa.%s/%d", scheme, bancho->endpoint.toUtf8(), user_id);
    int response_code;
    download(img_url.toUtf8(), &progress, data, &response_code);
    if(progress == -1.f) blacklist.push_back(user_id);
    if(data.empty()) return false;

    auto img_path = UString::format("%s/%d", server_dir.toUtf8(), user_id);
    FILE *file = fopen(img_path.toUtf8(), "wb");
    if(file != NULL) {
        fwrite(data.data(), data.size(), 1, file);
        fflush(file);
        fclose(file);
    }

    // NOTE: We return true even if progress is -1. Because we still get avatars from a 404!
    return true;
}

UIAvatar::UIAvatar(u32 player_id, float xPos, float yPos, float xSize, float ySize)
    : CBaseUIButton(xPos, yPos, xSize, ySize, "avatar", "") {
    this->player_id = player_id;

    this->avatar_path = UString::format(MCENGINE_DATA_DIR "avatars/%s/%d", bancho->endpoint.toUtf8(), player_id).toUtf8();
    this->setClickCallback(fastdelegate::MakeDelegate(this, &UIAvatar::onAvatarClicked));

    struct stat attr;
    bool exists = (stat(this->avatar_path.c_str(), &attr) == 0);
    if(exists) {
        // File exists, but if it's more than 7 days old assume it's expired
        time_t now = time(NULL);
        struct tm expiration_date = *localtime(&attr.st_mtime);
        expiration_date.tm_mday += 7;
        if(now > mktime(&expiration_date)) {
            exists = false;
        }
    }

    if(exists) {
        this->avatar = resourceManager->loadImageAbs(this->avatar_path, this->avatar_path);
    }
}

UIAvatar::~UIAvatar() {
    // XXX: leaking avatar Resource here, because we don't know in how many places it will be reused
}

void UIAvatar::draw_avatar(float alpha) {
    if(!this->on_screen) return;  // Comment when you need to debug on_screen logic

    if(this->avatar == NULL) {
        // Don't download during gameplay to avoid lagspikes
        if(!osu->isInPlayMode()) {
            if(download_avatar(this->player_id)) {
                this->avatar = resourceManager->loadImageAbs(this->avatar_path, this->avatar_path);
            }
        }
    } else {
        g->pushTransform();
        g->setColor(0xffffffff);
        g->setAlpha(alpha);
        g->scale(this->vSize.x / this->avatar->getWidth(), this->vSize.y / this->avatar->getHeight());
        g->translate(this->vPos.x + this->vSize.x / 2.0f, this->vPos.y + this->vSize.y / 2.0f);
        g->drawImage(this->avatar);
        g->popTransform();
    }

    // For debugging purposes
    // if(on_screen) {
    //     g->pushTransform();
    //     g->setColor(0xff00ff00);
    //     g->drawQuad((int)this->vPos.x, (int)this->vPos.y, (int)this->vSize.x, (int)this->vSize.y);
    //     g->popTransform();
    // } else {
    //     g->pushTransform();
    //     g->setColor(0xffff0000);
    //     g->drawQuad((int)this->vPos.x, (int)this->vPos.y, (int)this->vSize.x, (int)this->vSize.y);
    //     g->popTransform();
    // }
}

void UIAvatar::onAvatarClicked(CBaseUIButton *btn) {
    if(osu->isInPlayMode()) {
        // Don't want context menu to pop up while playing a map
        return;
    }

    osu->user_actions->open(this->player_id);
}
