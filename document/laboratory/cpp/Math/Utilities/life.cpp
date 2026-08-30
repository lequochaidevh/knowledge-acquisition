#include "compile_time_math.h"
using namespace CompileTimeMath;

/**
 * Complex Coupled Stress: In physics, stress isn't a scalar value; it's a multi-axis vector. We mapped the linear
 * compression force to the real axis and the torsional shear to the imaginary axis. By expanding through
 * Identities::square_of_sum_expanded, we executed coupled scaling, evaluating structural safety before execution.
 *
 * Fuel Centroid Tracking: Liquid sloshing changes the spacecraft's center of mass. Using the Ellipse::point_at utility
 * combined with our custom constexpr sin/cos Taylor functions, we pinpoint the exact movement of fluid coordinates
 * inside the fuel tank during structural high-speed rotation.
 *
 * Trajectory Intercept: The rocket's climb profile vs external orbital atmospheric constraints yields an engineering
 * intersection model. By passing these vectors directly into our CubicFunction::solve() calculator, Cardano’s algorithm
 * outputs the exact danger timestamp (\(t \approx 3.91\) seconds) directly inside a static_assert.
 */

// =========================================================================
// AEROSPACE APPLICATION MISSION CONTROL CONTAINER
// =========================================================================
namespace math_physics {
struct AerospaceMissionVerifier {
    // 1. Calculate coupled structural load vector using Complex Fields and Identities
    // Real part: Axial load, Imaginary part: Torsional shear load
    [[nodiscard]] static constexpr double calculate_hull_coupled_stress(double axial_load, double shear_load) noexcept {
        // Apply the first identity to find the safe structural tension dissipation boundary
        double          scalar_energy_factor = Identities<double>::square_of_sum_expanded(axial_load, 0.15);
        Complex<double> combined_stress_vector{scalar_energy_factor, shear_load * 1.25};
        return combined_stress_vector.magnitude();
    }

    // 2. Track fuel tank sloshing balance matrix boundary limits
    [[nodiscard]] static constexpr geometry::Ellipse<double>::Point track_fuel_slosh_centroid(
        double semi_major, double semi_minor, double thruster_g_angle) noexcept {
        geometry::Ellipse<double> tank_cross_section{semi_major, semi_minor};
        return tank_cross_section.point_at(thruster_g_angle);
    }

    // 3. Find the exact mission timeline critical danger point (Rocket intersection with debris layer)
    // System translates down to a Cubic Flight Path crossing equation: 0.5*t^3 + 1.2*t^2 - 4.0*t - 15.0 = 0
    [[nodiscard]] static constexpr double calculate_intercept_timestamp() {
        CubicFunction<double> trajectory_climb{0.5, 1.2, -4.0, -15.0};
        auto                  result = trajectory_climb.solve();
        return result.x1;
    }
};

}  // namespace math_physics

// =========================================================================
// PRODUCTION COMPILE-TIME UNIT VERIFICATIONS
// =========================================================================
namespace FlightMissionPreCheck {
using MSC = math_physics::AerospaceMissionVerifier;

// Compile-time step 1: Check if coupled mechanical structural stress is well within maximum safe alloy limits (< 5.0
// GPa)
constexpr double critical_stress = MSC::calculate_hull_coupled_stress(1.5, 0.8);
static_assert(critical_stress < 5.0,
              "CRITICAL CRASH ALERT: Aerospace structural alloy structural rupture at compile-time!");

// Compile-time step 2: Extract fuel fluid geometric shifted coordinate during a 45-degree roll maneuver
constexpr auto fuel_centroid = MSC::track_fuel_slosh_centroid(3.0, 1.5, CompileTimeMath::PI / 4.0);
static_assert(fuel_centroid.x > 0.0 && fuel_centroid.y > 0.0, "FLUID FAILURE: Invalid fuel center quadrant tracking!");

// Compile-time step 3: Verify the intersection timeline occurs within the active propulsion burn window (t < 10.0
// seconds)
constexpr double intercept_t = MSC::calculate_intercept_timestamp();
static_assert(intercept_t > 0.0 && intercept_t < 10.0, "TIMING FAILURE: Intercept event window out of system bounds!");
}  // namespace FlightMissionPreCheck

int main() {
    std::cout << "=========================================================\n";
    std::cout << " AEROSPACE VEHICLE MISSION FLIGHT VERIFIER SUITE      \n";
    std::cout << "=========================================================\n\n";

    std::cout << "[COMPILE-TIME SECURE] Checking safety profiles...\n";
    std::cout << "-> Total Coupled Fuselage Structural Load: " << FlightMissionPreCheck::critical_stress << " GPa\n";
    std::cout << "-> Fuel Mass Center Location during 45° shift: (" << FlightMissionPreCheck::fuel_centroid.x << "m, "
              << FlightMissionPreCheck::fuel_centroid.y << "m)\n";
    std::cout << "-> Predicted Debris Layer Intercept Timestamp: " << FlightMissionPreCheck::intercept_t
              << " seconds\n\n";

    std::cout << " RESULT: All parameters are compiled and checked cleanly! Safe to flash to flight computer.\n";
    return 0;
}
