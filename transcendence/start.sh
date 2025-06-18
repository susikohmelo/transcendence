#!/bin/sh
# start.sh

# sighandler ()
# {
#     kill $PID
#     exit 0
# }
# trap sigint_handler SIGINT SIGTERM

# while true; do
#     #node server.js "$CRED_PATH/$CRED_KEY" "$CRED_PATH/$CRED_CERT" &
#     PID=$!
#     inotifywait -q -e modify ./server.js
#     kill $PID
#     echo '---------------------------------------------------------------------'
#     echo 'Restarting server...'
#     echo
# done

echo "Initializing SQLite database..."
/init-db.sh

echo "Starting NGINX..."
nginx -g 'daemon off;' &

echo "Starting Next.js app..."
npm start 

