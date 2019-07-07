@echo off


set IGNORED_WARNINGS=-D_CRT_SECURE_NO_WARNINGS -wd4100 -wd4201 -wd4189 -wd4505
set COMPILER_FLAGS_SLOW=-Od -Zi -nologo -fp:fast -fp:except- -WX -W4
set COMPILER_FLAGS_FAST=-O2     -nologo -fp:fast -fp:except- -WX -W4
set LINKER_FLAGS=user32.lib


if not exist build mkdir build

pushd build

if "%1"=="fast" (
	cl %COMPILER_FLAGS_FAST%  %IGNORED_WARNINGS% ..\main.cpp %LINKER_FLAGS%
) else (
	cl %COMPILER_FLAGS_SLOW%  %IGNORED_WARNINGS% ..\main.cpp %LINKER_FLAGS%
)


popd