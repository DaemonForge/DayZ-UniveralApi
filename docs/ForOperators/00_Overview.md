# Universal Framework - Server Operator Documentation

## Overview

Universal Framework (UF) is a two-component system providing backend services for DayZ servers:

1. **DayZ Mod** (`@UFramework`) - Runs inside the DayZ server and client, providing API access to mods.
2. **UF Server Service** - A standalone backend service handling database, Discord, AI, and other features.

### Service Deployment Models

The UF Server Service is deployed differently depending on your operating system:

- **Windows**: Distributed as a standalone **Dashboard Application** (Electron).
  - Offers a full Graphical User Interface (GUI) for configuration, log viewing, and management.
  - No command-line usage required.
  - Install via standard `.exe` installer.

- **Linux**: Distributed as a consolidated **Binary Executable**.
  - Runs as a headless systemd service.
  - Managed via CLI commands and configuration files.
  - No Node.js installation or building from source required.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         DayZ Game Servers                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ Server US-1  │  │ Server EU-1  │  │ Server AU-1  │              │
│  │ @UFramework  │  │ @UFramework  │  │ @UFramework  │              │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │
└─────────┼─────────────────┼─────────────────┼───────────────────────┘
          │ HTTPS           │ HTTPS           │ HTTPS
          └────────────────┬┴─────────────────┘
                           │
