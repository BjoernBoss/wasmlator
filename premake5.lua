newaction {
	trigger     = "clean",
	description = "Remove all build artifacts",
	execute     = function ()
		os.rmdir("./build")
		os.rmdir("./server/wat")
		os.rmdir("./server/wasm")
		print("All artifacts have been removed.")
	end
}

workspace "wasmlator"
	language "C++"
	cppdialect "C++20"
	configurations { "debug", "release", "distribute" }
	location "build"
	systemversion "latest"
	startproject "wasmlator"
	architecture "x86_64"
	staticruntime "On"
	kind "ConsoleApp"

	includedirs { "repos" }

	-- enforce msvc to use character-encoding utf-8
	filter { "toolset:msc*" }
		buildoptions{ "/execution-charset:utf-8" }
	filter ""

project "wasmlator"
	files { "env/**", "interface/**", "gen/**", "repos/**", "entry/standalone.cpp", "entry/null-interface.cpp" }

project "make-glue"
	files { "env/**", "interface/**", "gen/**", "repos/**", "entry/make-core.cpp", "entry/null-interface.cpp" }

project "make-core"
	files { "env/**", "interface/**", "gen/**", "repos/**", "entry/make-core.cpp", "entry/null-interface.cpp" }

project "make-block"
	files { "env/**", "interface/**", "gen/**", "repos/**", "entry/make-block.cpp", "entry/null-interface.cpp" }

filter "configurations:debug"
	symbols "On"
	runtime "Debug"

filter "configurations:release"
	optimize "Full"
	symbols "On"
	runtime "Release"

filter "configurations:distribute"
	optimize "Full"
	symbols "Off"
	runtime "Release"
