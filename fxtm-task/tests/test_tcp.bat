@start cmd /c wsclient.exe -p 40001 -t TCP -c 8 -n 1000 -s 192.168.1.5 2^> err_client_tcp01.log
@start cmd /c wsclient.exe -p 40001 -t TCP -c 8 -n 1000 -s 192.168.1.5 2^> err_client_tcp02.log
@start cmd /c wsclient.exe -p 40001 -t TCP -c 8 -n 1000 -s 192.168.1.5 2^> err_client_tcp03.log
@start cmd /c wsclient.exe -p 40001 -t TCP -c 8 -n 1000 -s fe80::3801:bda6:d37f:804a 2^> err_client_tcp04.log
@start cmd /c wsclient.exe -p 40001 -t TCP -c 8 -n 1000 -s fe80::3801:bda6:d37f:804a 2^> err_client_tcp05.log
@start cmd /c wsclient.exe -p 40001 -t TCP -c 8 -n 1000 -s fe80::3801:bda6:d37f:804a 2^> err_client_tcp06.log
