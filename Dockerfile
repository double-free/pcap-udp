FROM gcc:latest
RUN apt-get update

RUN apt-get install libpcap-dev -y \
    && apt-get install less -y \
    && apt-get install vim -y

COPY . /usr/src/pcap_reader
WORKDIR /usr/src/pcap_reader/build

RUN g++ -g -Wall -o pcap_reader ../src/main.cpp ../src/csv/writer.cpp \
                ../src/md/preprocessor.cpp ../src/md/snapshot.cpp \
                -lpcap -lz \
                -std=c++11

CMD ["/bin/bash"]
