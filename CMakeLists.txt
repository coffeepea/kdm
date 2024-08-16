cmake_minimum_required(VERSION 3.18)
include("cmake/rut.cmake")

project(KDM)
add_executable(KDM
  "src/main.c"
)
set_target_properties(KDM PROPERTIES C_STANDARD 17)
target_include_directories(KDM PUBLIC "include/")

RUTTargetPedantic(KDM)
RUTTargetNoExcept(KDM)

RUTGenBin2HeaderC("winio" "drv/WinIo.sys")
