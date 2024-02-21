add_rules("plugin.compile_commands.autoupdate")
set_config("cxxflags", "-O3 -Wall -std=c++20 -pedantic-errors")
set_config("ldflags", "-lpthread")
target("proxy") do
    set_kind("binary")
    add_files("src/*.cpp")
    after_build
    (
        function (target)
            os.mv("$(buildir)/linux/x86_64/release/proxy", "$(projectdir)/proxy")
        end
    )
end
