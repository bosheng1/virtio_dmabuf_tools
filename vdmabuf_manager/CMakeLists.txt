cmake_minimum_required (VERSION 2.6)
project (vdmabuf_manager)
# The version number.
set (VDMABUF_MANAGER_VERSION_MAJOR 1)
set (VDMABUF_MANAGER_VERSION_MINOR 0)

add_definitions(" -g")

set (VDMABUF_MANAGER_SRC
	vdmabuf_manager.c
	vdmabuf_common.c
	vdmabuf_ioctl.c)

set (VDMABUF_CLIENT_LIB_SRC
	vdmabuf_client.c
	vdmabuf_common.c)
add_library (vdmabuf_client SHARED ${VDMABUF_CLIENT_LIB_SRC})
add_executable(vdmabuf_manager ${VDMABUF_MANAGER_SRC})

add_executable(vdmabuf_client_test_producer ${VDMABUF_CLIENT_LIB_SRC} client_test_producer.c)
add_executable(vdmabuf_client_test_consumer ${VDMABUF_CLIENT_LIB_SRC} client_test_consumer.c)

install(TARGETS vdmabuf_manager vdmabuf_client_test_producer vdmabuf_client_test_consumer DESTINATION bin)
install(TARGETS vdmabuf_client DESTINATION lib64)
