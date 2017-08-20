add_executable(dfk-ftp-server
  dfk_ftp_server.c
  ftp_protocol.c)

target_link_libraries(dfk-ftp-server dfk)
