using System;
using System.Runtime.InteropServices;
using System.Numerics;

namespace Interface.Models
{
    // Sequential layout forces the CLI runtime to layout bytes identically to C++ struct alignas(16)
    [StructLayout(LayoutKind.Sequential, Pack = 16)]
    public struct SphereData
    {
        public int Id;
        public float Radius;
        public Vector3 Position; // System.Numerics.Vector3 is native blittable float x,y,z
        private float Padding;   // Matches the 16-byte alignment padding from C++
        public Vector3 Velocity;
        private float Padding2;
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

        // This is our zero-copy powerhouse. We pass a pointer to a pointer.
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int GetSimulationState(IntPtr instance, out SphereData* outBufferPtr);
    }
}