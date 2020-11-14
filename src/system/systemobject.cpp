#include "systemobject.h"

static std::map<String, SystemObject*> objectList;

SystemObject::SystemObject(String name)
{
    object_name = name;
    objectList[name] = this;
}

SystemObject* SystemObject::get_object(String name)
{
    auto iterator = objectList.find(name);

    if(iterator == objectList.end())
    {
        return NULL;
    }
    else
    {
        return iterator->second;
    }    
}