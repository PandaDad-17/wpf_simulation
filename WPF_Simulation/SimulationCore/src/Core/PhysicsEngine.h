#pragma once

#include <vector>

#include "../Shared/SimulationTypes.h"

#ifdef SIMULATIONCORE_EXPORTS
    #define NATIVE_API __declspec(dllexport)
#else
    #define NATIVE_API __declspec(dllimport)
#endif

// Thread-safe, object-oriented C++ implementation hidden from C#
class PhysicsEngine {
public:
    PhysicsEngine();

    void Initialize(const SimulationConfig& config);
    void Step(float delta_time);

    void UpdateConfig(const SimulationConfig& config);

    int32_t GetSphereCount() const { return static_cast<int32_t>(m_spheres.size()); }
    SphereData* GetSpheresBuffer() { return m_spheres.data(); }

private:
    SimulationConfig m_config;
    std::vector<SphereData> m_spheres;

    void ResolveCollisions();
    void ApplyBoundaryConstraints();
};

// ============================================================================
// EXPORTED C-INTERFACE FOR P/INVOKE
// ============================================================================
extern "C" {

    NATIVE_API PhysicsEngine* CreateEngine();

    NATIVE_API void DestroyEngine(PhysicsEngine* instance);

    NATIVE_API void InitializeEngine(PhysicsEngine* instance, SimulationConfig config);

    NATIVE_API void UpdateEngineConfig(PhysicsEngine* instance, SimulationConfig config);

    NATIVE_API void StepSimulation(PhysicsEngine* instance, float delta_time);

    // Zero-Copy Direct Memory Access Strategy:
    // Returns a raw pointer to the internal state buffer. 
    // WPF will read this directly using an unsafe pointer context.
    NATIVE_API int32_t GetSimulationState(PhysicsEngine* instance, SphereData** out_buffer_ptr);

}