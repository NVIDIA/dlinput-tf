include_directories(${PROJECT_SOURCE_DIR})

execute_process(COMMAND python3 -c "import tensorflow as tf; print(' '.join(tf.sysconfig.get_compile_flags()), end='')" OUTPUT_VARIABLE TF_CFLAGS)
execute_process(COMMAND python3 -c "import tensorflow as tf; print(' '.join(tf.sysconfig.get_link_flags()), end='')" OUTPUT_VARIABLE TF_LFLAGS)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC ${TF_CFLAGS} ${TF_LFLAGS} -O2 -DNDEBUG")

SET(ZMQ_OP_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/zmq_ops)

add_library(util util.cc)
add_library(zmq_ops SHARED zmq_ops.cc zmq_op.cc)

set_target_properties(zmq_ops PROPERTIES PREFIX "nv")
# For dev, If want to custom .so output path, uncomment and modify following 2 lines.
#set_target_properties(nvzmq_ops PROPERTIES
#         LIBRARY_OUTPUT_DIRECTORY ${ZMQ_OP_OUTPUT_DIR})

target_link_libraries(zmq_ops zmq tensorflow_framework util)
