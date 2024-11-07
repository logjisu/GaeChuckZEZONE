build_lib(
  LIBNAME zrp
  SOURCE_FILES
    model/zrp-routing-protocol.cc
    helper/zrp-helper.cc
  HEADER_FILES
    model/zrp-routing-protocol.h
    helper/zrp-helper.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libwifi}
                    ${libmobility}
                    ${libapplications}
                    ${libnetanim}
)
