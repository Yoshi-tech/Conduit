# Conduit

```
  ██████╗ ██████╗ ███╗   ██╗██████╗ ██╗   ██╗██╗████████╗
 ██╔════╝██╔═══██╗████╗  ██║██╔══██╗██║   ██║██║╚══██╔══╝
 ██║     ██║   ██║██╔██╗ ██║██║  ██║██║   ██║██║   ██║   
 ██║     ██║   ██║██║╚██╗██║██║  ██║██║   ██║██║   ██║   
 ╚██████╗╚██████╔╝██║ ╚████║██████╔╝╚██████╔╝██║   ██║   
  ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝   ╚═╝  
```

A lightweight, local terminal chat application written in C++. Spin up a server, share your IP, and anyone on the same network can join instantly — no accounts, no internet required.

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

---

## Building from source

**Requirements:**
- C++11 or later
- ncurses (`brew install ncurses`)

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

Conduit is built on raw POSIX sockets. The server accepts multiple simultaneous TCP connections using a thread-per-client model, broadcasting messages to all connected peers. The client uses ncurses to render a split terminal UI — a scrolling message window on top, and an input bar at the bottom.

Built from scratch in C++ with no networking libraries.

---

## License

MIT
