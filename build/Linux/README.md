# Linux / NetBeans builds

NetBeans stores project metadata under `nbproject/` in the project directory,
so the **library** and **demo** must live in separate folders.

| Project | Path | Output |
|---------|------|--------|
| **wxPlotting** (static library) | `build/Linux/wxPlotting/` | `dist/…/libwxPlotting.a` |
| **wxPlottingDemo** (application) | `build/Linux/wxPlottingDemo/` | `dist/…/wxPlottingDemo` |
| **wxPlottingRealTimeDemo** (application) | `build/Linux/wxPlottingRealTimeDemo/` | `dist/…/wxPlottingRealTimeDemo` |

## Command line

```bash
# Library only
cd build/Linux/wxPlotting && make CONF=Debug

# Interactive property demo (also builds the library via subproject)
cd build/Linux/wxPlottingDemo && make CONF=Debug

# Real-time streaming demo
cd build/Linux/wxPlottingRealTimeDemo && make CONF=Debug

# Or from build/Linux:
make demo
make realtime
```

## NetBeans

1. **File → Open Project** → select `build/Linux/wxPlotting`
2. **File → Open Project** → select `build/Linux/wxPlottingDemo`
3. **File → Open Project** → select `build/Linux/wxPlottingRealTimeDemo`

Build the library first, or build a demo (each depends on the library).

Sources remain at the repository root; projects include `../../../`.
