//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#include "Resource.h"
#include "Engine.h"
#include "Environment.h"

Resource::Resource(std::string filepath)
{
	m_sFilePath = filepath;

	if (m_sFilePath.length() > 0 && !env->fileExists(m_sFilePath))
	{
		UString errorMessage = "File does not exist: ";
		errorMessage.append(m_sFilePath.c_str());
		debugLog("Resource Warning: File %s does not exist!\n", m_sFilePath.c_str());
		///engine->showMessageError("Resource Error", errorMessage);

		// HACKHACK: workaround retry different case variations due to linux fs case sensitivity
		if (env->getOS() == Environment::OS::OS_LINUX)
		{
			// NOTE: this assumes that filepaths in code are always fully lowercase
			// better than not doing/trying anything though

			// try loading a toUpper() version of the file extension

			// search backwards from end to first dot, then toUpper() forwards till end of string
			for(int s = m_sFilePath.size(); s >= 0; s--) {
				if(m_sFilePath[s] == '.') {
					for(int i = s+1; i < m_sFilePath.size(); i++) {
						m_sFilePath[i] = std::toupper(m_sFilePath[i]);
					}
					break;
				}
			}

			if (!env->fileExists(m_sFilePath))
			{
				// if still not found, try with toLower() filename (keeping uppercase extension)

				// search backwards from end to first dot, then toLower() everything until first slash
				bool foundFilenameStart = false;
				for(int s = m_sFilePath.size(); s >= 0; s--) {
					if(foundFilenameStart) {
						if(m_sFilePath[s] == '/') break;
						m_sFilePath[s] = std::tolower(m_sFilePath[s]);
					}

					if(m_sFilePath[s] == '.') {
						foundFilenameStart = true;
					}
				}

				if (!env->fileExists(m_sFilePath))
				{
					// last chance, try with toLower() filename + extension

					// toLower() backwards until first slash
					for(int s = m_sFilePath.size(); s >= 0; s--) {
						if(m_sFilePath[s] == '/') {
							break;
						}

						m_sFilePath[s] = std::tolower(m_sFilePath[s]);
					}
				}
			}
		}
	}

	m_bReady = false;
	m_bAsyncReady = false;
	m_bInterrupted = false;
}

Resource::Resource()
{
	m_bReady = false;
	m_bAsyncReady = false;
	m_bInterrupted = false;
}

void Resource::load()
{
	init();
}

void Resource::loadAsync()
{
	initAsync();
}

void Resource::reload()
{
	release();
	loadAsync(); // TODO: this should also be reloaded asynchronously if it was initially loaded so, maybe
	load();
}

void Resource::release()
{
	destroy();

	// NOTE: these are set afterwards on purpose
	m_bReady = false;
	m_bAsyncReady = false;
}

void Resource::interruptLoad()
{
	m_bInterrupted = true;
}
