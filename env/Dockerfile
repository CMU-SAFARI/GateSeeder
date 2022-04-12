FROM ubuntu:18.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -y update
RUN apt-get -y install unzip git zip findutils curl default-jdk wget
RUN wget https://git.esa.informatik.tu-darmstadt.de/tapasco/tapasco/-/jobs/artifacts/master/raw/toolflow/scala/build/distributions/tapasco_2021-01_amd64.deb?job=build_scala_tapasco_ubuntu_18_04 -O tapasco_2021-01_amd64.deb
RUN apt-get -y install ./tapasco_2021-01_amd64.deb
CMD echo "test"
