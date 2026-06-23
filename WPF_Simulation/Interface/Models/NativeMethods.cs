using System;
using System.Runtime.InteropServices;
using System.Numerics;

namespace Interface.Models
{
    [StructLayout(LayoutKind.Explicit, Size = 48)]
    public struct SphereData
    {
        [FieldOffset(0)] public int Id;
        [FieldOffset(4)] public float Radius;

        // Offset 8-15 is the alignment padding from C++ struct layout

        [FieldOffset(16)] public Vector3 Position;  // Matches C++ alignas(16) boundary
        [FieldOffset(32)] public Vector3 Velocity;  // Matches second Vector3D alignment
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SimulationConfig
    {
        public int MaxSpheres;
        public float SphereRadius;
        public float TimeScale;
        public float BoxSideLength;
    }

    public static unsafe class NativeMethods
    {
        private const string DllName = "SimulationCore.dll";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr CreateEngine();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void DestroyEngine(IntPtr instance);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void InitializeEngine(IntPtr instance, SimulationConfig config);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void UpdateEngineConfig(IntPtr instance, SimulationConfig config);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void StepSimulation(IntPtr instance, float deltaTime);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int GetSimulationState(IntPtr instance, out SphereData* outBufferPtr);
    }
}