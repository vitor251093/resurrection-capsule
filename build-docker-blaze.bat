FOR /F "tokens=* USEBACKQ" %%F IN (`echo %cd%`) DO (SET pwd=%%F)
docker build -t darkspore_server .
docker run -it -p 42127:42127/tcp -v %pwd%\storage:/darkspore_server_storage --name darkspore_server_blaze -td darkspore_server
docker exec -it darkspore_server_blaze bash /darkspore_server/run-docker-blaze.sh
