package("mytbb")
	add_urls("https://github.com/oneapi-src/oneTBB/releases/download/v2021.7.0/oneapi-tbb-2021.7.0-win.zip")
	add_versions("2021.7.0", "cebeee095f3513815bf72437dc32d43ce964367323de9d7a651294ae51790d53")
	on_install("windows", function (package)
		local libdir = "lib/"
		local bindir = "redist/"
		print(os.curdir())
		print(package:installdir())
		os.cp("include", package:installdir())
		local prefix = ""
		if package:is_arch("x64", "x86_64") then
			prefix = "intel64/vc14"
		else
			prefix = "ia32/vc14"
		end
        if package:config("debug") then
            os.cp(libdir .. prefix .. "/*_debug.*", package:installdir("lib"))
            os.cp(bindir .. prefix .. "/*_debug.*", package:installdir("bin"))
        else
            os.cp(libdir .. prefix .. "/**|*_debug.*", package:installdir("lib"))
            os.cp(bindir .. prefix .. "/**|*_debug.*", package:installdir("bin"))
        end
	end)
package_end()
