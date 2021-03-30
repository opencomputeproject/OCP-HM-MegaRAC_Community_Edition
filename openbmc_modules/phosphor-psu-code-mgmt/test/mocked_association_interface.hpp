#pragma once

#include "association_interface.hpp"

#include <gmock/gmock.h>

class MockedAssociationInterface : public AssociationInterface
{
  public:
    virtual ~MockedAssociationInterface() = default;

    MOCK_METHOD1(createActiveAssociation, void(const std::string& path));
    MOCK_METHOD1(addFunctionalAssociation, void(const std::string& path));
    MOCK_METHOD1(addUpdateableAssociation, void(const std::string& path));
    MOCK_METHOD1(removeAssociation, void(const std::string& path));
};
