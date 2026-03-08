# 08 — Multiplayer Arena  (Advanced)

A networked deathmatch arena demonstrating the engine's UDP client/server
networking, entity replication, client-side prediction, and lag compensation.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Server hosting | `NetworkManager::StartServer(port, maxClients)` |
| Client connection | `NetworkManager::Connect(address, port, playerName)` |
| Entity replication | `NetworkIdentity` — networkID, ownerClientID, isLocalAuthority |
| Property dirtying | `NetworkManager::MarkPropertyDirty()` |
| Client-side prediction | `NetworkManager::SendClientInput()`, `GetPendingInputs()` |
| Lag compensation | `LagCompensator::RecordSnapshot()`, `RewindToTime()` |
| Chat messages | `NetworkManager::SendChatMessage()` |
| Message channels | Unreliable, Reliable, ReliableOrdered |

## Controls

| Key | Action |
|-----|--------|
| H | Host a server (port 27015, max 8 players) |
| J | Join a server at 127.0.0.1:27015 |
| WASD | Move the local player |
| T | Send a chat message |

## Network architecture

- **Server**: Authoritative, records lag compensation snapshots at 20 Hz
- **Client**: Sends inputs with client-side prediction, server reconciles
- **Replication**: Transform and health auto-replicated for all NetworkIdentity entities
- **Protocol**: UDP with reliable/ordered channels for important messages

## Building

Requires an **installed** SparkEngine SDK (see the repository root
[README](../../README.md#prerequisites) for install instructions).

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-SparkEngine-install-prefix>
cmake --build build --config Release
```

> `CMAKE_PREFIX_PATH` must point at the SparkEngine **install prefix** (the
> directory you passed to `cmake --install --prefix`), **not** the build tree or
> its `bin/` subdirectory.

This produces `MultiplayerArena.dll` (Windows) or `libMultiplayerArena.so`
(Linux). Run it with the engine:

```bash
SparkEngine.exe -game MultiplayerArena.dll
```
