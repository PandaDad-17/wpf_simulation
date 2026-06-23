#include "pch.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

PhysicsEngine::PhysicsEngine()
    : m_spheres(nullptr)
    , m_current_sphere_count(0)
{
    m_config = { 
        .max_spheres = 0,
        .sphere_radius = 0.0f,
        .time_scale = 1.0f,
        .box_side_length = 0.0f
    };
}

PhysicsEngine::~PhysicsEngine()
{
    if (m_spheres)
    {
        delete[] m_spheres;
        m_spheres = nullptr;
    }
}

void PhysicsEngine::Initialize(const SimulationConfig& config)
{
    // Clean up existing buffer if size changes or re-initializing
    if (m_spheres)
    {
        delete[] m_spheres;
        m_spheres = nullptr;
    }

    m_config = config;
    m_current_sphere_count = config.max_spheres;

    // Pre-allocate contiguous block for zero-copy window exposure
    m_spheres = new SphereData[m_config.max_spheres];

    // Seed for deterministic generation
    srand(42);

    float half_box = m_config.box_side_length / 2.0f;

    for (int32_t i = 0; i < m_current_sphere_count; ++i)
    {
        m_spheres[i].id = i;
        m_spheres[i].radius = m_config.sphere_radius;

        // Randomize positions inside the bounding cube bounds
        float padding = m_spheres[i].radius * 1.5f;
        m_spheres[i].position.x = -half_box + padding + static_cast<float>(rand()) / (RAND_MAX / (m_config.box_side_length - 2 * padding));
        m_spheres[i].position.y = -half_box + padding + static_cast<float>(rand()) / (RAND_MAX / (m_config.box_side_length - 2 * padding));
        m_spheres[i].position.z = -half_box + padding + static_cast<float>(rand()) / (RAND_MAX / (m_config.box_side_length - 2 * padding));

        // Randomize velocities (-5.0 to +5.0 units/sec)
        m_spheres[i].velocity.x = -5.0f + static_cast<float>(rand()) / (RAND_MAX / 10.0f);
        m_spheres[i].velocity.y = -5.0f + static_cast<float>(rand()) / (RAND_MAX / 10.0f);
        m_spheres[i].velocity.z = -5.0f + static_cast<float>(rand()) / (RAND_MAX / 10.0f);
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

        for (int32_t i = 0; i < m_current_sphere_count; ++i)
            m_spheres[i].radius = config.sphere_radius;
    }
}

void PhysicsEngine::Step(float delta_time)
{
    if (!m_spheres || (m_current_sphere_count <= 0))
        return;

    // Apply time scaling multiplier (0.5x - 2.0x)
    float dt = delta_time * m_config.time_scale;

    // Phase 1: Integrate positions
    for (int32_t i = 0; i < m_current_sphere_count; ++i)
    {
        m_spheres[i].position.x += m_spheres[i].velocity.x * dt;
        m_spheres[i].position.y += m_spheres[i].velocity.y * dt;
        m_spheres[i].position.z += m_spheres[i].velocity.z * dt;
    }

    // Phase 2: Resolve internal collisions & box boundary constraints
    ResolveCollisions();
    ApplyBoundaryConstraints();
}

void PhysicsEngine::ApplyBoundaryConstraints()
{
    float half_box = m_config.box_side_length * 0.5f;
    for (int32_t i = 0; i < m_current_sphere_count; ++i)
    {
        SphereData& s = m_spheres[i];

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
    for (int32_t i = 0; i < m_current_sphere_count; ++i)
    {
        for (int32_t j = i + 1; j < m_current_sphere_count; ++j)
        {
            SphereData& s1 = m_spheres[i];
            SphereData& s2 = m_spheres[j];

            float dx = s2.position.x - s1.position.x;
            float dy = s2.position.y - s1.position.y;
            float dz = s2.position.z - s1.position.z;

            float distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
            float min_distance = s1.radius + s2.radius;

            if ((distance < min_distance) && (distance > 0.0f))
            {
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
                    impulse_scalar /= 2.0f; // Assuming equal mass matrices

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