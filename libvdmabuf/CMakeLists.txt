cmake_minimum_required (VERSION 2.6)
project (libvdmabuf)
# The version number.
set (VDMABUF_VERSION_MAJOR 1)
set (VDMABUF_VERSION_MINOR 0)

add_definitions(" -g")

set (VDMABUF_LIB_SRC
	vdmabuf_ioctl.c
	vdmabuf_common.c)
add_library (vdmabuf SHARED ${VDMABUF_LIB_SRC})

install(TARGETS vdmabuf DESTINATION lib64)
