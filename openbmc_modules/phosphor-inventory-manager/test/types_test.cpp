#include "../types.hpp"

/**
 * No runtime tests - just make sure we have the right headers included.
 */
using namespace phosphor::inventory::manager;

struct Empty
{
};

void functionUsingInterfaceVariantType(InterfaceVariantType&)
{
}
void functionUsingInterfaceType(InterfaceType<Empty>&)
{
}
void functionUsingObjectType(ObjectType<Empty>&)
{
}
void functionUsingInterface(Interface&)
{
}
void functionUsingObject(Object&)
{
}
void functionUsingAction(Action&)
{
}
void functionUsingFilter(Filter&)
{
}
void functionUsingPathCondition(PathCondition&)
{
}
void functionUsingGetProperty(GetProperty<Empty>&)
{
}
