docker build -t darkspore_server .
docker run -it -p 80:5000/tcp --name darkspore_server -td darkspore_server
docker exec -it darkspore_server bash /darkspore_server/run-docker.sh
