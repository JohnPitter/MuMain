mu_add_test(
    NAME test_celestial_model_contract
    SOURCES test_celestial_model_contract.cpp
)

mu_add_test(
    NAME test_equipment_state
    SOURCES
        test_equipment_state.cpp
        ../../src/source/Network/Server/EquipmentStatePacket.cpp
        ../../src/source/Character/CharacterMovementEffects.cpp
)

mu_add_test(
    NAME test_equipment_catalog
    SOURCES
        test_equipment_catalog.cpp
        ../../src/source/Network/Server/EquipmentBonusCatalogPacket.cpp
        ../../src/source/Character/EquipmentBonusCatalog.cpp
        ../../src/source/Character/EquipmentCatalogCache.cpp
        ../../src/source/UI/Items/EquipmentTooltipContent.cpp
)

mu_add_test(
    NAME test_celestial_shimmer
    SOURCES
        test_celestial_shimmer.cpp
        ../../src/source/Render/Models/CelestialShimmer.cpp
)
