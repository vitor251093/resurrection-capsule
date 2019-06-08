FOR /F "tokens=* USEBACKQ" %%F IN (`echo %cd%`) DO (SET pwd=%%F)
cd darkspore_server_python
docker build -t darkspore_server .
cd ..
docker run -it -v %pwd%\storage:/darkspore_server_storage --name darkspore_server -td darkspore_server
docker exec -it darkspore_server bash /darkspore_server/build-exe-docker.sh
