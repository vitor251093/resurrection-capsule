dockerid=$(docker container ls -a  | awk 'NR == 2 {print}' | awk '{print $1;}')
docker kill $dockerid
docker rm $dockerid
