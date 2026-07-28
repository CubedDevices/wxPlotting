# Linux / NetBeans builds

NetBeans stores project metadata under `nbproject/` in the project directory,
so the **library** and **demo** must live in separate folders.

| Project | Path | Output |
|---------|------|--------|
| **wxPlotting** (static library) | `build/Linux/wxPlotting/` | `dist/…/libwxPlotting.a` |
| **wxPlottingDemo** (application) | `build/Linux/wxPlottingDemo/` | `dist/…/wxPlottingDemo` |

## Command line

```bash
# Library only
cd build/Linux/wxPlotting && make CONF=Debug

# Demo (also builds the library via subproject)
cd build/Linux/wxPlottingDemo && make CONF=Debug
```

## NetBeans

1. **File → Open Project** → select `build/Linux/wxPlotting`
2. **File → Open Project** → select `build/Linux/wxPlottingDemo`

Build the library first, or build the demo (it depends on the library).

Sources remain at the repository root; both projects include `../../../`.
