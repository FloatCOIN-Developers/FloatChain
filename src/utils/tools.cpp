// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"

#include <iosfwd>
#include <string>
#include <map>

using namespace std;


void FC_MapStringIndex::Init()
{
    Destroy();
    mapObject=new std::map<string, int>;
}

void FC_MapStringIndex::Destroy()
{
    if(mapObject)
    {
        delete (std::map<string, int>*)mapObject;
    }
}

void FC_MapStringIndex::Clear()
{
    Init();
}

void FC_MapStringIndex::Add(const char* key, int value)
{
    ((std::map<string, int>*)mapObject)->insert(std::pair<string, int>(string(key), value));
}

void FC_MapStringIndex::Add(const unsigned char* key, int size, int value)
{
    ((std::map<string, int>*)mapObject)->insert(std::pair<string, int>(string(key,key+size), value));
}

void FC_MapStringIndex::Remove(const char* key,int size)
{
    ((std::map<string, int>*)mapObject)->erase(string(key,key+size));
}

int FC_MapStringIndex::Get(const char* key)
{
    std::map<string, int>::iterator it;
    int value=-1;
    it=((std::map<string, int>*)mapObject)->find(string(key));
    if (it != ((std::map<string, int>*)mapObject)->end())
    {
        value=it->second;
    }    
    return value;
}

int FC_MapStringIndex::Get(const unsigned char* key,int size)
{
    std::map<string, int>::iterator it;
    int value=-1;
    it=((std::map<string, int>*)mapObject)->find(string(key,key+size));
    if (it != ((std::map<string, int>*)mapObject)->end())
    {
        value=it->second;
    }    
    return value;
}

void FC_MapStringString::Init()
{
    mapObject=new std::map<string, string>;
}

void FC_MapStringString::Destroy()
{
    if(mapObject)
    {
        delete (std::map<string, string>*)mapObject;
    }
}

void FC_MapStringString::Add(const char* key, const char*  value)
{
    ((std::map<string, string>*)mapObject)->insert(std::pair<string, string>(string(key), value));
}

const char* FC_MapStringString::Get(const char* key)
{
    std::map<string, string>::iterator it;
    it=((std::map<string, string>*)mapObject)->find(string(key));
    if (it != ((std::map<string, string>*)mapObject)->end())
    {
         return it->second.c_str();
    }    
    return NULL;
}

int FC_MapStringString::GetCount()
{
    return ((std::map<string, string>*)mapObject)->size();
}