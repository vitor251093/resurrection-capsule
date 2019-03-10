FOR /F "tokens=* USEBACKQ" %%F IN (`echo %cd%`) DO (SET pwd=%%F)
docker build -t darkspore_server .
docker run -it -p 80:80/tcp -v %pwd%\storage:/darkspore_server_storage --name darkspore_server -td darkspore_server
docker exec -it darkspore_server bash /darkspore_server/run-docker.sh
