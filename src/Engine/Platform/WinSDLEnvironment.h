#pragma once
#ifdef _WIN32

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class WinSDLEnvironment : public SDLEnvironment {
   public:
    WinSDLEnvironment();
    virtual ~WinSDLEnvironment() { ; }

    // system
    virtual OS getOS();
    virtual void sleep(unsigned int us);
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
    virtual std::string getFileNameFromFilePath(std::string filePath);

    // dialogs & message boxes
    virtual UString openFileWindow(const char *filetypefilters, UString title, UString initialpath);
    virtual UString openFolderWindow(UString title, UString initialpath);

   private:
    void path_strip_filename(TCHAR *Path);
};

#endif

#endif
