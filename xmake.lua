add_rules("mode.debug", "mode.release") 
set_languages("c++20")

target("shepherd-core")
    set_kind("binary")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_syslinks("pthread") 