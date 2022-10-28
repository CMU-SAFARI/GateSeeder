FROM ubuntu:20.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -y update
COPY xrt_202210.2.13.479_20.04-amd64-xrt.deb .
COPY xilinx-u280-gen3x16-xdma-1-202211-1-dev_1-3585755_all.deb .
RUN apt-get -y install ./xrt_202210.2.13.479_20.04-amd64-xrt.deb
RUN apt-get -y install ./xilinx-u280-gen3x16-xdma-1-202211-1-dev_1-3585755_all.deb
CMD echo "DONE"