┌──────────────────────────┼──────────────────────────────────────────┐
│                          ▼                                          │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                  UF Server Service                           │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐            │   │
│  │  │ Object  │ │ Player  │ │ Discord │ │ AI Chat │            │   │
│  │  │ Handler │ │ Handler │ │ Handler │ │ Handler │            │   │
│  │  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘            │   │
│  │       │           │           │           │                  │   │
│  │       └───────────┴─────┬─────┴───────────┘                  │   │
│  │                         │                                    │   │
│  └─────────────────────────┼────────────────────────────────────┘   │
│                            │                                        │
│  ┌─────────────────────────┼────────────────────────────────────┐   │
│  │                         ▼                                    │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │   │
│  │  │  MongoDB    │  │  Discord    │  │   OpenAI    │          │   │
│  │  │  Database   │  │  API        │  │   API       │          │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘          │   │
│  │                  External Services                           │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                        Backend Infrastructure                        │
└──────────────────────────────────────────────────────────────────────┘
```

## Communication Flow

The mod uses DayZ's built-in `RestApi` to make HTTPS calls:

```
DayZ Server (Enforce Script) <--HTTPS REST API--> UF Server Service <---> MongoDB / Discord / OpenAI
```

All requests are authenticated using server auth tokens (JWT for player requests).

## Key Features

| Feature | Description |
|---------|-------------|
| **Database Operations** | Load, Save, Query, Update, Transaction on MongoDB |
| **Discord Integration** | Link Steam to Discord, manage roles, send DMs, voice control |
| **AI Chat** | OpenAI-powered conversational AI for in-game NPCs |
| **Knowledge Bases** | Vector search for contextual AI responses |
| **Text-to-Speech** | Audio generation for in-game announcements |
| **Image Generation** | AI image generation with DDS conversion |
| **Message Queues** | Server-to-client and client-to-server messaging |
| **Server Query** | Query DayZ server status via Steam Query |
| **Logging** | Centralized logging from DayZ servers |
| **Quantum Random** | True random numbers from ANU QRNG |
| **Cryptography** | Hash generation for data integrity |

---

## Documentation Index

### Getting Started

| Document | Description |
|----------|-------------|
| [Installation Guide](01_Installation.md) | Complete installation for Windows and Linux |
| [Service Configuration](02_ServiceConfiguration.md) | All config.json options explained |
| [Mod Configuration](03_ModConfiguration.md) | DayZ mod UFramework.json settings |

### Infrastructure

| Document | Description |
|----------|-------------|
| [MongoDB Setup](04_MongoDB.md) | Database requirements, setup, maintenance |
| [Discord Setup](05_Discord.md) | Discord bot, OAuth2, and linking features |
| [Discord Templates](21_Discord_Templates.md) | Customizing Discord linking page templates |
| [SSL/HTTPS Configuration](06_SSL_HTTPS.md) | Certificate options, Let's Encrypt |

### Operations

| Document | Description |
|----------|-------------|
| [Troubleshooting](07_Troubleshooting.md) | Common issues and solutions |
| [FAQ](08_FAQ.md) | Frequently asked questions |
| [Logging & Monitoring](16_Logging_Monitoring.md) | Log locations, analysis, monitoring |
| [Backup & Recovery](17_Backup_Recovery.md) | Backup strategies, disaster recovery |
| [Multi-Server Setup](18_Multi_Server_Setup.md) | Running multiple DayZ servers |

### Platform Quick References

| Document | Description |
|----------|-------------|
| [Windows Quick Reference](19_Windows_QuickRef.md) | Windows-specific commands, paths, and tips |
| [Linux Quick Reference](20_Linux_QuickRef.md) | Linux-specific commands, paths, and tips |

### UI Guides (Windows Electron App)

| Document | Description |
|----------|-------------|
| [Configuration Editor](23_Config_Editor_UI.md) | Settings UI for managing service configuration |
| [KB Manager](24_KB_Manager_UI.md) | Knowledge Base Manager for AI document collections |
| [Globals Editor](25_Globals_Editor_UI.md) | Editor for global state variables |

### API Reference

| Document | Description |
|----------|-------------|
| [Object Endpoints](09_API_Endpoints_Object.md) | Object database API |
| [Player Endpoints](10_API_Endpoints_Player.md) | Player database API |
| [Globals & Status](11_API_Endpoints_Globals_Status.md) | Globals and health check API |
| [Messages Queue](12_API_Endpoints_Messages.md) | Message queue API |
| [Discord API](13_API_Endpoints_Discord.md) | Discord integration API |
| [Utility Endpoints](14_API_Endpoints_Utility.md) | Logger, Random, Crypto, ServerQuery |
| [AI Features](15_AI_Features.md) | AI Chat, TTS, Images, Knowledge Bases |

---

## Important Notes

### Server Wipe Required

**Installing Universal Framework on an existing server will require a server wipe.** The framework stores player data in MongoDB, and without the database populated from player connections, players will experience issues with any mods that depend on UF data.

Plan your installation during server downtime and communicate with your players about the wipe beforehand.

### System Requirements

#### UF Server Service
| Component | Requirement |
|-----------|-------------|
| MongoDB | 4.4 or later (required) |
| Node.js | 18+ (development only) |
| FFmpeg | Optional, for TTS audio |
| ImageMagick | Optional, for DDS image conversion |
| RAM | 2 GB minimum (4 GB recommended with MongoDB) |
| Disk | 1 GB + space for logs and MongoDB |

#### DayZ Server
| Component | Requirement |
|-----------|-------------|
| DayZ Server | Standard installation |
| Network | Outbound HTTPS to UF Service |
| Mod | `@UFramework` loaded |

### Supported Platforms

| Platform | Installation Method |
|----------|---------------------|
| Windows | Electron app with system tray interface |
| Linux | Binary with systemd service |

---

## Quick Start

1. **Install MongoDB** - Required database backend
2. **Download UF Server Service** - From GitHub releases
3. **Configure the service** - Edit `config.json` with your settings
4. **Set up Discord** (optional) - Create bot and configure OAuth2
5. **Add `@UFramework` mod** - To your DayZ server mod list
6. **Configure the mod** - Edit `$profile/UF/UFramework.json`
7. **Start your DayZ server** - Players can now connect

See [Installation Guide](01_Installation.md) for detailed step-by-step instructions.

---

## Endpoints Overview

| Base Path | Purpose | Auth Required |
|-----------|---------|---------------|
| `/Status` | Service health check | Optional |
| `/Object/*` | Object database operations | Server/Player |
| `/Player/*` | Player database operations | Server/Player |
| `/Globals/*` | Global state operations | Server/Player |
| `/Messages/*` | Message queue operations | Server/Player |
| `/Discord/*` | Discord integration | Server |
| `/AI/Chat/*` | AI Chat sessions | Server |
| `/TTS/*` | Text-to-Speech | Server |
| `/Images/*` | Image generation | Server |
| `/KB/*` | Knowledge Bases | Server |
| `/Logger/*` | Log ingestion | Server/Player |
| `/Random` | Quantum random numbers | Server/Player |
| `/Crypto/*` | Cryptographic operations | Server/Player |
| `/ServerQuery/*` | DayZ server status | Server/Player |
| `/GetAuth/*` | Player token generation | Server |

---

## Support

- **GitHub Issues**: https://github.com/daemonforge/DayZ-UniveralApi/issues
- **Wiki**: https://github.com/daemonforge/DayZ-UniveralApi/wiki
- **Releases**: https://github.com/daemonforge/DayZ-UniveralApi/releases

## Version Information

**Current Version**: 2.0.0

Check the [GitHub Releases](https://github.com/daemonforge/DayZ-UniveralApi/releases) page for the latest version and changelog.

## Tags
`operators`, `overview`, `architecture`, `deployment`, `service`, `reference`, `doc-usage`
