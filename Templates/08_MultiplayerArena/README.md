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

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<SparkEngine install>
cmake --build build --config Release
```
