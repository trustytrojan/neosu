//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		macOS SDL environment
//
// $NoKeywords: $macsdlenv
//===============================================================================//

#ifdef __APPLE__

#ifndef MACOSSDLENVIRONMENT_H
#define MACOSSDLENVIRONMENT_H

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

class MacOSEnvironment;

class MacOSSDLEnvironment : public SDLEnvironment {
   public:
    MacOSSDLEnvironment();
    virtual ~MacOSSDLEnvironment();

    // system
    OS getOS();
    void sleep(unsigned int us);
    void openURLInDefaultBrowser(UString url);

    // user
    UString getUsername();
    std::string getUserDataPath();

    // file IO
    bool directoryExists(std::string directoryName);
    bool createDirectory(std::string directoryName);
    std::vector<UString> getFilesInFolder(UString folder);
    std::vector<UString> getFoldersInFolder(UString folder);
    std::vector<UString> getLogicalDrives();
    std::string getFolderFromFilePath(std::string filepath);

    // dialogs & message boxes
    UString openFileWindow(const char *filetypefilters, UString title, UString initialpath);
    UString openFolderWindow(UString title, UString initialpath);

    // window
    int getDPI();

   private:
    MacOSEnvironment *m_environment;
};

#endif

#endif

#endif
