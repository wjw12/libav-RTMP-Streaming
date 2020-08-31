FROM node:12
COPY . /app
WORKDIR /app
RUN npm install
COPY --from=docker:19.03 /usr/local/bin/docker /usr/local/bin/
CMD [ "npm", "start" ]
