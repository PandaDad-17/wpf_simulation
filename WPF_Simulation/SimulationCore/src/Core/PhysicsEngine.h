#pragma once

#include "../Shared/SimulationTypes.h"

#ifdef NATIVECORE_EXPORTS
#define NATIVE_API __declspec(dllexport)
#else
#define NATIVE_API __declspec(dllimport)
#endif

// Thread-safe, object-oriented C++ implementation hidden from C#
class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();

    void Initialize(const SimulationConfig& config);
    void Step(float delta_time);

    // Copy out performance data to a pre-allocated pointer safely
    int32_t GetActiveSphereData(SphereData* out_buffer, int32_t buffer_capacity);
    void UpdateConfig(const SimulationConfig& config);

private:
    SimulationConfig m_config;
    SphereData* m_spheres;
    int32_t m_current_sphere_count;

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