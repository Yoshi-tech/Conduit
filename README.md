# Conduit

```
  ██████╗ ██████╗ ███╗   ██╗██████╗ ██╗   ██╗██╗████████╗
 ██╔════╝██╔═══██╗████╗  ██║██╔══██╗██║   ██║██║╚══██╔══╝
 ██║     ██║   ██║██╔██╗ ██║██║  ██║██║   ██║██║   ██║   
 ██║     ██║   ██║██║╚██╗██║██║  ██║██║   ██║██║   ██║   
 ╚██████╗╚██████╔╝██║ ╚████║██████╔╝╚██████╔╝██║   ██║   
  ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝   ╚═╝  
```

A lightweight, local terminal chat application written in C++. Spin up a server, share your hostname, and anyone on the same network can join instantly — no accounts, no internet required.

![Conduit chat UI screenshot](docs/screenshot-chat.png)

---

## Features

- Multi-client broadcast chat over raw TCP sockets
- Split terminal UI — scrolling message window + input bar
- In-chat ASCII image rendering via `/image <url>`
- Built-in tic-tac-toe — challenge anyone in the chat
- Zero config — just a hostname and a username

---

## Installation

```bash
brew tap Yoshi-tech/conduit
brew install conduit
```

---

## Usage

### Start the server
Run this on the machine that will host the chat:
```bash
conduit-server
```

Find your hostname:
```bash
hostname
```

Share that hostname with anyone on the same network.

### Join as a client
```bash
conduit <hostname>
```

You'll be prompted for a username. Start chatting.

![Username prompt and connection screen](docs/screenshot-connect.png)

---

## Commands

| Command | Description |
|---|---|
| `/users` | List connected users |
| `/image <url>` | Fetch and render an image as ASCII art |
| `/tictactoe <username>` | Challenge a user to tic-tac-toe |
| `/accept` | Accept a pending tic-tac-toe challenge |
| `/move <1-9>` | Make a move (positions 1–9, left to right, top to bottom) |
| `/help` | Show command reference |

![ASCII image rendering example](docs/screenshot-image.png)

---

## Building from source

**Requirements:**
- C++11 or later
- ncurses (`brew install ncurses`)
- libcurl (`brew install curl`)

```bash
git clone https://github.com/Yoshi-tech/conduit.git
cd conduit
make
```

This produces two binaries:
- `chatServer` — run on the host machine
- `chatClient` — run on each client machine

---

## How it works

Conduit is built entirely on raw POSIX sockets with no networking libraries. The server accepts simultaneous TCP connections using a thread-per-client model, broadcasting messages to all connected peers. The client uses ncurses to render a split terminal UI — a scrolling message window on top and an input bar at the bottom.

**ASCII image rendering** works in four stages: libcurl fetches the raw image bytes over HTTP, stb_image decodes the JPEG/PNG into a flat RGB pixel array, stb_image_resize downsamples it to fit the terminal dimensions, and then a custom mapping converts each pixel's greyscale brightness to a character (`@`, `#`, `*`, `.`, ` `). The result is broadcast to all connected clients.

![Tic-tac-toe in the terminal](docs/screenshot-tictactoe.png)

---

## License

MIT
