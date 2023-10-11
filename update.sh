#!/bin/bash
VKDTDIR=../vkdt
cd ${VKDTDIR}
git fetch
VER=$(make release | cut -f4 -d' ')
cd -
git rm -rf bin/ doc/ ext/ src/
tar xJf ${VKDTDIR}/vkdt-${VER}.tar.xz --strip-components=1
git add $(tar tJf ${VKDTDIR}/vkdt-${VER}.tar.xz | cut -f2- -d/)
git commit -a -m "update to ${VER}"
git tag ${VER}
