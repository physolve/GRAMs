#pragma once

struct Constants{
    static float constexpr specific_gravity{0.07};
    static float constexpr pressure_std_bar{1.0};
    static float constexpr temperature_std_K{273.15};
    static float constexpr gas_constant{8.31446};
    static float constexpr gamma_H{1.41};
    static float constexpr M_H{2.016e-3}; // kg/mol
    static int constexpr filterPointCount{128};
};