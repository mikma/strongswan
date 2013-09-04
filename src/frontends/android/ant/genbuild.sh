#!/bin/sh
#
# Generate build.xml and substitute ${sdk.dir}/tools/ant/build.xml
# with ant/build-<version>.xml which supports
# the aapt property "manifestPackage".
#
# Add the following to project.properties
# project.manifest.package=<my manifest package>
#

PROJECT_PATH=$1
PACKAGE_NAME=$2
TARGET=$3

TOOLS_REVISION=`grep Pkg.Revision= ${ANDROID_SDK_ROOT}/tools/source.properties | sed -e 's/.*=//'`
BUILD_XML=ant/build-${TOOLS_REVISION}.xml
ANDROID=android

test -f ${BUILD_XML} || (echo "Unsupported tools revision: ${TOOLS_REVISION}" ;exit 1)
${ANDROID} update project --path ${PROJECT_PATH} \
    --target ${TARGET} --name ${PACKAGE_NAME}
sed -i -e "s|import file=\".*/build.xml\"|import file=\"${BUILD_XML}\"|" build.xml
