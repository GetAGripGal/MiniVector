# Package minivector as a windows 96 executable
OUT_DIR="build/bin/Release"
EXEC_NAME="minivector"
OUT_NAME=$EXEC_NAME".wex"

echo "Building $EXEC_NAME"
premake5 gmake --win96
cd build
make config=release CC=emcc
cd ..

echo "Packaging $EXEC_NAME as $OUT_DIR/$OUT_NAME"

mv $OUT_DIR/$EXEC_NAME $OUT_DIR/$EXEC_NAME.js
python3 vendor/win96sdk/tools/reformat_js.py $OUT_DIR/$EXEC_NAME.js
python3 vendor/win96sdk/tools/mkwex.py $OUT_DIR/$EXEC_NAME.wasm $OUT_DIR/$EXEC_NAME.js
mv prgm.wex $OUT_DIR/$OUT_NAME

