FROM ubuntu

RUN apt-get update
RUN apt-get install python -y
RUN apt-get install python-dev -y
RUN apt-get install python-pip -y

RUN pip install --upgrade pip
RUN pip install setuptools
RUN pip install flask
RUN pip install python-magic
# Instead in Windows:
# pip install python-libmagic
# pip install python-magic-bin==0.4.14
# Also requires VCforPython from Microsoft

RUN pip install twisted
RUN pip install pyopenssl
RUN pip install service_identity

# Used to build the server into an EXE
RUN apt-get install lib32z1-dev -y
RUN python -m pip install cx_Freeze --upgrade

# Used to compress the server EXE into a single EXE file
RUN apt-get install p7zip-full -y
RUN apt-get install wget -y

COPY darkspore_server /darkspore_server
