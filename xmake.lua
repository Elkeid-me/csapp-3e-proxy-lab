add_rules("plugin.compile_commands.autoupdate")
set_languages("cxx20")
set_optimize("fastest")
set_warnings("all", "pedantic")
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
