platform_linker_flags="-lX11 -ldl -lm -lGL -Wl,--no-as-needed"
platform_source="code/linux_tds.cpp"
platform_target="linux_tds"
shared_compiler_flags="-m64 -fPIC -g"
library_source="code/tds.cpp"
library_target="libtds.so"

cc $library_source $shared_compiler_flags -shared -o tmp$library_target
mv tmp$library_target $library_target

cc $platform_source $shared_compiler_flags $platform_linker_flags -o $platform_target
