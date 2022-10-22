package("mimalloc")

    set_sourcedir(path.join(os.scriptdir(), "./"))
    add_configs("secure", {description = "Use a secured version of mimalloc", default = false, type = "boolean"})
    add_configs("rltgenrandom", {description = "Use a RtlGenRandom instead of BCrypt", default = false, type = "boolean"})

    add_deps("cmake")

    if is_plat("windows") then
        add_syslinks("advapi32", "bcrypt")
    elseif is_plat("linux") then
        add_syslinks("pthread")
    elseif is_plat("android") then
        add_syslinks("atomic")
    end

    on_install("macosx", "windows", "linux", "android", function (package)
        local configs = {"-DMI_OVERRIDE=OFF"}
        table.insert(configs, "-DMI_BUILD_STATIC=" .. (package:config("shared") and "OFF" or "ON"))
        table.insert(configs, "-DMI_BUILD_SHARED=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DMI_SECURE=" .. (package:config("secure") and "ON" or "OFF"))
        table.insert(configs, "-DMI_BUILD_TESTS=OFF")
        table.insert(configs, "-DMI_BUILD_OBJECT=OFF")
        --table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (is_mode("debug") and "Debug" or "Release"))
        if is_mode("debug") then
            table.insert(configs, "-D_DEBUG=")
        end
        local cxflags
        if package:config("rltgenrandom") then
            if xmake:version():ge("2.5.1") then
                cxflags = "-DMI_USE_RTLGENRANDOM"
            else
                -- it will be deprecated after xmake/v2.5.1
                package:configs().cxflags = "-DMI_USE_RTLGENRANDOM"
            end
        end
        import("package.tools.cmake").build(package, configs, {buildir = "build", cxflags = cxflags})

        if package:is_plat("windows") then
            os.trycp("build/**.dll", package:installdir("bin"))
            os.trycp("build/**.lib", package:installdir("lib"))
        else
            os.trycp("build/*.so", package:installdir("lib"))
            os.trycp("build/*.a", package:installdir("lib"))
        end
        os.cp("include", package:installdir())
    end)

    on_test(function (package)
        assert(package:has_cfuncs("mi_malloc", {includes = "mimalloc.h"}))
    end)
