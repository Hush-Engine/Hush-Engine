# bc7enc — public-domain BC1-7 / DXT block texture compression library
# by Rich Geldreich (https://github.com/richgel999/bc7enc)
#
# Source files should be placed in this directory:
#   bc7enc.h / bc7enc.cpp   — BC7 encoder
#   bc7decomp.h / bc7decomp.cpp — BC7 decompressor
#   rgbcx.h / rgbcx.cpp     — BC1-5 / DXT compressor

add_library(bc7enc STATIC
    bc7enc.cpp
    bc7decomp.cpp
    rgbcx.cpp
)

target_include_directories(bc7enc PUBLIC .)
target_compile_definitions(bc7enc PRIVATE BC7ENC_USE_UNION=0)

# Suppress some warnings in third-party code
if(MSVC)
    target_compile_options(bc7enc PRIVATE /wd4244 /wd4305)
endif()
