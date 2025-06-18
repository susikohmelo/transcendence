COMPOSE ?= docker-compose

.PHONY: all build up down restart logs clean

all:
	cd terminal_client && make docker_compile
	docker compose up --build

# --no-cache
build:
	$(COMPOSE) build

re: clean build up

up-dev:
	$(COMPOSE) up

up:
	$(COMPOSE) up -d --remove-orphans

down:
	$(COMPOSE) down -v --remove-orphans

restart: down up

logs:
	$(COMPOSE) logs -f

clean:
	$(COMPOSE) down -v --remove-orphans
	docker image prune -a -f
	docker volume prune -f
	docker network prune -f
	docker builder prune -f

ps:
	$(COMPOSE) ps
