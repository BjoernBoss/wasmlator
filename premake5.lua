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
	files { "environment/**", "generate/**", "host/**", "repos/**", "rv64/**", "system/**", "util/**" }

	-- enforce msvc to use character-encoding utf-8
	filter { "toolset:msc*" }
		buildoptions{ "/execution-charset:utf-8" }
	filter ""

project "wasmlator"
	files { "entry/standalone.cpp", "entry/null-interface.cpp", "entry/null-specification.h" }

project "make-glue"
	files { "entry/make-glue.cpp", "entry/null-interface.cpp", "entry/null-specification.h" }

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
