# Geomatrix
A learning adventure for myself from which may come out a fast-paced multiplayer game where interesting geometrical shapes fight each other to dominate the world of maths.
## Game Demo & Technical Walkthrough

[![Geomatrix Alpha Live Demo Video](https://img.youtube.com/vi/FMHLG_zqpoI/0.jpg)](https://youtu.be/FMHLG_zqpoI?si=HZ_pQRCz5DoegHpV)
Watch the live demo of the game↑
---
## Built With

This project wouldn't be possible without these incredible open-source libraries:
*   **[Raylib](https://github.com/raysan5/raylib)** — Handling our lightweight, high-performance 2D graphics rendering pipeline.
*   **[ENet](https://github.com/lsalzman/enet)** — Powering our low-latency UDP network layer with reliable/unreliable packet management.
--- 
# How to get started
1. Head over to [Releases](https://github.com/AliendsOG/Geomatrix/releases) and download the Server and Client executables.
2. Download the [maps](https://github.com/AliendsOG/Geomatrix/tree/main/maps)  folder and put it in the same directory as the executables, your local directory should look like this:
    ```
    ├── Client.exe
    ├── Server.exe
    └── maps/
        └── [map files]
    ```
3. Run the `Server.exe` first to start up the authoritative simulation framework.
4. Run `Client.exe` to join the game. For multiple players run multiple instances of the `Client.exe`.
5. It's play time now.

# How to Play

**Keyboard**
| Action | Control |
| :--- | :--- |
| **Movement** | `W` `A` `S` `D` |
| **Aiming** | `←` `↑` `→` `↓` (Arrow Keys) |
| **Shooting** | `E` |

**Controller** (PlayStation controller support not tested)
| Action | Control |
| :--- | :--- |
| **Movement** | `Left Joystick` |
| **Aiming** | `Right Joystick`  |
| **Shooting** | `Right Trigger (RT/R2)` |

# How it is built

**The Server**
* Built in C++, uses ENet for communication with the client over the UDP protocol for minimum latency;
* Implemented Fog Of War by calculating the distance between the players and sending each custom dynamically sized packets to save bandwidth and prevent cheating;
* The server processes the input for every player, runs the collision simulation and updates the players/projectiles position and health at a fixed tick rate, being the only source of truth;
* After each match it resets itself, getting ready for a new match;

**The Client**
* Built in C++, uses Raylib for rendering the UI and the game match itself;
* Uses interpolation to smooth out the position updates received from the server and render them at the specified frame rate;
* Loads the saved settings or data stored in the local file;
* Sends the inputs of the player to the server for processing;

# AI Use
I used Gemini during the development of this project for:
* **Brainstorming:** Discussing different architecture designs and client-server communication options.
* **Learning:** Asking AI about what specific Raylib or ENet functions are used for so I could more easily find what I was looking for.
* **Debugging:** Finding logic or syntax errors in the code.
*All core game loops, game mechanics, physics simulation, ENet network event handling, game state serialization, and Raylib rendering implementations were manually written and debugged by me.*



