#include "observableobject.h"

static std::map<String, ObservableObject *> objectList;

ObservableObject::ObservableObject(String name)
{
    this->object_name = name;
    objectList[name] = this;
}

ObservableObject* ObservableObject::get_object(String name)
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