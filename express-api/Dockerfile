FROM node:12
COPY . /app
WORKDIR /app
RUN npm install
RUN add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
RUN apt-get update && apt-get install docker-ce docker-ce-cli containerd.io
CMD [ "npm", "start" ]
