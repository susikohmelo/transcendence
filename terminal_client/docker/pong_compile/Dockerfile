FROM ubuntu:18.04


#Install toolchain
COPY install_tools.sh .
RUN chmod +x install_tools.sh
RUN ./install_tools.sh

#Set variables
ARG username=cpp_user
ARG home_dir=/home/${username}
ARG websocketpp_examples_dir=${home_dir}/websocketpp_examples

#Create user
#RUN useradd -m ${username}
RUN adduser --disabled-password --gecos '' ${username}
RUN echo ${username}:${username} | chpasswd
RUN apt-get install -y sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
RUN sudo usermod -aG sudo ${username}

RUN mkdir ${websocketpp_examples_dir}
COPY *.sh ${websocketpp_examples_dir}/
COPY *.cpp ${websocketpp_examples_dir}/
COPY runme.sh .
RUN chmod +x runme.sh
RUN chown -R $username:$username ${websocketpp_examples_dir}

USER $username

#Install Boost
RUN chmod +x ${websocketpp_examples_dir}/install_boost.sh
RUN ${websocketpp_examples_dir}/install_boost.sh 1.67.0

#Download websocketpp
RUN chmod +x ${websocketpp_examples_dir}/download_websocketpp.sh
RUN ${websocketpp_examples_dir}/download_websocketpp.sh 0.8.1

#Create certificate
WORKDIR ${websocketpp_examples_dir}
RUN chmod +x ${websocketpp_examples_dir}/create_certificate.sh
RUN ${websocketpp_examples_dir}/create_certificate.sh

#Build websocketpp examples
#RUN make websocketpp_examples

ARG MODE=${MODE}
ENV MODE=${MODE}

RUN chmod +x ./runme.sh
ENTRYPOINT ["./runme.sh"]

