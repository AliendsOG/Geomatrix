# Geometrical-Brawl-Stars
A learning adventure for myself from which may come out a game like Brawl Stars but instead of brawlers there are interesting geometrical shapes.
---
This project wouldn't be possible without [Raylib](https://github.com/raysan5/raylib) and [Enet](https://github.com/lsalzman/enet)
# How to get started
1. Head over to [Releases](https://github.com/AliendsOG/Geometrical-Brawl-Stars/releases) and download the Server and Client exe, don't forget to download the [map folder](https://github.com/AliendsOG/Geomatrix/tree/main/Client%20Side/maps) and put it in the same directory as the exes;
2. Run the Server exe and then the Client exe;
3. for multiple players run multiple instances of the Client exe;
4. It's play time now;

# How to Play

**Keyboard**
- **Movement**: W A S D;
- **Aiming**: Arrow keys;
- **Shooting**: press E;

**Controller** (PS controler support not tested)
- **Movement**: Left Joystick;
- **Aiming**: Right Joystick;
- **Shooting**: Right Trigger;

# How is it build

**The Server**
- Built in C++, uses ENet for comunication with the client over the UDP protocol for minimum latency;
- Implemented Fog Of War by calculating the distance between the players and sending each custom dynamically sized packages to save bandwith and prevent cheating;
- The server processes the input for every player, runs the collision simulation and updates the players/projectiles position and health at a fixed tick rate, being the only source of truth;
- After each match it resets itself, getting ready for a new match;

**The Client**
- Built in C++, uses Raylib for rendering the UI and the game match itself;
- Uses interpolation to smooth out the position updates received from the server and render them at the specified frame rate;
- Loads the saved settings or data stored in the local life;
- Sends the inputs of the player to the server for processing;

