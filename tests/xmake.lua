target("collab-env-tests")
    set_kind("binary")
    add_files("*.cpp")
    add_deps("collab-env")
    add_packages("catch2")
    add_includedirs("../src")  -- access internal headers (merge.hpp, expand.hpp)
    add_collab_packages("collab-process")  -- CLI tests spawn dotenv binary
