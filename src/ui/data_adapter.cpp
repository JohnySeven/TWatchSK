#include "data_adapter.h"
#include <vector>

std::vector<DataAdapter *> adapters;

std::vector<DataAdapter *> &DataAdapter::get_adapters()
{
    return adapters;
}

DataAdapter::DataAdapter(String sk_path, int sk_subscription_period, Component*target)
{
    targetObject_ = target;
    path = sk_path;
    subscription_period = sk_subscription_period;
    adapters.push_back(this);
}