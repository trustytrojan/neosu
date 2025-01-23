//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#include "Resource.h"

#include "Engine.h"
#include "Environment.h"

Resource::Resource(std::string filepath) {
    this->sFilePath = filepath;

    if(this->sFilePath.length() > 0 && !env->fileExists(this->sFilePath)) {
        UString errorMessage = "File does not exist: ";
        errorMessage.append(this->sFilePath.c_str());
        debugLog("Resource Warning: File %s does not exist!\n", this->sFilePath.c_str());
        /// engine->showMessageError("Resource Error", errorMessage);

        // HACKHACK: workaround retry different case variations due to linux fs case sensitivity
        if(env->getOS() == Environment::OS::LINUX) {
            // NOTE: this assumes that filepaths in code are always fully lowercase
            // better than not doing/trying anything though

            // try loading a toUpper() version of the file extension

            // search backwards from end to first dot, then toUpper() forwards till end of string
            for(int s = this->sFilePath.size(); s >= 0; s--) {
                if(this->sFilePath[s] == '.') {
                    for(int i = s + 1; i < this->sFilePath.size(); i++) {
                        this->sFilePath[i] = std::toupper(this->sFilePath[i]);
                    }
                    break;
                }
            }

            if(!env->fileExists(this->sFilePath)) {
                // if still not found, try with toLower() filename (keeping uppercase extension)

                // search backwards from end to first dot, then toLower() everything until first slash
                bool foundFilenameStart = false;
                for(int s = this->sFilePath.size(); s >= 0; s--) {
                    if(foundFilenameStart) {
                        if(this->sFilePath[s] == '/') break;
                        this->sFilePath[s] = std::tolower(this->sFilePath[s]);
                    }

                    if(this->sFilePath[s] == '.') {
                        foundFilenameStart = true;
                    }
                }

                if(!env->fileExists(this->sFilePath)) {
                    // last chance, try with toLower() filename + extension

                    // toLower() backwards until first slash
                    for(int s = this->sFilePath.size(); s >= 0; s--) {
                        if(this->sFilePath[s] == '/') {
                            break;
                        }

                        this->sFilePath[s] = std::tolower(this->sFilePath[s]);
                    }
                }
            }
        }
    }

    this->bReady = false;
    this->bAsyncReady = false;
    this->bInterrupted = false;
}

Resource::Resource() {
    this->bReady = false;
    this->bAsyncReady = false;
    this->bInterrupted = false;
}

void Resource::load() { this->init(); }

void Resource::loadAsync() { this->initAsync(); }

void Resource::reload() {
    this->release();
    this->loadAsync();  // TODO: this should also be reloaded asynchronously if it was initially loaded so, maybe
    this->load();
}

void Resource::release() {
    this->destroy();

    // NOTE: these are set afterwards on purpose
    this->bReady = false;
    this->bAsyncReady = false;
}

void Resource::interruptLoad() { this->bInterrupted = true; }
