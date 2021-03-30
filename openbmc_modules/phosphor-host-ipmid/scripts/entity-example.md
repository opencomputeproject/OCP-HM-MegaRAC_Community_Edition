If your platform requires the entity container map, you can provide a json file of the format:

```
[
  {
     "id" : 1,
     "containerEntityId" : 2,
     "containerEntityInstance" : 3,
     "isList" : false,
     "isLinked" : false,
     "entities" : [
         {"id" : 1, "instance" : 2},
         {"id" : 1, "instance" : 3},
         {"id" : 1, "instance" : 4},
         {"id" : 1, "instance" : 5}
     ]
  }
]
```

as part of your `phosphor-ipmi-config`

The above json is identical to the original YAML documented below:

```
# This record has:
# Container Entity Id and Container Entity Instance = (0x13, 0x81)
# Contained Entity Id and Contained Entity Instance = (0x0A, 0x1),
# (0x0A, 0x3), (0x0A, 0x5), (0x0A, 0x7)
# Entity Record id is the key
0x01:
  # Container entity contains other entities
  # Entity Id and entity Instance for the container entity
  containerEntityId: 0x13
  containerEntityInstance: 0x81
  # A record can have contained entities as a four entry list or as upto
  # two ranges of entity instances; this record has contained entities
  # as a four entry list
  isList: "true"
  # Records can be linked if necessary to extend the number of contained
  # entities within a container entity; this record is not linked
  isLinked: "false"
  entityId1: 0x0A
  entityInstance1: 0x1
  entityId2: 0x0A
  entityInstance2: 0x3
  entityId3: 0x0A
  entityInstance3: 0x5
  entityId4: 0x0A
  entityInstance4: 0x7

# The below two records have:
# Container Entity Id and Container Entity Instance = (0x18, 0x2)
# Contained Entity Id and Contained Entity Instance = (0x1D, 0x1),
# (0x1D, 0x4), (0x1D, 0x6), (0x2B, 0x1), (0x2B, 0x3), (0x0F, 0x1),
# (0x0F, 0x3), (0x10, 0x5)
0x02:
  containerEntityId: 0x18
  containerEntityInstance: 0x2
  # This record  has contained entities as a four entry list
  isList: "true"
  # This record is linked with the below record; this record and the
  # below record have the same container entity Id and container entity
  # instance;
  isLinked: "true"
  entityId1: 0x1D
  entityInstance1: 0x1
  entityId2: 0x1D
  entityInstance2: 0x4
  entityId3: 0x1D
  entityInstance3: 0x6
  entityId4: 0x2B
  entityInstance4: 0x1

0x03:
  containerEntityId: 0x18
  containerEntityInstance: 0x2
  # This record  has contained entities as a four entry list
  isList: "true"
  # This record is linked with the above record; this record and the
  # above record have the same container entity Id and container entity
  # instance
  isLinked: "true"
  entityId1: 0x2B
  entityInstance1: 0x3
  entityId2: 0x0F
  entityInstance2: 0x1
  entityId3: 0x0F
  entityInstance3: 0x3
  entityId4: 0x10
  entityInstance4: 0x5

# This record has:
# Container Entity Id and Container Entity Instance = (0x1E, 0x1)
# Contained Entity Id and Contained Entity Instance = (0x20, 0x1),
# (0x20, 0x2), (0x20, 0x3), (0x20, 0x7), (0x20, 0x8), (0x20, 0x9)
0x04:
  containerEntityId: 0x1E
  containerEntityInstance: 0x1
  # This record has contained entities as two ranges of entity instances
  isList: "false"
  # This record is not linked
  isLinked: "false"
  entityId1: 0x20
  entityInstance1: 0x1
  entityId2: 0x20
  entityInstance2: 0x3
  entityId3: 0x20
  entityInstance3: 0x7
  entityId4: 0x20
  entityInstance4: 0x9

# The below two records have:
# Container Entity Id and Container Entity Instance = (0x1E, 0x3)
# Contained Entity Id and Contained Entity Instance = (0x20, 0x1),
# (0x20, 0x2), (0x20, 0x3), (0x20, 0x6), (0x20, 0x7), (0x20, 0x8),
# (0x20, 0xA), (0x20, 0xB), (0x20, 0xD), (0x20, 0xE), (0x20, 0xF)
0x05:
  containerEntityId: 0x1E
  containerEntityInstance: 0x03
  # This record has contained entities as two ranges of entity instances
  isList: "false"
  # This record is linked with the below record; this record and the
  # below record have the same container entity Id and container entity
  # instance;
  isLinked: "true"
  entityId1: 0x20
  entityInstance1: 0x1
  entityId2: 0x20
  entityInstance2: 0x3
  entityId3: 0x20
  entityInstance3: 0x6
  entityId4: 0x20
  entityInstance4: 0x8

0x06:
  containerEntityId: 0x1E
  containerEntityInstance: 0x03
  # This record has contained entities as two ranges of entity instances
  isList: "false"
  # This record is linked with the above record; this record and the
  # above record have the same container entity Id and container entity
  # instance;
  isLinked: "true"
  entityId1: 0x20
  entityInstance1: 0xA
  entityId2: 0x20
  entityInstance2: 0xB
  entityId3: 0x20
  entityInstance3: 0xD
  entityId4: 0x20
  entityInstance4: 0xF
```
