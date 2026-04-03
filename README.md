# dotenv 🔧

Set env vars. Run commands. Load `.env` files. Every platform.

> `env API_KEY=abc123 node server.js` — works on Windows. Finally.

```bash
env API_KEY=abc123 node server.js          # set vars + run command
env .env.local DB=localhost node server.js  # extra .env file + vars + run
env node server.js                          # auto-load .env files + run
env                                         # print full environment
```

On macOS and Linux, the binary is named `dotenv` to avoid conflicting with the built-in `env`.

Also available as a C++23 static library for loading `.env` files in your own projects — like Python's `python-dotenv`.

## Table of Contents

- [Install](#install)
- [CLI Usage](#cli-usage)
  - [How arguments are parsed](#how-arguments-are-parsed)
  - [How .env files are found](#how-env-files-are-found)
- [Library — `dotenv`](#library--dotenv)
  - [Quick Start](#quick-start)
  - [Install (library)](#install-library)
  - [API Reference](#api-reference)
- [File Formats](#file-formats)
- [Building from Source](#building-from-source)
- [Testing](#testing)

## Install

Download from [Releases](https://github.com/BuildWithCollab/dotenv/releases) and put it on your PATH.

| Platform | File | Contains |
|----------|------|----------|
| Windows | `dotenv-windows-x64.zip` | `env.exe` + `dotenv.exe` |
| Linux | `dotenv-linux-x64.tar.gz` | `dotenv` |
| macOS | `dotenv-macos-arm64.tar.gz` | `dotenv` |

On Windows, both names are the same binary. `env` is shorter. `dotenv` avoids conflicts with Git Bash's built-in `env`.

## CLI Usage

```bash
env FOO=bar some-command [args...]            # Set vars + run command
env .env.local FOO=bar some-cmd [args...]     # Extra file + vars + run command
env .env.local FOO=bar -- some-cmd [args...]  # Same, but skip .env auto-loading
env                                           # Print all env vars (with .env files)
env --                                        # Print all env vars (without .env files)
env -h | --help                               # Show help
env --version                                 # Show version
```

### How arguments are parsed

Arguments are read left to right:

- Files named `.env` or starting with `.env.` (like `.env.local`, `.env.production`) → **env file**
- `KEY=value` → **inline variable**
- Anything else → **command starts here** (everything from this point is passed to it)

```bash
# Just run a command — .env files are loaded automatically
env node server.js

# Set one-off variables
env DEBUG=true PORT=3000 node server.js

# Load an extra env file on top of auto-discovered ones
env .env.local node server.js

# Skip auto-loading, only use what you list
env API_KEY=test123 -- curl https://api.example.com

# -- after the command starts is just a regular arg
env node server.js -- --extra-flag

# Just print the full environment
env
```

### How .env files are found

Starting from your current directory, `env` checks every parent directory all the way up to the filesystem root. If a `.env`, `.env.yaml`, `.env.yml`, or `.env.json` file exists in any of those directories, it gets loaded.

When the same variable appears in multiple files, the **closest file to your current directory wins**.

```
C:\projects\                .env          ← loaded first (lowest priority)
C:\projects\myapp\          .env.yaml     ← loaded second
C:\projects\myapp\backend\  .env.json     ← loaded last (highest priority, you are here)
```

A project-level `.env` can set defaults. A subdirectory `.env` can override them. Setting a variable to empty (`KEY=` or `"KEY": ""`) unsets it.

Use `--` to disable auto-loading entirely — only files you list explicitly will be used.

## Library — `dotenv`

### Quick Start

```cpp
#include <dotenv.hpp>

// Load all .env files from cwd to root, merged and expanded
auto vars = dotenv::load();

// Apply them to the current process environment
dotenv::apply(vars);
```

That's the two-liner equivalent of Python's `load_dotenv()`.

If you just want the data without touching the environment:

```cpp
auto vars = dotenv::load();
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
add_requires("dotenv")

target("myapp")
    add_packages("dotenv")
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
            "packages": ["collab-core", "dotenv"]
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
    "dependencies": ["dotenv"]
}
```

> The `name` and `version-string` fields just need to be valid — they can be anything. `name` must be all lowercase letters, numbers, and hyphens.

**`CMakeLists.txt`**:

```cmake
find_package(dotenv CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE collab::dotenv)
```

For more details, see the [Packages registry README](https://github.com/BuildWithCollab/Packages).

#### Then

```cpp
#include <dotenv.hpp>
```

### API Reference

#### `EnvironmentVariable`

```cpp
struct EnvironmentVariable {
    std::string key;
    std::string value;
};
```

#### Parsing (from string content, no filesystem)

```cpp
auto parse_dotenv(std::string_view content) -> std::vector<EnvironmentVariable>;
auto parse_yaml(std::string_view content) -> std::vector<EnvironmentVariable>;
auto parse_json(std::string_view content) -> std::vector<EnvironmentVariable>;
```

Parse `.env`, YAML, or JSON format content. These take raw string content, not file paths — useful for testing or when you already have the content in memory.

#### File Loading

```cpp
auto load_file(const std::filesystem::path& path) -> std::vector<EnvironmentVariable>;
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
    -> std::vector<EnvironmentVariable>;
```

Walk to root, load all env files, merge, and expand `${VAR}` references. Closest file wins. This is the equivalent of Python's `dotenv_values()`.

#### Operations

```cpp
auto merge(std::vector<EnvironmentVariable> vars) -> std::vector<EnvironmentVariable>;
auto expand(std::vector<EnvironmentVariable>& vars) -> void;
auto apply(const std::vector<EnvironmentVariable>& vars) -> void;
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

## Building from Source

Requires C++23. Uses [xmake](https://xmake.io).

```bash
xmake -y
```

## Testing

Tests use [Catch2](https://github.com/catchorg/Catch2).

```bash
xmake run dotenv-tests                         # run all tests
xmake run dotenv-tests "[parse_dotenv]"        # just dotenv parsing
xmake run dotenv-tests "[parse_yaml]"          # just yaml parsing
xmake run dotenv-tests "[parse_json]"          # just json parsing
xmake run dotenv-tests "[merge]"               # just merge logic
xmake run dotenv-tests "[expand]"              # just variable expansion
xmake run dotenv-tests "[load]"                # just file loading + discovery
xmake run dotenv-tests "[apply]"               # just env application
xmake run dotenv-tests "[cli]"                 # just CLI integration tests
```
