# Clipong

A command line client for Super Pong 3D.

## Build Instructions

Run `make docker_compile` to build the application.

## Usage Instructions

Run `make run` to run the application. The application features a TUI that has clickable buttons. If the mouse cursor coordinates is off, click in a few different points anywhere in the screen to calibrate.

### Register Account and Login

After running `make run` you will have options to register or login. Click the buttons and follow the instructions on the screen. After successfull login, you are able to create tournaments, join them, or play in split screen.

### Create Tournament

Click `Create tournament` on the main menu. Follow the instructions on the screen. As the creator of the tournament, you can either wait for other players to join your tournament, or start the tournament. If the tournament is not full when starting, it will be filled with AI players.

### Join Tournament

Click 'Join tournament' on the main menu and enter your display name (alias). Click the tournament you want to join into. You will wait for the owner of the tournament to start the tournament.

### Split Screen

Click 'Split screen' on the main menu. Follow the instructions.

### Gameplay

In tournaments, when you are assigned to a game, you click `Ready to start match` when you are ready. The game starts when both players are ready, or immediately if opponent is an AI. You can then use the mouse or the arrow keys to move the paddle. Note that if you are playing against an AI, the AI will automatically match your input behaviour, i.e. it will send virtual keystrokes if you use your keyboard, or move a virtual mouse otherwise.

In split screen, the game starts when `Start split-screen` button is clicked. Player 1 uses WASD keys, player 2 uses arrow keys. Mouse is disabled for fair play.

If the mouse cursor coordinates or the rendering coordinates are off, click in a few different points anywhere in the screen to calibrate.

