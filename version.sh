#!/bin/bash

VERSION=`git describe`

echo -e "#ifndef __MTS_CELLULAR_INTERFACE_VERSION_H__\r\n#define __MTS_CELLULAR_INTERFACE_VERSION_H__\r\n\r\n#define MTS_CELLULAR_INTERFACE_VERSION \"$VERSION\"\r\n\r\n#endif" > mts_cellular_interface_version.h

