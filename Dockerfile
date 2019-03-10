FROM ubuntu

RUN apt-get update
RUN apt-get install python -y
RUN apt-get install python-dev -y
RUN apt-get install python-pip -y

RUN pip install --upgrade pip
RUN pip install setuptools
RUN pip install flask
RUN pip install twisted
RUN pip install python-magic
RUN pip install service_identity

COPY darkspore_server /darkspore_server

EXPOSE 5000
