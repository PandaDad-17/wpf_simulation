#include "pch.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <random>

PhysicsEngine::PhysicsEngine()
{
    m_config = { 
        .max_spheres = 0,
        .sphere_radius = 0.0f,
        .time_scale = 1.0f,
        .box_side_length = 0.0f
    };
}

void PhysicsEngine::Initialize(const SimulationConfig& config)
{
    m_config = config;

    // Pre-allocate contiguous block for zero-copy window exposure
    m_spheres.resize(m_config.max_spheres);

    // Seed for deterministic generation
    std::mt19937 rng(42);

    float half_box = m_config.box_side_length * 0.5f;
    float padding = m_config.sphere_radius * 1.5f;

    // Randomize positions inside the bounding cube bounds
    std::uniform_real_distribution<float> pos_dist(-half_box + padding, half_box - padding);

    // Randomize velocities (-5.0 to +5.0 units/sec)
    std::uniform_real_distribution<float> vel_dist(-5.0f, 5.0f);

    int32_t index = 0;
    for (SphereData& s : m_spheres)
    {
        s.id = index++;
        s.radius = m_config.sphere_radius;

        s.position.x = pos_dist(rng);
        s.position.y = pos_dist(rng);
        s.position.z = pos_dist(rng);

        s.velocity.x = vel_dist(rng);
        s.velocity.y = vel_dist(rng);
        s.velocity.z = vel_dist(rng);
    }
}

void PhysicsEngine::UpdateConfig(const SimulationConfig& config)
{
    // If runtime settings change sphere limits, re-initialize cleanly
    if (config.max_spheres != m_config.max_spheres)
        Initialize(config);
    else
    {
        // Update live parameters on the fly
        m_config.time_scale = config.time_scale;
        m_config.box_side_length = config.box_side_length;

        for (SphereData& s : m_spheres)
            s.radius = config.sphere_radius;
    }
}

void PhysicsEngine::Step(float delta_time)
{
    if (m_spheres.empty())
        return;

    // Apply time scaling multiplier (0.5x - 2.0x)
    float dt = delta_time * m_config.time_scale;

    // Phase 1: Integrate positions
    for (SphereData& s : m_spheres)
    {
        s.position.x += s.velocity.x * dt;
        s.position.y += s.velocity.y * dt;
        s.position.z += s.velocity.z * dt;
    }

    // Phase 2: Resolve internal collisions & box boundary constraints
    ResolveCollisions();
    ApplyBoundaryConstraints();
}

void PhysicsEngine::ApplyBoundaryConstraints()
{
    float half_box = m_config.box_side_length * 0.5f;
    for (SphereData& s : m_spheres)
    {
        // X-axis check
        if ((s.position.x - s.radius) < -half_box)
        {
            s.position.x = -half_box + s.radius;
            s.velocity.x *= -1.0f;
        }
        else if ((s.position.x + s.radius) > half_box)
        {
            s.position.x = half_box - s.radius;
            s.velocity.x *= -1.0f;
        }

        // Y-axis check
        if ((s.position.y - s.radius) < -half_box)
        {
            s.position.y = -half_box + s.radius;
            s.velocity.y *= -1.0f;
        }
        else if ((s.position.y + s.radius) > half_box)
        {
            s.position.y = half_box - s.radius;
            s.velocity.y *= -1.0f;
        }

        // Z-axis check
        if ((s.position.z - s.radius) < -half_box)
        {
            s.position.z = -half_box + s.radius;
            s.velocity.z *= -1.0f;
        }
        else if ((s.position.z + s.radius) > half_box)
        {
            s.position.z = half_box - s.radius;
            s.velocity.z *= -1.0f;
        }
    }
}

void PhysicsEngine::ResolveCollisions()
{
    // O(N^2) naive check—perfect for showcasing baseline performance optimization constraints
    for (size_t i = 0; i < m_spheres.size(); ++i)
    {
        for (size_t j = i + 1; j < m_spheres.size(); ++j)
        {
            SphereData& s1 = m_spheres[i];
            SphereData& s2 = m_spheres[j];

            float dx = s2.position.x - s1.position.x;
            float dy = s2.position.y - s1.position.y;
            float dz = s2.position.z - s1.position.z;

            float distance_squared = (dx * dx) + (dy * dy) + (dz * dz);
            float min_distance = s1.radius + s2.radius;
			float min_distance_squared = min_distance * min_distance;

            if ((distance_squared < min_distance_squared) && (distance_squared > 0.0f))
            {
				float distance = std::sqrt(distance_squared);
				float inverse_distance = 1.0f / distance;

                // Normal vector of contact
                float nx = dx * inverse_distance;
                float ny = dy * inverse_distance;
                float nz = dz * inverse_distance;

                // Push apart along contact normal to prevent overlap sticking
                float overlap = min_distance - distance;
                s1.position.x -= nx * overlap * 0.5f;
                s1.position.y -= ny * overlap * 0.5f;
                s1.position.z -= nz * overlap * 0.5f;

                s2.position.x += nx * overlap * 0.5f;
                s2.position.y += ny * overlap * 0.5f;
                s2.position.z += nz * overlap * 0.5f;

                // Relative velocity vector
                float rvx = s2.velocity.x - s1.velocity.x;
                float rvy = s2.velocity.y - s1.velocity.y;
                float rvz = s2.velocity.z - s1.velocity.z;

                // Velocity along the normal
                float vel_along_normal = rvx * nx + rvy * ny + rvz * nz;

                // Do not resolve if velocities are already separating
                if (vel_along_normal < 0)
                {
                    // Elastic impulse constant (1.0 = perfect elastic bounce)
                    float restitution = 1.0f;
                    float impulse_scalar = -(1.0f + restitution) * vel_along_normal;
                    impulse_scalar *= 0.5f; // Assuming equal mass matrices

                    // Apply impulse to each object
                    s1.velocity.x -= impulse_scalar * nx;
                    s1.velocity.y -= impulse_scalar * ny;
                    s1.velocity.z -= impulse_scalar * nz;

                    s2.velocity.x += impulse_scalar * nx;
                    s2.velocity.y += impulse_scalar * ny;
                    s2.velocity.z += impulse_scalar * nz;
                }
            }
        }
    }
}

// ============================================================================
// EXPORTED C-LINKAGE WRAPPER CODES
// ============================================================================

extern "C"
{

    NATIVE_API PhysicsEngine* CreateEngine()
    {
        return new PhysicsEngine();
    }

    NATIVE_API void DestroyEngine(PhysicsEngine* instance)
    {
        if (instance)
            delete instance;
    }

    NATIVE_API void InitializeEngine(PhysicsEngine* instance, SimulationConfig config)
    {
        if (instance)
            instance->Initialize(config);
    }

    NATIVE_API void UpdateEngineConfig(PhysicsEngine* instance, SimulationConfig config)
    {
        if (instance)
            instance->UpdateConfig(config);
    }

    NATIVE_API void StepSimulation(PhysicsEngine* instance, float delta_time)
    {
        if (instance)
            instance->Step(delta_time);
    }

    NATIVE_API int32_t GetSimulationState(PhysicsEngine* instance, SphereData** out_buffer_ptr)
    {
        if (instance && out_buffer_ptr)
        {
            *out_buffer_ptr = instance->GetSpheresBuffer();
            return instance->GetSphereCount();
        }
        return 0;
    }
}