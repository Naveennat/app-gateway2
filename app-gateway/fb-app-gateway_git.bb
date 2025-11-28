SUMMARY = "FB App gateway plugins"
LICENSE = "CLOSED"

PR = "r1"
PV = "1.0+git${SRCPV}"

inherit cmake pkgconfig python3native

# Build from your working tree (three dirs up from this recipe dir) - uncomment:
inherit externalsrc
EXTERNALSRC = "${@os.path.abspath(os.path.join(d.getVar('THISDIR'), '../../../'))}"
EXTERNALSRC_BUILD = "${WORKDIR}/extern-build"
SRCTREECOVEREDTASKS = ""
python () {
    for t in ('do_configure','do_compile','do_install','do_package','do_populate_sysroot','do_populate_lic'):
        d.delVarFlag(t, 'file-checksums')
}
do_compile[file-checksums] = ""
do_install[file-checksums] = ""


# Or use remote repo
#S = "${WORKDIR}/git"
#SRC_URI = "git://github.com/rdk-e/app-gateway.git;protocol=${RDK_GIT_PROTOCOL};branch=main"
#SRCREV = "${AUTOREV}"

# Deps
DEPENDS += "wpeframework wpeframework-tools-native wpeframework-interfaces networkmanager-plugin"
DEPENDS += "websocketpp boost openssl curl"

RDEPENDS:${PN} += "wpeframework openssl curl"

# Flags
CXXFLAGS:append = " -DLOGMILESTONE"

EXTRA_OECMAKE += "\
  -DCMAKE_SYSROOT=${STAGING_DIR_HOST} \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_REFERENCE=INVALID \
"

# Packaging
FILES_SOLIBSDEV = ""
FILES:${PN} = " \
  ${libdir}/wpeframework/plugins/*.so \
  ${libdir}/wpeframework/proxystubs/*.so.* \
  ${datadir}/WPEFramework/* \
  ${sysconfdir}/entservices/*Config.json \
  ${sysconfdir}/WPEFramework/plugins/*.json \
  ${sysconfdir}/app-gateway/resolution*.json \
"

FILES:${PN}-dev += " \
  ${libdir}/wpeframework/proxystubs/*.so \
  ${libdir}/*.so \
  ${libdir}/pkgconfig/*.pc \
  ${libdir}/cmake/* \
"

FILES:${PN}-dbg += " \
  ${libdir}/wpeframework/proxystubs/.debug/ \
"
