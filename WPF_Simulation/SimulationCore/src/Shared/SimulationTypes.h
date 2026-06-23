#pragma once

#include <cstdint>

// Explicitly aligned 3D Vector matching C# System.Numerics.Vector3
struct alignas(16) Vector3D
{
    float x;
    float y;
    float z;
    float padding; // Ensures 16-byte alignment for SIMD optimization
};

// The core layout of a single sphere's render data
struct SphereData
{
    int32_t id;
    float radius;
    Vector3D position;
    Vector3D velocity;
};

// Configuration payload passed from WPF to C++
struct SimulationConfig
{
    int32_t max_spheres;
    float sphere_radius;
    float time_scale; // 0.5x to 2.0x
    float box_side_length;
};