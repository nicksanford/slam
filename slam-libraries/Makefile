BUILD_CHANNEL?=local

bufinstall:
	sudo apt-get install -y protobuf-compiler-grpc libgrpc-dev libgrpc++-dev || brew install grpc openssl --quiet

bufsetup:
	GOBIN=`pwd`/grpc/bin go install github.com/bufbuild/buf/cmd/buf@v1.8.0
	ln -sf `which grpc_cpp_plugin` grpc/bin/protoc-gen-grpc-cpp
	pkg-config openssl || export PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:`find \`which brew > /dev/null && brew --prefix\` -name openssl.pc | head -n1 | xargs dirname`

buf: bufsetup
	PATH="${PATH}:`pwd`/grpc/bin" buf generate --template ./buf.gen.yaml https://github.com/nicksanford/api.git
	PATH="${PATH}:`pwd`/grpc/bin" buf generate --template ./buf.grpc.gen.yaml https://github.com/nicksanford/api.git
	PATH="${PATH}:`pwd`/grpc/bin" buf generate --template ./buf.gen.yaml buf.build/googleapis/googleapis

clean-all: cleanorb cleancarto
	rm -rf grpc

cleanorb:
	rm -rf viam-orb-slam3/build
	rm -rf viam-orb-slam3/ORB_SLAM3/build
	rm -rf viam-orb-slam3/ORB_SLAM3/Thirdparty/DBoW2/build
	rm -rf viam-orb-slam3/ORB_SLAM3/Thirdparty/g2o/build
	rm -rf viam-orb-slam3/ORB_SLAM3/Thirdparty/Sophus/build
	rm -rf viam-orb-slam3/bin

cleancarto:
	rm -rf viam-cartographer/build
	rm -rf viam-cartographer/cartographer/build

format-setup:
	sudo apt-get install -y clang-format

format:
	find . -type f -not -path \
		-and ! -path '*viam-cartographer/cartographer*' \
		-and ! -path '*viam-cartographer/build*' \
		-and ! -path '*viam-orb-slam3/ORB_SLAM3*' \
		-and ! -path '*api*' \
		-and \( -iname '*.h' -o -iname '*.cpp' -o -iname '*.cc' \) \
		| xargs clang-format -i --style="{BasedOnStyle: Google, IndentWidth: 4}"

buildorb:
	cd viam-orb-slam3 && ./build_orbslam.sh

setuporb: 
ifeq ("Darwin", "$(shell uname -s)")
	cd viam-orb-slam3 && ./setup_orbslam_macos.sh
else
	cd viam-orb-slam3 && ./setup_orbslam.sh
endif

testorb:
	cd viam-orb-slam3 && ./test_orbslam.sh

orbslam-all: bufinstall buf setuporb buildorb testorb


appimageorb: buildorb
	cd packaging/appimages && BUILD_CHANNEL=${BUILD_CHANNEL} appimage-builder --recipe orb_grpc_server-`uname -m`.yml
	cd packaging/appimages && ./package_release_orb.sh
	mkdir -p packaging/appimages/deploy/
	mv packaging/appimages/*.AppImage* packaging/appimages/deploy/
	chmod 755 packaging/appimages/deploy/*.AppImage

appimagecarto: buildcarto
	cd packaging/appimages && BUILD_CHANNEL=${BUILD_CHANNEL} appimage-builder --recipe carto_grpc_server-`uname -m`.yml
	cd packaging/appimages && ./package_release_carto.sh
	mkdir -p packaging/appimages/deploy/
	mv packaging/appimages/*.AppImage* packaging/appimages/deploy/
	chmod 755 packaging/appimages/deploy/*.AppImage

setupcarto:
ifeq ("Darwin", "$(shell uname -s)")
	cd viam-cartographer/scripts && ./setup_cartographer_macos.sh
else
	cd viam-cartographer/scripts && ./setup_cartographer_linux.sh
endif
	

buildcarto:
ifneq ($(wildcard viam-cartographer/build/.),)
	cd viam-cartographer && ./scripts/build_viam_cartographer.sh 
else 
	cd viam-cartographer && ./scripts/build_cartographer.sh && ./scripts/build_viam_cartographer.sh
endif

install-lua-carto:
	sudo mkdir -p /usr/local/share/cartographer/lua_files/
	sudo cp viam-cartographer/lua_files/* /usr/local/share/cartographer/lua_files/
	sudo cp viam-cartographer/cartographer/configuration_files/* /usr/local/share/cartographer/lua_files/

testcarto:
	cd viam-cartographer && ./scripts/test_cartographer.sh

carto-all: bufinstall buf setupcarto buildcarto install-lua-carto testcarto

include *.make
