using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using Interface.ViewModels;

namespace Interface
{
    public partial class MainWindow : Window
    {
        private GeometryModel3D? _sphereTemplateModel;
        private List<TranslateTransform3D> _sphereTransforms = new();
        private int _currentRenderedCount = 0;

        public MainWindow()
        {
            InitializeComponent();
            BuildSphereTemplate();
            BuildWireframeCube();
            DataContextChanged += MainWindow_DataContextChanged;
        }

        private unsafe void MainWindow_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (e.NewValue is MainViewModel vm)
            {
                vm.OnFrameUpdated += UpdateVisualPositions;
            }
        }


        private void BuildSphereTemplate()
        {
            MeshGeometry3D mesh = new MeshGeometry3D();
            int slices = 16;
            int stacks = 16;
            double radius = 1.0;

            // 1. Generate Vertices
            for (int stack = 0; stack <= stacks; stack++)
            {
                double phi = Math.PI * stack / stacks;
                double y = radius * Math.Cos(phi);
                double rSinPhi = radius * Math.Sin(phi);

                for (int slice = 0; slice <= slices; slice++)
                {
                    double theta = 2.0 * Math.PI * slice / slices;
                    double x = rSinPhi * Math.Cos(theta);
                    double z = rSinPhi * Math.Sin(theta);

                    mesh.Positions.Add(new Point3D(x, y, z));
                }
            }

            // 2. Generate Indices with Correct Counter-Clockwise Winding (Outward Facing)
            int verticesPerRow = slices + 1;
            for (int stack = 0; stack < stacks; stack++)
            {
                for (int slice = 0; slice < slices; slice++)
                {
                    int p0 = stack * verticesPerRow + slice;         // Top-Left
                    int p1 = p0 + 1;                                 // Top-Right
                    int p2 = (stack + 1) * verticesPerRow + slice;   // Bottom-Left
                    int p3 = p2 + 1;                                 // Bottom-Right

                    // Triangle 1: Top-Left -> Top-Right -> Bottom-Left
                    mesh.TriangleIndices.Add(p0);
                    mesh.TriangleIndices.Add(p1);
                    mesh.TriangleIndices.Add(p2);

                    // Triangle 2: Top-Right -> Bottom-Right -> Bottom-Left
                    mesh.TriangleIndices.Add(p1);
                    mesh.TriangleIndices.Add(p3);
                    mesh.TriangleIndices.Add(p2);
                }
            }

            // 3. Apply Materials
            MaterialGroup materials = new MaterialGroup();
            materials.Children.Add(new DiffuseMaterial(new SolidColorBrush(Color.FromRgb(0, 90, 156))));
            materials.Children.Add(new SpecularMaterial(Brushes.White, 80.0));

            _sphereTemplateModel = new GeometryModel3D(mesh, materials);

            // Always keep BackMaterial defined during debugging so you can see meshes from the inside out!
            _sphereTemplateModel.BackMaterial = new DiffuseMaterial(Brushes.DarkRed);

            _sphereTemplateModel.Freeze();
        }

        private void BuildWireframeCube()
        {
            MeshGeometry3D mesh = new MeshGeometry3D();

            // The C++ engine uses BoxSideLength = 20.0f, meaning bounds are -10 to +10
            double half = 10.0;
            double thickness = 0.08; // Set wire thickness (in world units)

            // --- 4 Edges Parallel to X-Axis ---
            AddBoxEdge(mesh, new Point3D(0, -half, -half), half * 2, thickness, thickness);
            AddBoxEdge(mesh, new Point3D(0, -half, half), half * 2, thickness, thickness);
            AddBoxEdge(mesh, new Point3D(0, half, -half), half * 2, thickness, thickness);
            AddBoxEdge(mesh, new Point3D(0, half, half), half * 2, thickness, thickness);

            // --- 4 Edges Parallel to Y-Axis ---
            AddBoxEdge(mesh, new Point3D(-half, 0, -half), thickness, half * 2, thickness);
            AddBoxEdge(mesh, new Point3D(-half, 0, half), thickness, half * 2, thickness);
            AddBoxEdge(mesh, new Point3D(half, 0, -half), thickness, half * 2, thickness);
            AddBoxEdge(mesh, new Point3D(half, 0, half), thickness, half * 2, thickness);

            // --- 4 Edges Parallel to Z-Axis ---
            AddBoxEdge(mesh, new Point3D(-half, -half, 0), thickness, thickness, half * 2);
            AddBoxEdge(mesh, new Point3D(-half, half, 0), thickness, thickness, half * 2);
            AddBoxEdge(mesh, new Point3D(half, -half, 0), thickness, thickness, half * 2);
            AddBoxEdge(mesh, new Point3D(half, half, 0), thickness, thickness, half * 2);

            // Create a high-visibility green material composition
            MaterialGroup materials = new MaterialGroup();
            materials.Children.Add(new DiffuseMaterial(new SolidColorBrush(Color.FromRgb(0, 200, 0))));

            // Emissive material forces self-illumination so the lines pop out in dark scenes
            materials.Children.Add(new EmissiveMaterial(new SolidColorBrush(Color.FromRgb(0, 90, 0))));

            GeometryModel3D boxModel = new GeometryModel3D(mesh, materials);
            boxModel.Freeze(); // Keep it optimized on the GPU layout thread

            ContainerBoxGroup.Children.Add(boxModel);
        }

