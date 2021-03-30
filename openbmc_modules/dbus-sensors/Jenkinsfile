#!groovy


stage 'Debug Build'
    sh '''
    rm -rf build_debug
    mkdir build_debug
    cd build_debug
    cmake .. -DCMAKE_BUILD_TYPE="Debug" -DHUNTER_ENABLED=1 -DEXTERNAL_PROJECT=1
    cmake --build .'''

stage 'Debug Test'
    sh '''cd build_debug
    dbus-launch ctest -V --output-on-failure'''

stage 'Release Build'
    sh '''
    rm -rf build_release
    mkdir build_release
    cd build_release
    cmake .. -DCMAKE_BUILD_TYPE="Release" -DHUNTER_ENABLED=1 -DEXTERNAL_PROJECT=1
    cmake --build .'''

stage 'Release Test'
    sh '''cd build_release
    dbus-launch ctest -V --output-on-failure'''
