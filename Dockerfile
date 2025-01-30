FROM debian:12

RUN apt-get update
RUN apt-get install --no-install-recommends -y \
	cmake tor build-essential libprotobuf-dev protobuf-compiler \
	libssl-dev qtbase5-dev
RUN apt-get install -y git

RUN mkdir -p /work
WORKDIR /work

RUN git clone https://github.com/wfr/ricochet-irc/ && \
	cd ricochet-irc && \
	git submodule update --init src/extern/tor && \
	git submodule update --init src/extern/fmt

RUN cd ricochet-irc && \
	mkdir build && \
	cmake -S ./src -B ./build -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel \
	  -DRICOCHET_REFRESH_INSTALL_DESKTOP=OFF -DUSE_SUBMODULE_FMT=ON -DENABLE_GUI=off && \
	cmake --build ./build -j$(nproc)

ENTRYPOINT /work/ricochet-irc/build/irc/ricochet-irc --host 0.0.0.0 --i-know-what-i-am-doing
