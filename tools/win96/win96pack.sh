# Package minivector as a windows 96 executable
OUT_DIR="build/bin/Release"
EXEC_NAME="minivector"
OUT_NAME=$EXEC_NAME".wex"

echo "Packaging $EXEC_NAME as $OUT_DIR/$OUT_NAME"

cp $OUT_DIR/$EXEC_NAME $OUT_DIR/$EXEC_NAME.js
python3 vendor/win96sdk/tools/reformat_js.py $OUT_DIR/$EXEC_NAME.js
python3 ./tools/inline_worker.py $EXEC_NAME $OUT_DIR/$EXEC_NAME.js $OUT_DIR/$EXEC_NAME.worker.js
python3 vendor/win96sdk/tools/mkwex.py $OUT_DIR/$EXEC_NAME.wasm $OUT_DIR/$EXEC_NAME.js
mv prgm.wex $OUT_DIR/$OUT_NAME