        private void AddBoxEdge(MeshGeometry3D mesh, Point3D center, double sx, double sy, double sz)
        {
            int baseIndex = mesh.Positions.Count;

            double hx = sx / 2.0;
            double hy = sy / 2.0;
            double hz = sz / 2.0;

            // Generate 8 localized structural vertices for the thin line segment
            mesh.Positions.Add(new Point3D(center.X - hx, center.Y - hy, center.Z - hz)); // 0
            mesh.Positions.Add(new Point3D(center.X + hx, center.Y - hy, center.Z - hz)); // 1
            mesh.Positions.Add(new Point3D(center.X + hx, center.Y + hy, center.Z - hz)); // 2
            mesh.Positions.Add(new Point3D(center.X - hx, center.Y + hy, center.Z - hz)); // 3
            mesh.Positions.Add(new Point3D(center.X - hx, center.Y - hy, center.Z + hz)); // 4
            mesh.Positions.Add(new Point3D(center.X + hx, center.Y - hy, center.Z + hz)); // 5
            mesh.Positions.Add(new Point3D(center.X + hx, center.Y + hy, center.Z + hz)); // 6
            mesh.Positions.Add(new Point3D(center.X - hx, center.Y + hy, center.Z + hz)); // 7

            // 12 triangles (6 faces) mapped out with perfect outward-facing CCW winding layouts
            int[] indices = new int[] {
                0, 1, 2,   0, 2, 3, // Front (Z-)
                4, 6, 5,   4, 7, 6, // Back (Z+)
                4, 0, 3,   4, 3, 7, // Left (X-)
                1, 5, 6,   1, 6, 2, // Right (X+)
                3, 2, 6,   3, 6, 7, // Top (Y+)
                4, 5, 1,   4, 1, 0  // Bottom (Y-)
            };

            foreach (int idx in indices)
            {
                mesh.TriangleIndices.Add(baseIndex + idx);
            }
        }

        private unsafe void UpdateVisualPositions(Models.SphereData* nativeBuffer, int count)
        {
            // If sphere count slider updates, reconstruct the 3D visual tree
            if (count != _currentRenderedCount)
            {
                SpheresGroup.Children.Clear();
                _sphereTransforms.Clear();

                for (int i = 0; i < count; i++)
                {
                    Model3DGroup individualSphereInstance = new Model3DGroup();
                    // Clone the template for each instance. A GeometryModel3D cannot be
                    // shared as a child in multiple places in the 3D visual tree, so
                    // create a copy per-instance to ensure it is rendered.
                    individualSphereInstance.Children.Add((_sphereTemplateModel != null) ? _sphereTemplateModel.Clone() : null);

                    var transform = new TranslateTransform3D();
                    var scale = new ScaleTransform3D();

                    Transform3DGroup transformGroup = new Transform3DGroup();
                    transformGroup.Children.Add(scale);
                    transformGroup.Children.Add(transform);

                    individualSphereInstance.Transform = transformGroup;

                    // Injects our meshes directly into the XAML named container
                    SpheresGroup.Children.Add(individualSphereInstance);
                    _sphereTransforms.Add(transform);
                }
                _currentRenderedCount = count;
            }

            // Zero-Copy pointer parsing loop: updates position matrices on every GPU sync frame
            for (int i = 0; i < count; i++)
            {
                Models.SphereData* current = nativeBuffer + i;
                TranslateTransform3D transform = _sphereTransforms[i];

                // Direct Win32 memory map assignment to dependency properties
                transform.OffsetX = current->Position.X;
                transform.OffsetY = current->Position.Y;
                transform.OffsetZ = current->Position.Z;

                var group = (Transform3DGroup)SpheresGroup.Children[i].Transform;
                var scale = (ScaleTransform3D)group.Children[0];
                scale.ScaleX = current->Radius;
                scale.ScaleY = current->Radius;
                scale.ScaleZ = current->Radius;
            }
        }
    }
}