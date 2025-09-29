# JsonViewer
A modified version of [realworld666/JsonViewer](https://github.com/realworld666/JsonViewer), which is itself based off of epocalipse.com JsonViewer

The JSON View package is a set of two viewers available in the following flavors:
1) A standalone viewer - JsonView.exe
2) A visualizer for Viusal Studio - JsonVisualizer.dll

The viewer supports plugins to allow you to customize the way JSON objects are displayed. Sample plugins 
are provided within the source.

Improvements
============
- JSON parsing moved to background worker and throttled for performance
- Better status information
- HiDPI support
- Minor bugfixes and feature cleanup
- 64-bit build
- Updated Newtonsoft.Json and retarget for .NET 4.5

Installation
============

The archive contains the following directories:
\JsonView
\Visualizer

- To use the standalone viewer, run JsonView.exe from \JsonView
- To use the Fiddler2 plugin, copy the files from the \Fiddler directory to fiddler's \Inspectors 
  directory and add the following to the <runtime> section of the fiddler.exe.config:
- To use the Visual Studio Visualizer, copy the JsonVisualizer.dll to the Visual Studio Visualizers 
  directory (usually under \My Documents\Visual Studio 2005\Visualizers) and copy the following files
  to the IDE directory of Visual Studio (Where devenv.exe is located - <Visual Studio>\Common7\IDE):
  - JsonViewer.dll
  - JsonViewer.dll.config
  - Newtonsoft.Json.dll
