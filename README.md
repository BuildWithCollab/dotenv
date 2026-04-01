# collab-env 📄

A C++23 `.env` file library and CLI. Load environment variables from `.env`, `.env.yaml`, and `.env.json` files.

Two things in one repo:

| | What | For |
|---|---|---|
| **`collab-env`** | Static library | Any C++ project that wants to load `.env` files |
| **`dotenv`** | CLI binary | Run any command with `.env` variables injected |

## Table of Contents

- [CLI — `dotenv`](#cli--dotenv)
  - [Install (binary)](#install-binary)
  - [Usage](#usage)
  - [How .env files are found](#how-env-files-are-found)
- [Library — `collab-env`](#library--collab-env)
  - [Quick Start](#quick-start)
  - [Install (library)](#install-library)
  - [API Reference](#api-reference)
- [File Formats](#file-formats)
- [Building](#building)
- [Testing](#testing)

## CLI — `dotenv`

### Install (binary)

Download from [Releases](https://github.com/BuildWithCollab/dotenv/releases) and put it on your PATH.

| Platform | File | Contains |
|----------|------|----------|
| Windows | `dotenv-windows-x64.zip` | `env.exe` + `dotenv.exe` |
| Linux | `dotenv-linux-x64.tar.gz` | `dotenv` |
| macOS | `dotenv-macos-arm64.tar.gz` | `dotenv` |

On Windows, both `env.exe` and `dotenv.exe` are the same binary. Use whichever you prefer — `dotenv` avoids conflicts with Git Bash's built-in `env`.

### Usage

```bash
dotenv                                       # Print all env vars (with .env files)
dotenv --                                    # Print all env vars (without .env files)
dotenv FOO=bar some-command [args...]        # Run command with env var
dotenv .env.local FOO=bar some-cmd [args...] # Extra file + var, run command
dotenv .env.local FOO=bar -- some-cmd        # Same, but skip .env auto-loading
dotenv -h | --help                           # Show help
dotenv --version                             # Show version
```

**How arguments are parsed:**

Arguments are read left to right. Files named `.env` or starting with `.env.` (like `.env.local`) are collected as env files. `KEY=value` pairs are collected as inline vars. The first argument that is neither is the command — everything from that point on is passed to it.

By default, `.env` files are automatically loaded from every parent directory up to the root. Use `--` to disable auto-loading — only files you list explicitly will be used. If `--` appears after the command has started, it's just passed to the command as a regular argument.

```bash
# Just run a command — .env files are loaded automatically
dotenv node server.js

# Set a one-off variable
dotenv DEBUG=true node server.js

# Load an extra env file
dotenv .env.local node server.js

# Skip auto-loading, only use what you specify
dotenv API_KEY=test123 -- curl https://api.example.com

# Pass -- to the command (it's after the command, so it's just an arg)
dotenv node server.js -- --extra-flag

# Just print the full environment
dotenv
```

### How .env files are found

Starting from your current directory, `dotenv` looks in every parent directory all the way up to the filesystem root. If a `.env`, `.env.yaml`, `.env.yml`, or `.env.json` file exists in any of those directories, it gets loaded.

When the same variable appears in multiple files, the **closest file to your current directory wins**.

```
C:\projects\                .env          ← loaded first (lowest priority)
C:\projects\myapp\          .env.yaml     ← loaded second
C:\projects\myapp\backend\  .env.json     ← loaded last (highest priority, you are here)
```

This means a project-level `.env` can set defaults, and a subdirectory `.env` can override them. Setting a variable to empty (`KEY=` or `"KEY": ""`) unsets it.

## Library — `collab-env`

### Quick Start

```cpp
#include <collab/env.hpp>

// Load all .env files from cwd to root, merged and expanded
auto vars = collab::env::load();

// Apply them to the current process environment
collab::env::apply(vars);
```

That's the two-liner equivalent of Python's `load_dotenv()`.

If you just want the data without touching the environment:

```cpp
auto vars = collab::env::load();
for (auto& v : vars)
    std::cout << v.key << "=" << v.value << "\n";
```

### Install (library)

Packages are hosted on the [BuildWithCollab/Packages](https://github.com/BuildWithCollab/Packages) registry.

#### xmake

Add the package registry to your `xmake.lua` (one-time setup):

```lua
add_repositories("BuildWithCollab https://github.com/BuildWithCollab/Packages.git")
```

Then require and use the package:

```lua
add_requires("collab-env")

target("myapp")
    add_packages("collab-env")
```

#### CMake / vcpkg

Custom registries for vcpkg are a bit more involved, but still easy to set up. You need two configuration files in your project root.

**`vcpkg-configuration.json`** — tells vcpkg where to find packages. A `baseline` is a git commit hash that determines which package versions are available:

```json
{
    "default-registry": {
        "kind": "git",
        "repository": "https://github.com/microsoft/vcpkg.git",
        "baseline": "<latest-vcpkg-commit-hash>"
    },
    "registries": [
        {
            "kind": "git",
            "repository": "https://github.com/BuildWithCollab/Packages.git",
            "baseline": "<latest-packages-commit-hash>",
            "packages": ["collab-core", "collab-env"]
        }
    ]
}
```

> `collab-core` must be listed in `packages` — it's a transitive dependency.

To get the latest baselines:

```bash
# BuildWithCollab registry
git ls-remote https://github.com/BuildWithCollab/Packages.git HEAD

# Microsoft vcpkg registry
git ls-remote https://github.com/microsoft/vcpkg.git HEAD
```

> When the registry is updated with new versions, you'll need to update the baseline to see them.

**`vcpkg.json`** — your project manifest:

```json
{
    "name": "my-project",
    "version-string": "0.0.1",
    "dependencies": ["collab-env"]
}
```

> The `name` and `version-string` fields just need to be valid — they can be anything. `name` must be all lowercase letters, numbers, and hyphens.

**`CMakeLists.txt`**:

```cmake
find_package(collab-env CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE collab::collab-env)
```

For more details, see the [Packages registry README](https://github.com/BuildWithCollab/Packages).

#### Then

```cpp
#include <collab/env.hpp>
```

### API Reference

#### `EnvVar`

```cpp
struct EnvVar {
    std::string key;
    std::string value;
};
```

#### Parsing (from string content, no filesystem)

```cpp
auto parse_dotenv(std::string_view content) -> std::vector<EnvVar>;
auto parse_yaml(std::string_view content) -> std::vector<EnvVar>;
auto parse_json(std::string_view content) -> std::vector<EnvVar>;
```

Parse `.env`, YAML, or JSON format content. These take raw string content, not file paths — useful for testing or when you already have the content in memory.

#### File Loading

```cpp
auto load_file(const std::filesystem::path& path) -> std::vector<EnvVar>;
```

Load a single file, auto-detecting format by extension (`.yaml`/`.yml` → YAML, `.json` → JSON, everything else → dotenv).

#### Discovery

```cpp
auto find_env_files(const std::filesystem::path& from = std::filesystem::current_path())
    -> std::vector<std::filesystem::path>;
```

Walk from `from` to the filesystem root, collecting all `.env`, `.env.yaml`, `.env.yml`, and `.env.json` files found. Returns paths ordered root → closest.

#### Load (the main entry point)

```cpp
auto load(const std::filesystem::path& from = std::filesystem::current_path())
    -> std::vector<EnvVar>;
```

Walk to root, load all env files, merge, and expand `${VAR}` references. Closest file wins. This is the equivalent of Python's `dotenv_values()`.

#### Operations

```cpp
auto merge(std::vector<EnvVar> vars) -> std::vector<EnvVar>;
auto expand(std::vector<EnvVar>& vars) -> void;
auto apply(const std::vector<EnvVar>& vars) -> void;
```

| Function | What it does |
|----------|-------------|
| `merge` | Deduplicate by key (case-insensitive). Last write wins. First occurrence's position and key name are preserved. |
| `expand` | Resolve `${VAR}` references. Looks in the vars list first (case-insensitive), then falls back to the real environment. Iterates until stable (max 20 passes). |
| `apply` | Set each var in the current process environment. Empty value = unset. |

## File Formats

```
.env                         .env.yaml / .env.yml
┌────────────────────────┐   ┌────────────────────────┐
│ DB_HOST=localhost      │   │ DB_HOST: localhost     │
│ DB_PORT=5432           │   │ DB_PORT: 5432          │
│ SECRET="my secret"     │   │ SECRET: my secret      │
│ # comment              │   │                        │
│ UNSET_ME=              │   │ UNSET_ME:              │
└────────────────────────┘   └────────────────────────┘

.env.json                      Variable references
┌────────────────────────────┐ ┌────────────────────────────────┐
│ {                          │ │ HOST=localhost                 │
│   "DB_HOST": "localhost",  │ │ PORT=5432                      │
│   "DB_PORT": 5432,         │ │ URL=http://${HOST}:${PORT}     │
│   "SECRET": "my secret",   │ └────────────────────────────────┘
│   "UNSET_ME": ""           │
│ }                          │
└────────────────────────────┘
```

| Syntax | Meaning |
|--------|---------|
| `KEY=value` | Set a variable |
| `KEY=` | Unset a variable (empty value removes it) |
| `${VAR}` | Reference another variable (from .env files or real env) |
| `# comment` | Ignored (`.env` format only) |
| `export KEY=value` | Optional `export` prefix accepted (`.env` format only) |

## Building

Requires C++23. Uses [xmake](https://xmake.io).

```bash
xmake -y
```

## Testing

Tests use [Catch2](https://github.com/catchorg/Catch2).

```bash
xmake run collab-env-tests                         # run all tests
xmake run collab-env-tests "[parse_dotenv]"        # just dotenv parsing
xmake run collab-env-tests "[parse_yaml]"          # just yaml parsing
xmake run collab-env-tests "[parse_json]"          # just json parsing
xmake run collab-env-tests "[merge]"               # just merge logic
xmake run collab-env-tests "[expand]"              # just variable expansion
xmake run collab-env-tests "[load]"                # just file loading + discovery
xmake run collab-env-tests "[apply]"               # just env application
xmake run collab-env-tests "[cli]"                 # just CLI integration tests
```
