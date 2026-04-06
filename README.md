<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2025 yanujz
-->

# libat

[![CI](https://github.com/Yanujz/libat/actions/workflows/ci.yml/badge.svg)](https://github.com/Yanujz/libat/actions/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)

A **header-only, zero-copy AT command parser** written in modern C++23.

Compliant with [ITU-T V.250](https://www.itu.int/rec/T-REC-V.250) and
[3GPP TS 27.007](https://www.3gpp.org/ftp/Specs/archive/27_series/27.007/).
Designed for embedded systems, modem drivers, and cellular IoT stacks where
heap allocations and runtime overhead are unacceptable.

---

## Features

| | |
|---|---|
| **Standard** | C++23 — `std::expected`, `std::generator`, `std::from_chars` |
| **Deployment** | Header-only — one `#include <at/parser.hpp>` |
| **Allocations** | Zero heap allocations during parsing |
| **Commands** | Basic (`ATH`, `ATA`, `ATZ` …), Extended (`AT+`, `AT#`, `AT%`, `AT&`), S-parameters (`ATS0=3`) |
| **Streaming** | `at::stream_parser` for continuous UART / serial input |
| **Dispatch** | `at::command_registry` with transparent (zero-copy) lookup |
| **Versioning** | Git-aware semver via generated `<at/version.hpp>` |

---

## Requirements

| Toolchain | Minimum version | Notes |
|---|---|---|
| GCC | 14 | libstdc++-14 ships `<generator>` |
| Clang | 18 | Requires GCC-14 headers in the include path |
| MSVC | 19.38 (VS 2022 17.8) | `/std:c++latest` |
| CMake | 3.20 | |

---

## Integration

### CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    libat
    GIT_REPOSITORY https://github.com/Yanujz/libat.git
    GIT_TAG        v1.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(libat)

target_link_libraries(your_target PRIVATE at::libat)
```

### Manual (copy header)

```bash
cp include/at/parser.hpp <your-project>/include/at/parser.hpp
```

---

## Usage

### Single-command parsing

```cpp
#include <at/parser.hpp>

at::parser p{"AT+CGDCONT=1,\"IP\",\"internet\""};
auto result = p.parse_command();

if (result) {
    // result->name  == "CGDCONT"
    // result->type  == at::command_type::set
    auto apn = result->params.get_as<std::string_view>(2);
    // apn == "internet"
}
```

### Streaming UART input

```cpp
#include <at/parser.hpp>

at::stream_parser sp;

// Feed bytes as they arrive — '\r' (S3 default) terminates each command.
auto cmds = sp.feed("AT+CMGF=1\rAT+CSQ\r");
if (cmds) {
    for (const auto& cmd : *cmds) {
        // cmd.name, cmd.type, cmd.params …
    }
}
```

### Command dispatch

```cpp
at::command_registry reg;

reg.register_handler("CMGF", [](const at::command& cmd) {
    if (auto mode = cmd.params.get_as<int64_t>(0))
        set_sms_mode(*mode);
});

reg.dispatch(parsed_command);  // zero-copy name lookup
```

---

## Building

```bash
# Configure (GCC 15 recommended; requires GCC ≥ 14 for <generator>)
cmake -B build \
  -DCMAKE_CXX_COMPILER=g++-15 \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/basic_modem
```

### Developer CMake targets

```bash
cmake --build build --target libat_check    # verify SPDX license headers
cmake --build build --target libat_license  # add/update license headers
```

---

## Project layout

```
libat/
├── CMakeLists.txt
├── LICENSE                          # GPL-3.0-or-later
├── include/
│   └── at/
│       ├── parser.hpp               # ← include this (the whole library)
│       └── version.hpp.in           # CMake version template
├── examples/
│   └── basic_modem.cpp
└── tests/
    └── test_parser.cpp              # Google Test suite (36 tests)
```

---

## License

Copyright (C) 2025 yanujz

This program is free software: you can redistribute it and/or modify it under
the terms of the **GNU General Public License** as published by the Free
Software Foundation, either **version 3** of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but **WITHOUT
ANY WARRANTY**; without even the implied warranty of **MERCHANTABILITY** or
**FITNESS FOR A PARTICULAR PURPOSE**. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

```
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2025 yanujz
```
