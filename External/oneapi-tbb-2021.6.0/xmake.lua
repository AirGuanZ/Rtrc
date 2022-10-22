target("tbb")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_linkdirs("lib/intel64/vc14", {public = true})
    if is_mode("debug") then
        add_links("tbb_debug", {public = true})
        add_links("tbb12_debug", {public = true})
        add_links("tbbmalloc_debug", {public = true})
        add_links("tbbmalloc_proxy_debug", {public = true})
    else
        add_links("tbb", {public = true})
        add_links("tbb12", {public = true})
        add_links("tbbmalloc", {public = true})
        add_links("tbbmalloc_proxy", {public = true})
    end
    after_build(function(target)
        os.cp(path.join(os.scriptdir(), "redist/intel64/vc14/*.dll"), target:targetdir())
        print(target:targetdir())
    end)
