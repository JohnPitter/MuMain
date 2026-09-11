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
