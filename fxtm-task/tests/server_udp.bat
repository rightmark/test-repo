rem check wsserver.ini for parameters

@del /F task_state.* >nul 2>&1

@start cmd /c wsserver.exe -p 40002 -t UDP 2^> err_server_udp.log