using System.Windows;
using Interface.ViewModels;

namespace Interface
{
    public partial class App : Application
    {
        private MainViewModel? _mainViewModel;

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            // 1. Initialize our high-performance architecture viewmodel
            _mainViewModel = new MainViewModel();

            // 2. Instantiate the window view shell
            MainWindow mainWindow = new MainWindow();

            // 3. Inject the data context explicitly to achieve clean MVVM separation
            mainWindow.DataContext = _mainViewModel;

            // 4. Present the UI to the user
            mainWindow.Show();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            // CRITICAL: Ensure the native C++ engine is disposed of immediately on exit
            _mainViewModel?.Dispose();
            base.OnExit(e);
        }
    }
}