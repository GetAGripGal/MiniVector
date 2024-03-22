# Build for windows 96
OUT_DIR="build/bin/Release"
EXEC_NAME="minivector"
OUT_NAME=$EXEC_NAME".wex"

echo "Building $EXEC_NAME"
premake5 gmake --win96
cd build
make config=release CC=emcc
cd ..
