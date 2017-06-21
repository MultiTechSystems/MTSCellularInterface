@ECHO OFF

git describe > version.txt
set /p VERSION=<version.txt

echo #ifndef __MTS_CELLULAR_INTERFACE_VERSION_H__ > mts_cellular_interface_version.h
echo #define __MTS_CELLULAR_INTERFACE_VERSION_H__ >> mts_cellular_interface_version.h
echo( >> mts_cellular_interface_version.h
echo #define MTS_CELLULAR_INTERFACE_VERSION "%VERSION%" >> mts_cellular_interface_version.h
echo( >> mts_cellular_interface_version.h
echo #endif >> mts_cellular_interface_version.h

