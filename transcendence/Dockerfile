FROM node:slim
ARG CRED_PATH CRED_CERT CRED_KEY COUNTRY STATE LOCALITY ORGANIZATION ORG_UNIT COMMON_NAME
RUN apt update && apt upgrade -y && apt install -y openssl gettext-base inotify-tools nginx sqlite3

# Setup SQLite

COPY init-db.sh /init-db.sh
RUN chmod +x /init-db.sh

# Setup SSL key and cert

RUN mkdir -p ${CRED_PATH}

RUN openssl req -x509 -nodes -days 365 \
  -newkey rsa:2048 \
  -keyout ${CRED_PATH}/${CRED_KEY} \
  -out ${CRED_PATH}/${CRED_CERT} \
  -subj "/C=${COUNTRY}/ST=${STATE}/L=${LOCALITY}/O=${ORGANIZATION}/OU=${ORG_UNIT}/CN=${COMMON_NAME}"

COPY nginx.conf.template nginx.conf.template
RUN envsubst '$CRED_PATH $CRED_CERT $CRED_KEY' < nginx.conf.template > /etc/nginx/sites-available/default
RUN rm nginx.conf.template


# Setup Nextjs 


WORKDIR /app

# ENV NODE_ENV development

COPY package*.json ./
RUN npm install
COPY . .
RUN npm run build

ENV NODE_ENV production

# Initialize container startup

EXPOSE 8000 8080

COPY start.sh /start.sh
RUN chmod +x /start.sh

ENTRYPOINT [ "/start.sh" ]
