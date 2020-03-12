FROM ubuntu:16.04

RUN apt-get update && apt-get install -q -y \
    build-essential sudo python \
    python-numpy \
    git 

COPY . /usr/src/blender/
WORKDIR /usr/src/blender/
RUN git config submodule.scons.url git://git.blender.org/scons.git 
RUN git config submodule.release/scripts/addons_contrib.url git://git.blender.org/blender-addons-contrib.git 
RUN git config submodule.release/scripts/addons.url git://git.blender.org/blender-addons.git 
RUN git config submodule.release/datafiles/locale.url git://git.blender.org/blender-translations.git 
RUN git config submodule.source/tools.url git://git.blender.org/blender-dev-tools.git 
RUN git submodule update --init --recursive 
RUN git submodule foreach 'git checkout tags/v2.80 || :'
# RUN git submodule foreach git pull --rebase origin master 

RUN ./build_files/build_environment/install_deps.sh --skip-numpy
RUN mkdir build && cd build &&  \
    cmake -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_C_FLAGS="-fPIC" -DWITH_HEADLESS=ON -DWITH_PYTHON_INSTALL=OFF \
    -DWITH_PLAYER=OFF -DWITH_PYTHON_MODULE=ON -DWITH_INSTALL_PORTABLE=OFF .. \
    && make -j 8 && make install

RUN echo "/opt/lib/python-3.7/lib" > /etc/ld.so.conf.d/python.conf 
RUN ldconfig
ENV PATH "$PATH:/opt/lib/python-3.7/bin"

RUN rm -rf /usr/src/blender/