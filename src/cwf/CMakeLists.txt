
#ADD_CUSTOM_COMMAND(
#  OUTPUT  ${XNS_BUILD_DIR}/libfcgi.a
#  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libfcgi/make 
#  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/libfcgi/libfcgi.a ${XNS_BUILD_DIR}/libfcgi.a
#
#   #DEPENDS ${all_depends}
#  DEPENDS fcgiapp.c fcgi_stdio.c os_unix.c
#     COMMENT "-- Generating libfcgi.a file from ${CMAKE_CURRENT_SOURCE_DIR}/libfcgi/libfcgi.a"
#     VERBATIM
#)
set(FCGI_SRC_LISTS ${CMAKE_CURRENT_SOURCE_DIR}/libfcgi/fcgiapp.c ${CMAKE_CURRENT_SOURCE_DIR}/libfcgi/fcgi_stdio.c ${CMAKE_CURRENT_SOURCE_DIR}/libfcgi/os_unix.c)
message(LLL:${FCGI_SRC_LISTS})
add_library (fcgi ${FCGI_SRC_LISTS})

ADD_OCE_TARGET (cwf
  STATIC_LINKS cwf fcgi feedview ctemplate dbpool oldslice account user dbdesc arch crash base crash breakpad
  SHARED_LINKS pthread mysqlpp
  DEPENDS fcgi account storm oldslice base arch oldslice feedview crash dbpool dbdesc
  INCLUDE_PATH 
  EXCLUDE_FILES main.cc shop_client.cpp
  EXCLUDE_SHARED
  INCLUDE_EXE
  EXCLUDE_BOX
)

