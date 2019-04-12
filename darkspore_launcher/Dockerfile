FROM ubuntu

RUN apt-get update

# Needed parts to generate the EXE file
RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get install mingw-w64 -y

COPY ./ /darkspore_launcher
