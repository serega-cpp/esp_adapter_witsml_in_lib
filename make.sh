#! /bin/bash

if [ "$ESP_HOME" = "" ]
then
  echo "Environment variable ESP_HOME not found"
  exit
fi

make -C witsmler
make -C esp_adapter_witsml_in_lib

mkdir -p release
mv witsmler.a ./release
mv libesp_adapter_witsml_in_lib.so ./release

echo "Build completed successfully. Executables are in ./release"
