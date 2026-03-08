# 12 — Dedicated Server  (Advanced)

A fully authoritative headless game server that runs without a window, GPU, or
audio device. Built on SparkEngine's `-headless` mode, this template demonstrates
a complete FPS deathmatch server with physics validation, AI bots, lag
compensation, match lifecycle, and live admin console — all server-side.

## Quick start

Requires an **installed** SparkEngine SDK (see the repository root
[README](../../README.md#prerequisites) for install instructions).

```bash
# Build the server module
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-SparkEngine-install-prefix>
cmake --build build --config Release

# Launch headless (no window, no GPU)
SparkEngine.exe -headless -game DedicatedServer.dll

# Or with -dedicated (alias)
SparkEngine.exe -dedicated -game DedicatedServer.dll
```

> `CMAKE_PREFIX_PATH` must point at the SparkEngine **install prefix** (the
> directory you passed to `cmake --install --prefix`), **not** the build tree or
> its `bin/` subdirectory.

Clients connect from a separate windowed SparkEngine instance (e.g. the
`08_MultiplayerArena` template) pointed at this server's IP and port.

## Engine systems used

| System | Role on the server |
|--------|--------------------|
| **NetworkManager** | UDP server, client connections, replication, lag compensation |
| **ECS (World)** | Entity creation, component queries, authoritative game state |
| **Physics (Bullet)** | Server-side colliders for hit validation and boundary enforcement |
| **AI (BehaviorTree)** | Combat AI for bot players — patrol, engage, take cover |
| **EventBus** | Damage, kills, connections, disconnections, round changes |
| **SaveSystem** | Periodic auto-save of match state and leaderboards |
| **Coroutines** | Timed match phase transitions (warmup → playing → cooldown) |
| **DayNight / Weather** | Server-authoritative world state replicated to clients |

**Disabled in headless mode** (automatically skipped by the engine):
Graphics, Rendering, Input, Audio.

## Server architecture

### Match lifecycle

```
WaitingForPlayers ──→ Warmup (10s) ──→ Playing ──→ Cooldown (8s) ──→ MapRotation ──╮
        ↑                                                                           │
        └───────────────────────────────────────────────────────────────────────────╯
```

- **WaitingForPlayers** — Server idles until 2+ human players connect
- **Warmup** — 10-second countdown, players can move but kills don't count
- **Playing** — Full deathmatch, score limit (50) or time limit (5 min)
- **Cooldown** — Scoreboard display, 8 seconds before rotation
- **MapRotation** — Transitions to next map (loops back for this template)

### Authoritative simulation

The server is the single source of truth. Clients send inputs; the server
validates, simulates, and replicates results:

1. **Client inputs** arrive via `NetworkManager::GetPendingInputs(clientID)`
2. **Server applies** movement at `10 units/s`, updates `Transform`
3. **Physics validation** — hit reports are rewound via `LagCompensator::RewindToTime()` and raycasted against historical hitboxes
4. **Replication** at 20 Hz — `Transform` and `HealthComponent` marked dirty via `MarkPropertyDirty()`
5. **Lag compensation snapshots** recorded every tick via `RecordSnapshot()`

### Bot system

Empty player slots are filled with AI bots running `BehaviorTree::CreateCombatBehavior()`.
Each bot has randomized accuracy (0.50–0.80) and aggression levels. When a human
player connects, a bot is removed to make room. Bots are indistinguishable from
players in the ECS — they have the same components (Transform, Health, RigidBody,
Collider, NetworkIdentity) plus an `AIComponent`.

### Physics-only arena

The server builds a complete physics arena (floor, 4 walls, 6 cover objects) using
only `RigidBodyComponent` and `ColliderComponent` — no `MeshRenderer`. This means
the server has accurate collision geometry for hit validation without ever
initializing the graphics pipeline.

## Server console commands

Available in the headless console (stdin) at runtime:

| Command | Description |
|---------|-------------|
| `sv_status` | Show server phase, map, player counts, match time |
| `sv_players` | List all players with kills, deaths, score, bot/human status |
| `sv_kick <name>` | Kick a human player by name |
| `sv_bot_add` | Manually add a bot |
| `sv_bot_remove` | Remove one bot |
| `sv_restart_round` | Force-restart the current round |
| `sv_end_round` | Force-end the current round, show scoreboard |
| `sv_save` | Force an immediate save of server state |
| `sv_set <var> <val>` | Change a server variable at runtime |

### Configurable variables (`sv_set`)

| Variable | Default | Description |
|----------|---------|-------------|
| `scorelimit` | 50 | Kills to win a round |
| `timelimit` | 300 | Round duration in seconds |
| `botfill` | 8 | Target player count (humans + bots) |
| `respawndelay` | 3 | Seconds before respawn after death |

## Ways to utilize the dedicated server

The headless server architecture is not limited to a single deathmatch box.
Below are the primary patterns for deploying and connecting dedicated servers
using SparkEngine's networking layer.

### Single authoritative server

The default setup. One server instance handles one match:

```
┌──────────────────┐
│  Dedicated Server │  SparkEngine.exe -headless -game DedicatedServer.dll
│  port 27015       │
└──────┬───────────┘
       │ UDP
  ┌────┴────┬────────┬────────┐
  │ Client  │ Client │ Client │  (windowed SparkEngine instances)
  └─────────┴────────┴────────┘
```

Run multiple instances on different ports for separate matches:

```bash
SparkEngine.exe -headless -game DedicatedServer.dll -port 27015
SparkEngine.exe -headless -game DedicatedServer.dll -port 27016
SparkEngine.exe -headless -game DedicatedServer.dll -port 27017
```

### Multi-server with seamless transfer (MMO-style)

Each server instance owns a zone or region of the world. Clients transfer
between servers when crossing zone boundaries. The `NetworkManager` supports
disconnecting from one server and connecting to another at runtime:

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  Zone: Forest   │     │  Zone: Town     │     │  Zone: Dungeon  │
│  port 27015     │◄───►│  port 27016     │◄───►│  port 27017     │
└────────┬────────┘     └────────┬────────┘     └────────┬────────┘
         │                       │                       │
     Players in              Players in              Players in
     the forest              the town                the dungeon
```

**How it works:**
1. Each zone server runs its own `DedicatedServer.dll` (or a zone-specific module)
2. When a player reaches a zone boundary, the current server sends a
   `ServerTransfer` reliable message containing the target server's address/port
   and a session token
3. The client calls `NetworkManager::Disconnect()` then
   `NetworkManager::Connect(newAddress, newPort, playerName)` with the token
4. The receiving server validates the token and spawns the player at the
   zone entry point
5. Player state (inventory, health, quests) is persisted via `SaveSystem` to
   shared storage (database, network filesystem) so any server can load it

This pattern scales horizontally — add more zone servers as the world grows.
Each server only simulates physics and AI for its own zone.

### Lobby + match server separation

A lightweight lobby server handles matchmaking, player queues, and party
management. When a match is found, it spins up (or assigns) a match server
and sends clients there:

```
┌──────────────────┐
│   Lobby Server   │  Lightweight, many connections, no physics
│   port 27000     │
└──────┬───────────┘
       │  "Match found → connect to 10.0.0.5:27015"
       │
  ┌────┴──────────────────┐
  │                       │
  ▼                       ▼
┌──────────────┐   ┌──────────────┐
│ Match Server │   │ Match Server │  Full authoritative simulation
│ port 27015   │   │ port 27016   │
└──────────────┘   └──────────────┘
```

The lobby server is another headless module that only uses `NetworkManager` and
`EventBus` — no physics, no AI. Match servers are full `DedicatedServer` instances.

### Relay / hub topology

A central hub server coordinates multiple game servers and routes players
between them. Used for persistent worlds with instanced content:

```
                    ┌───────────────┐
                    │   Hub Server  │  Routes, authenticates, coordinates
                    │   port 27000  │
                    └───────┬───────┘
               ┌────────────┼────────────┐
               ▼            ▼            ▼
        ┌────────────┐ ┌────────────┐ ┌────────────┐
        │ Overworld  │ │ Instance A │ │ Instance B │
        │ port 27015 │ │ port 27016 │ │ port 27017 │
        └────────────┘ └────────────┘ └────────────┘
```

Players stay connected to the hub for chat, friends, and party state. When
entering an instance, the hub tells the client to open a second connection to
the instance server. `NetworkManager` can maintain multiple connections
simultaneously via separate channels.

### Headless simulation / AI training

No clients at all. Run pure server-side simulation for:

- **AI training** — Bots play against each other at accelerated tick rates,
  collect performance metrics, iterate on behavior tree parameters
- **Load testing** — Simulate hundreds of fake clients to profile server
  performance before launch
- **Replay validation** — Replay recorded client inputs through the
  authoritative server to verify determinism or detect cheats
- **Automated testing** — CI/CD pipeline runs the server headless, connects
  test bots, validates game logic, checks for crashes

```bash
# Run at 120 Hz with 16 bots, no human players expected
SparkEngine.exe -headless -game DedicatedServer.dll -tickrate 120
```

### Spectator / broadcast server

A dedicated server instance that receives replicated state from a match server
and redistributes it to many spectator clients. The spectator server doesn't
run physics or AI — it just relays entity snapshots. This offloads spectator
bandwidth from the match server, keeping competitive play unaffected:

```
┌──────────────┐         ┌──────────────┐
│ Match Server │────────►│ Spectator    │────► 100+ viewer clients
│ (16 players) │ mirror  │ Relay Server │
└──────────────┘         └──────────────┘
```

## Default server configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| Port | 27015 | UDP listen port |
| Max players | 16 | Maximum concurrent human players |
| Bot fill | 8 | Maintain this many total players (humans + bots) |
| Tick rate | 60 Hz | Server simulation frequency (engine-managed) |
| Replication rate | 20 Hz | Entity state broadcast frequency |
| Round time limit | 300s | 5 minutes per round |
| Score limit | 50 | Kills to win |
| Warmup time | 10s | Pre-match countdown |
| Cooldown time | 8s | Post-match scoreboard display |
| Auto-save interval | 60s | Periodic state persistence |
| Respawn delay | 3s | Time before respawn after death |

## Differences from Template 08 (MultiplayerArena)

| | 08_MultiplayerArena | 12_DedicatedServer |
|---|---|---|
| **Execution** | Windowed, player hosts and plays | Headless, no window or GPU |
| **Authority** | Host is both server and client | Pure dedicated server, no local player |
| **Rendering** | Full render pipeline | Zero rendering — physics-only arena |
| **AI bots** | None | Auto-filling combat bots with behavior trees |
| **Match flow** | Free-form | Full lifecycle (warmup → playing → cooldown → rotation) |
| **Admin** | None | 10 console commands for live administration |
| **Persistence** | None | Auto-save match state every 60s |
| **Scalability** | Single instance | Multiple instances, zone transfers, lobby separation |

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-SparkEngine-install-prefix>
cmake --build build --config Release
```

Requires SparkEngine built with `SPARK_HEADLESS_SUPPORT=ON` (default).

This produces `DedicatedServer.dll` (Windows) or `libDedicatedServer.so`
(Linux). See [Quick start](#quick-start) for launch commands.

## Linux

The headless server runs on Linux via `main(argc, argv)` with `SIGINT`/`SIGTERM`
handlers for graceful shutdown. No X11, Wayland, or GPU drivers required.

```bash
./SparkEngine -headless -game libDedicatedServer.so
```
