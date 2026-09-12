mu_add_test(
    NAME test_zeus_model_contract
    SOURCES test_zeus_model_contract.cpp
)

mu_add_test(
    NAME test_zeus_cloth_gate
    SOURCES test_zeus_cloth_gate.cpp
)

mu_add_test(
    NAME test_zeus_finish
    SOURCES test_zeus_finish.cpp
)

mu_add_test(
    NAME test_zeus_catalog
    SOURCES
        test_zeus_catalog.cpp
        ../../src/source/Network/Server/EquipmentBonusCatalogPacket.cpp
        ../../src/source/Character/EquipmentBonusCatalog.cpp
        ../../src/source/Character/EquipmentCatalogCache.cpp
)

mu_add_test(
    NAME test_authored_set_fx
    SOURCES test_authored_set_fx.cpp
)
