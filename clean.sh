#! /bin/bash

MMAKE_ROOT=$(pwd)
 
cd $MMAKE_ROOT/witsmler
make clean

cd $MMAKE_ROOT/esp_adapter_witsml_in_lib
make clean
