"""Late PlatformIO hook: the framework builder sets its linker flags after pre hooks.
The -flto compile flag must also be supplied to the final C++ linker driver.
"""
Import("env")
env.Append(LINKFLAGS=["-flto"])
print("[build] Post-framework linker flags:", env.get("LINKFLAGS"))
