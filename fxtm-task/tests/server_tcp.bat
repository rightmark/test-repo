rem check wsserver.ini for parameters

@del /F task_state.* >nul 2>&1

@start cmd /c wsserver.exe -t TCP 2^> err_server_tcp.log