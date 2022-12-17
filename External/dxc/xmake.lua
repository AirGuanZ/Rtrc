target("dxc")
    set_kind("phony")
    set_group("ThirdParty")
    add_includedirs("include", {public = true})
    add_links(path.join(os.scriptdir(), "lib/dxcompiler"), {public = true})
    on_build(function(target)
        if is_plat("windows") then
            os.cp(path.join(os.scriptdir(), "bin/dxcompiler.dll"), path.join(target:targetdir(), "dxcompiler.dll"))
            print("Copy dxcompiler.dll to "..target:targetdir())
        end
    end)
