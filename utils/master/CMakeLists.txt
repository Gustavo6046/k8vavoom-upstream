if(ENABLE_MASTER)
  #include_directories(../common)

  add_executable(k8vavoom-master
    src/main.cpp
    src/netchan.h
    src/netchan.cpp
  )
  set_target_properties(k8vavoom-master PROPERTIES OUTPUT_NAME ../../k8vavoom-master)
  if(CYGWIN OR MINGW)
    set_target_properties(k8vavoom-master PROPERTIES LINK_FLAGS "-Wl,--subsystem,console")
  endif()
  #target_link_libraries(k8vavoom-master core ${NET_LIBRARIES})
  target_link_libraries(k8vavoom-master ${NET_LIBRARIES})
  #add_dependencies(k8vavoom-master core)

  if(CYGWIN OR MINGW)
    install(TARGETS k8vavoom-master DESTINATION ${BINDIR}/tools)
  else()
    install(TARGETS k8vavoom-master DESTINATION ${BINDIR})
  endif()
endif(ENABLE_MASTER)
