cmake_minimum_required (VERSION 2.6)
project (vdmabuf_example)
# The version number.
set (VDMABUF_EXAMPLE_VERSION_MAJOR 1)
set (VDMABUF_EXAMPLE_VERSION_MINOR 0)

add_definitions(" -g")

add_executable(vdmabuf_unit_test_consumer test_vdmabuf_consumer.c)
add_executable(vdmabuf_unit_test_producer test_vdmabuf_producer.c)
