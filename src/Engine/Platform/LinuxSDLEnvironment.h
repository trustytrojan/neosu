//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		linux sdl environment
//
// $NoKeywords: $sdllinuxenv
//===============================================================================//

#ifdef __linux__

#ifndef LINUXSDLENVIRONMENT_H
#define LINUXSDLENVIRONMENT_H

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

class LinuxSDLEnvironment : public SDLEnvironment {
   public:
    LinuxSDLEnvironment();
    virtual ~LinuxSDLEnvironment() { ; }

    // system
    virtual OS getOS();
    virtual void openURLInDefaultBrowser(UString url);

    // user
    virtual UString getUsername();
    virtual std::string getUserDataPath();

    // file IO
    virtual bool directoryExists(std::string directoryName);
    virtual bool createDirectory(std::string directoryName);
    virtual std::vector<UString> getFilesInFolder(UString folder);
    virtual std::vector<UString> getFoldersInFolder(UString folder);
    virtual std::vector<UString> getLogicalDrives();
    virtual std::string getFolderFromFilePath(std::string filepath);

   private:
    static int getFilesInFolderFilter(const struct dirent *entry);
    static int getFoldersInFolderFilter(const struct dirent *entry);
};

#endif

#endif

#endif
