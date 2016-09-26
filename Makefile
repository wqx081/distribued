CXXFLAGS += -std=c++11
CXXFLAGS += -I./
CXXFLAGS += -std=c++11 -Wall -Werror -g -c -o

LIB_FILES :=-lglog -lgflags -levent  -lpthread -lssl -lcrypto -lz -lprotobuf -lpthread -ldl

TEST_LIB_FILES :=  -L/usr/local/lib -lgtest -lgtest_main -lpthread

PROTOC = protoc
GRPC_CPP_PLUGIN=grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PROTOS_PATH = ./protos

CPP_SOURCES := \
	./core/base/status.cc \
	./core/base/mem.cc \
	./core/base/threadpool.cc \
	\
	./core/strings/ordered_code.cc \
	./core/strings/string_piece.cc \
	./core/strings/stringprintf.cc \
	./core/strings/str_util.cc \
	./core/strings/base64.cc \
	./core/strings/numbers.cc \
	./core/strings/scanner.cc \
	./core/strings/strcat.cc \
	\
	./core/files/file_system.cc \
	./core/files/linux/linux_file_system.cc \
	./core/io/zero_copy_stream.cc \
	./core/io/zero_copy_stream_impl_lite.cc \
	./core/io/zero_copy_stream_impl.cc \
	./core/io/path.cc \
	\
	./core/system/load_library.cc \
	./core/system/env.cc \
	./core/system/linux/linux_env.cc \
	\
	./protobuf/mr_server.pb.cc \
	./protobuf/device_attributes.pb.cc \
	./framework/device_base.cc \
	./cr/device.cc \
	\
	./dr/server_interface.cc \
	./dr/rpc/grpc_server.cc \
	./dr/call_options.cc \

CPP_OBJECTS := $(CPP_SOURCES:.cc=.o)


TESTS := \
	./unittests/dr/mr_server_unittest \
	./unittests/core/threadpool_unittest \



all: $(CPP_OBJECTS) $(TESTS)
.cc.o:
	@echo "  [CXX]  $@"
	@$(CXX) $(CXXFLAGS) $@ $<

./unittests/dr/mr_server_unittest: \
	./unittests/dr/mr_server_unittest.o \
	./dr/server_interface.o \
	./dr/rpc/grpc_server.o 
	@echo "  [LINK] $@"
	@$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_FILES) -L/usr/local/lib -lgtest -lpthread
./unittests/dr/mr_server_unittest.o: \
	./unittests/dr/mr_server_unittest.cc
	@echo "  [CXX]  $@"
	@$(CXX) $(CXXFLAGS) $@ $<

./unittests/core/threadpool_unittest: \
	./unittests/core/threadpool_unittest.o \
	./core/base/threadpool.h \
	./core/base/threadpool.o \
	./core/base/thread/device_thread_pool.h \
	./core/base/thread/simple_thread_pool.h 
	@echo "  [LINK] $@"
	@$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_FILES) $(TEST_LIB_FILES)
./unittests/core/threadpool_unittest.o: \
	./unittests/core/threadpool_unittest.cc
	@echo "  [CXX]  $@"
	@$(CXX) $(CXXFLAGS) $@ $<


## /////////////////////////////

clean:
	@rm -fr $(TESTS)
	@echo "rm *_unittest"
	@rm -fr $(CPP_OBJECTS)
	@echo "rm *.o"
