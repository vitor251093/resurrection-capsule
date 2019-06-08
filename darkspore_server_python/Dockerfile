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
# Trying to fix the SSLv3_method issue
RUN pip install requests==2.6.0
RUN pip install pyopenssl
RUN pip install service_identity


# Needed parts to generate the EXE file
RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get install wine32 -y
RUN apt-get install wget -y
RUN apt-get install xvfb xinit -y
RUN apt-get install winetricks -y

RUN wget https://www.python.org/ftp/python/2.7.16/python-2.7.16.msi
RUN wget https://download.microsoft.com/download/7/9/6/796EF2E4-801B-4FC4-AB28-B59FBF6D907B/VCForPython27.msi
RUN wine msiexec /i python-2.7.16.msi -quiet -qn -norestart \
    && wine msiexec /i VCForPython27.msi -quiet -qn -norestart \
    && cd ~/.wine/drive_c/Python27 \
    && wine python.exe Scripts/pip.exe install pyinstaller \
    && wine python.exe Scripts/pip.exe install --upgrade pip \
    && wine python.exe Scripts/pip.exe install setuptools \
    && wine python.exe Scripts/pip.exe install flask \
    && wine python.exe Scripts/pip.exe install python-magic \
    && wine python.exe Scripts/pip.exe install python-libmagic \
    && wine python.exe Scripts/pip.exe install python-magic-bin==0.4.14 \
    && wine python.exe Scripts/pip.exe install twisted \
    && wine python.exe Scripts/pip.exe install pyopenssl \
    && wine python.exe Scripts/pip.exe install service_identity \
    && wine python.exe Scripts/pip.exe install pypiwin32 \
    && wine wineboot \
    && xvfb-run -a winetricks -q winxp \
    && xvfb-run -a winetricks -q vcrun2008

# Used to compress the server EXE into a single EXE file
RUN apt-get install p7zip-full -y

RUN cp ~/.wine/drive_c/windows/system32/msvcp90.dll ~/.wine/drive_c/Python27/msvcp90.dll
RUN cp ~/.wine/drive_c/windows/system32/msvcm90.dll ~/.wine/drive_c/Python27/msvcm90.dll

RUN python2 -m pip install --upgrade pip
RUN pip uninstall pyopenssl -y
RUN pip install requests==2.6.0
RUN pip install pyopenssl

COPY ./ /darkspore_server

# References
# https://www.andreafortuna.org/2017/12/27/how-to-cross-compile-a-python-script-into-a-windows-executable-on-linux/
# https://github.com/suchja/wine/issues/7
# https://github.com/sneumann/pwiz-appliance/blob/master/Dockerfile
