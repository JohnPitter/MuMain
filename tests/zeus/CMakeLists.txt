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
    SOURCES
        test_zeus_finish.cpp
        ../../src/source/Render/Models/ZeusFinish.cpp
)
