add_rules("mode.release")

-- Default to release simply because failed ASSERT causes annoying popups in debug mode :P
set_defaultmode("release")
set_languages("c++23")

includes("xmake/collab.lua")

-- BuildWithCollab package registry
add_repositories("BuildWithCollab https://github.com/BuildWithCollab/Packages.git")

-- Collab dependencies (local or registry, depending on env vars)
add_collab_requires("collab-core")
add_collab_requires("collab-process")

option("build_tests")
    set_default(true)
    set_showmenu(true)
    set_description("Build test targets")
option_end()

option("build_executable")
    set_default(true)
    set_showmenu(true)
    set_description("Build dotenv executable")
option_end()

if get_config("build_tests") then
    add_requires("catch2 3.x")
end

add_requires("nlohmann_json", "yaml-cpp")

target("collab-env")
    set_kind("static")
    add_headerfiles("include/(**.hpp)")
    add_includedirs("include", { public = true })
    add_files("src/*.cpp")
    add_collab_packages("collab-core")
    add_packages("nlohmann_json", "yaml-cpp", { public = true })

if get_config("build_executable") then
    target("env")
        set_kind("binary")
        add_files("cli/main.cpp")
        add_deps("collab-env")
        add_collab_packages("collab-process")
        set_rundir("$(projectdir)")
        after_build(function (target)
            local home = os.getenv("HOME") or os.getenv("USERPROFILE")
            if home then
                local bin_dir = path.join(home, "bin")
                os.mkdir(bin_dir)
                os.cp(target:targetfile(), path.join(bin_dir, path.filename(target:targetfile())))
                os.cp(target:targetfile(), path.join(bin_dir, "dotenv" .. (is_plat("windows") and ".exe" or "")))
                print("📦 Copied env + dotenv to " .. bin_dir)
            end
        end)
end

if get_config("build_tests") then
    includes("tests/xmake.lua")
end
