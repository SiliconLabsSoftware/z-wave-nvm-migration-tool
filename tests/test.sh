#!/bin/sh

readonly SCRIPT_PATH="$(dirname "$0")"
TEST_FOLDER="$(dirname "$SCRIPT_PATH")"
BINARY_FOLDER="$TEST_FOLDER/assets"
INPUT_BIN="$BINARY_FOLDER/nvm3_7_18_1.bin"
SCHEMA_FILE="../zwave_data_description_scheme.json"
MIGRATION_TOOL="../build/z-wave-nvm-migration-tool"
HARDWARE_PART="EFR32XG23"

for VERSION in 7.19.0 7.20.0 7.21.0 7.22.0 7.23.0 7.24.0; do
  VERSION_=$(echo "$VERSION" | sed 's/\./_/g')
  OUTPUT_BIN="/tmp/nvm3_${VERSION_}.bin"
  EXPECTED_BIN="$BINARY_FOLDER/expected_${VERSION_}.bin"

  echo "Migration from 7.18.1 to $VERSION..."
  $MIGRATION_TOOL -m "$INPUT_BIN" "$VERSION" $HARDWARE_PART $SCHEMA_FILE -o "$OUTPUT_BIN" > /dev/null 2>&1

  if [ $? -ne 0 ]; then
    echo "Migration process failed for $VERSION."
    rm -f "$OUTPUT_BIN"
    exit 1
  fi

  echo "Verifying output for $VERSION..."
  cmp -b "$OUTPUT_BIN" "$EXPECTED_BIN"
  if [ $? -eq 0 ]; then
    echo "$VERSION: PASSED"
  else
    echo "$VERSION: FAILED"
  fi

  rm -f "$OUTPUT_BIN"
done