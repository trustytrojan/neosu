//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#ifndef RESOURCE_H
#define RESOURCE_H

#include "cbase.h"

class Resource {
   public:
    Resource();
    Resource(std::string filepath);
    virtual ~Resource() { ; }

    void load();
    void loadAsync();
    void release();
    void reload();

    void interruptLoad();

    void setName(std::string name) { this->sName = name; }

    [[nodiscard]] inline std::string getName() const { return this->sName; }
    [[nodiscard]] inline std::string getFilePath() const { return this->sFilePath; }

    [[nodiscard]] inline bool isReady() const { return this->bReady.load(); }
    [[nodiscard]] inline bool isAsyncReady() const { return this->bAsyncReady.load(); }

   protected:
    virtual void init() = 0;
    virtual void initAsync() = 0;
    virtual void destroy() = 0;

    std::string sFilePath;
    std::string sName;

    std::atomic<bool> bReady;
    std::atomic<bool> bAsyncReady;
    std::atomic<bool> bInterrupted;
};

#endif
