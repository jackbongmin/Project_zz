#!/usr/bin/env python
# Project ZZ - GDExtension build script
# Usage:  scons                    (debug, for editor / testing)
#         scons target=template_release

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

library = env.SharedLibrary(
    "bin/project_zz{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
