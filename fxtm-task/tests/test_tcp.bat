@start cmd /c wsclient.exe -t TCP -n 1000 -s 192.168.1.2 2^> err_client_tcp01.log
@start cmd /c wsclient.exe -t TCP -n 1000 -s 192.168.1.2 2^> err_client_tcp02.log
rem @start cmd /c wsclient.exe -t TCP -n 1000 -s 192.168.1.2 2^> err_client_tcp03.log
@start cmd /c wsclient.exe -t TCP -n 1000 -s fe80::3801:bda6:d37f:804a 2^> err_client_tcp04.log
@start cmd /c wsclient.exe -t TCP -n 1000 -s fe80::3801:bda6:d37f:804a 2^> err_client_tcp05.log
rem @start cmd /c wsclient.exe -t TCP -n 1000 -s fe80::3801:bda6:d37f:804a 2^> err_client_tcp06.log
