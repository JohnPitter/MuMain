mu_add_test(
    NAME test_poseidon_model_contract
    SOURCES test_poseidon_model_contract.cpp
)

mu_add_test(
    NAME test_poseidon_catalog
    SOURCES
        test_poseidon_catalog.cpp
        ../../src/source/Network/Server/EquipmentBonusCatalogPacket.cpp
        ../../src/source/Character/EquipmentBonusCatalog.cpp
        ../../src/source/Character/EquipmentCatalogCache.cpp
)
