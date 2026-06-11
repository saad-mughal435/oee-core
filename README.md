# oee-core

A small, dependency-free C++17 library and CLI for Overall Equipment
Effectiveness (OEE) and downtime analytics.

[![CI](https://github.com/saad-mughal435/oee-core/actions/workflows/ci.yml/badge.svg)](https://github.com/saad-mughal435/oee-core/actions/workflows/ci.yml)

## What it computes

OEE = **Availability × Performance × Quality**

| Factor | Formula |
| ------ | ------- |
| Availability | run time ÷ planned time  (run time = planned − downtime) |
| Performance  | (ideal cycle × total units) ÷ run time |
| Quality      | good units ÷ total units |

`oee::compute` guards against divide-by-zero and clamps each factor to `[0, 1]`
(performance is capped at 100 % to absorb logged over-speed). `oee::grade` bands
the result (World class ≥ 85 %, Typical ≥ 60 %, …), and `oee::analyze_downtime`
builds a **Pareto of stops** with MTTR / MTBF.

## Build & test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requires CMake ≥ 3.16 and a C++17 compiler. Tests use
[Catch2](https://github.com/catchorg/Catch2) (fetched automatically by CMake).

## CLI

```bash
./build/oee --planned 480 --downtime 47 --total 19271 --rejects 423 \
            --cycle 1.337 --downtime-csv sample/downtime.csv
```

```
OEE report
----------
Availability :   90.2 %
Performance  :   99.2 %
Quality      :   97.8 %
OEE          :   87.5 %  (World class)
Run time     :  433.0 min
Good units   : 18848 of 19271

Downtime (Pareto)
-----------------
Stops        : 6 events, 47.0 min
MTTR         :    7.8 min
MTBF         :   72.2 min
  Changeover           30.0 min (63.8 %)
  ...
```

## Library API

```cpp
#include <oee/oee.hpp>

oee::ProductionInput in;
in.planned_time_min = 480.0;
in.downtime_min     = 47.0;
in.total_count      = 19271;
in.reject_count     = 423;
in.ideal_cycle_sec  = 1.337;

oee::OeeResult r = oee::compute(in);   // r.oee ≈ 0.875, oee::grade(r.oee) == "World class"
```

## License

MIT © Muhammad Saad
