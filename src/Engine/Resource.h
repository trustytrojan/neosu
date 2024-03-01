//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		base class for resources
//
// $NoKeywords: $res
//===============================================================================//

#ifndef RESOURCE_H
#define RESOURCE_H

#include "cbase.h"

class Resource
{
public:
	Resource();
	Resource(std::string filepath);
	virtual ~Resource() {;}

	void load();
	void loadAsync();
	void release();
	void reload();

	void interruptLoad();

	void setName(std::string name) {m_sName = name;}

	inline std::string getName() const {return m_sName;}
	inline std::string getFilePath() const {return m_sFilePath;}

	inline bool isReady() const {return m_bReady.load();}
	inline bool isAsyncReady() const {return m_bAsyncReady.load();}

protected:
	virtual void init() = 0;
	virtual void initAsync() = 0;
	virtual void destroy() = 0;

	std::string m_sFilePath;
	std::string m_sName;

	std::atomic<bool> m_bReady;
	std::atomic<bool> m_bAsyncReady;
	std::atomic<bool> m_bInterrupted;
};

#endif
