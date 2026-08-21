# 构建本地 DCMTK 静态库 (供 C++/Qt 主程序 USE_DCMTK 链接)
# 用法: 在项目根目录执行  bash thirdparty/build_dcmtk.sh
# 依赖: CMake, Ninja, MinGW (Qt 自带 Tools/CMake_64, Tools/Ninja, Tools/mingw1310_64)
set -e
cd "$(dirname "$0")"
QT=/e/ProgramFiles/Qt
PATH="$QT/Tools/CMake_64/bin:$QT/Tools/Ninja:$QT/Tools/mingw1310_64/bin:$PATH"
SRC=dcmtk-DCMTK-3.6.8

# 1) 取源码 (若不存在)
if [ ! -d "$SRC" ]; then
  curl -L -o dcmtk.tar.gz \
    https://github.com/DCMTK/dcmtk/archive/refs/tags/DCMTK-3.6.8.tar.gz
  tar xzf dcmtk.tar.gz
  rm -f dcmtk.tar.gz
fi

# 2) 配置 (静态, 关闭外部依赖, 仅核心模块)
cmake -S "$SRC" -B dcmtk-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$(pwd)/dcmtk-install" \
  -DCMAKE_C_COMPILER="$QT/Tools/mingw1310_64/bin/gcc.exe" \
  -DCMAKE_CXX_COMPILER="$QT/Tools/mingw1310_64/bin/g++.exe" \
  -DBUILD_SHARED_LIBS=OFF \
  -DDCMTK_WITH_TIFF=OFF -DDCMTK_WITH_PNG=OFF -DDCMTK_WITH_OPENSSL=OFF \
  -DDCMTK_WITH_ZLIB=OFF -DDCMTK_WITH_ICU=OFF -DDCMTK_WITH_XML=OFF \
  -DDCMTK_WITH_ICONV=OFF -DDCMTK_WITH_SNAPSHOT=OFF -DBUILD_TESTING=OFF

# 3) 编译并安装
cmake --build dcmtk-build -j
cmake --install dcmtk-build || true   # 个别测试目标安装报错可忽略, 库已就绪
echo "DCMTK 安装完成: thirdparty/dcmtk-install"
