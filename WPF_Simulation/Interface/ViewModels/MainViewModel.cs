using System;
using System.Windows.Media;
using System.Diagnostics;
using Interface.Models;

namespace Interface.ViewModels
{
    public class MainViewModel : ViewModelBase, IDisposable
    {
        public unsafe delegate void FrameUpdatedHandler(Models.SphereData* nativeBuffer, int count);
        public event FrameUpdatedHandler? OnFrameUpdated;

        private IntPtr _enginePtr;
        private Stopwatch _stopwatch;
        private double _lastElapsedTime;
        private bool _isDisposed;

        // UI Properties back-stores
        private int _sphereCount = 50;
        private float _sphereRadius = 0.5f;
        private float _timeScale = 1.0f;
        private string _performanceMetrics = "FPS: 0 | Frame Time: 0.00 ms";

        public MainViewModel()
        {
            // 1. Instantiating the unmanaged native engine instance
            _enginePtr = NativeMethods.CreateEngine();

            // 2. Pass down initial configuration setup
            UpdateEngineConfiguration();

            // 3. Kick off timing loops
            _stopwatch = Stopwatch.StartNew();
            _lastElapsedTime = 0;

            // 4. Hook into the WPF composition frame step (60 FPS driver)
            CompositionTarget.Rendering += OnRenderFrame;
        }

        #region UI Data Bound Properties

        public int SphereCount
        {
            get => _sphereCount;
            set
            {
                if (SetProperty(ref _sphereCount, value))
                    UpdateEngineConfiguration();
            }
        }

        public float SphereRadius
        {
            get => _sphereRadius;
            set
            {
                if (SetProperty(ref _sphereRadius, value))
                    UpdateEngineConfiguration();
            }
        }

        public float TimeScale
        {
            get => _timeScale;
            set
            {
                if (SetProperty(ref _timeScale, value))
                    UpdateEngineConfiguration();
            }
        }

        public string PerformanceMetrics
        {
            get => _performanceMetrics;
            set => SetProperty(ref _performanceMetrics, value);
        }

        #endregion

        private void UpdateEngineConfiguration()
        {
            if (_enginePtr == IntPtr.Zero)
                return;

            var config = new SimulationConfig
            {
                MaxSpheres = _sphereCount,
                SphereRadius = _sphereRadius,
                TimeScale = _timeScale,
                BoxSideLength = 20.0f // Fixed sandbox boundary metrics
            };

            NativeMethods.UpdateEngineConfig(_enginePtr, config);
        }

        /// <summary>
        /// This method fires every frame render cycle (roughly every 16.67ms for 60Hz screens)
        /// </summary>
        private unsafe void OnRenderFrame(object? sender, EventArgs e)
        {
            if (_enginePtr == IntPtr.Zero)
                return;

            // Calculate precise delta time elapsed since last frame
            double currentTime = _stopwatch.Elapsed.TotalSeconds;
            float deltaTime = (float)(currentTime - _lastElapsedTime);
            _lastElapsedTime = currentTime;

            // Guard against massive frame drops spikes (e.g. window moving dragging pauses)
            if (deltaTime > 0.1f)
                deltaTime = 0.1f;

            // Step 1: Advance the C++ physics computations
            NativeMethods.StepSimulation(_enginePtr, deltaTime);

            // Step 2: Core Zero-Copy Memory Pull
            // Grabs the raw pointer mapping directly to the C++ array matrix
            int activeSpheres = NativeMethods.GetSimulationState(_enginePtr, out SphereData* nativeBuffer);

            if (activeSpheres > 0 && nativeBuffer != null)
            {
                OnFrameUpdated?.Invoke(nativeBuffer, activeSpheres);

                // Unsafe Raw Pointer Loop Execution Showcase:
                // We parse structural components instantly across borders without .NET allocating memory loops
                for (int i = 0; i < activeSpheres; i++)
                {
                    SphereData* currentSphere = nativeBuffer + i;

                    // TODO: Pass data down directly to write-buffers of our rendering viewport.
                    // e.g. updating positions of a WriteableBitmap or Writeable3D Mesh instances.
                    float posX = currentSphere->Position.X;
                    float posY = currentSphere->Position.Y;
                    float posZ = currentSphere->Position.Z;
                }
            }

            // Step 3: Update performance panel counters metrics telemetry string
            float frameTimeMs = deltaTime * 1000.0f;
            float fps = deltaTime > 0 ? 1.0f / deltaTime : 0;
            PerformanceMetrics = $"FPS: {fps:F0} | Native Step Latency: {frameTimeMs:F2} ms | Tracked Nodes: {activeSpheres}";
        }

        #region Lifecycle Destructor Cleanups

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_isDisposed)
            {
                // Unhook render steps to prevent execution spikes on closed windows
                CompositionTarget.Rendering -= OnRenderFrame;

                if (_enginePtr != IntPtr.Zero)
                {
                    // CRITICAL: Clean up unmanaged heap allocation inside C++
                    NativeMethods.DestroyEngine(_enginePtr);
                    _enginePtr = IntPtr.Zero;
                }

                _isDisposed = true;
            }
        }

        ~MainViewModel()
        {
            Dispose(false);
        }

        #endregion
    }
}