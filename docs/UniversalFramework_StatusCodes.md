# Universal Framework - Status Codes & Constants

## Status Codes

All callbacks receive a `status` parameter (HTTP-style codes). Check before using data.

| Constant | Value | Meaning | Action |
|----------|-------|---------|--------|
| `UF_SUCCESS` | 200 | Operation successful | Use data |
| `UF_EMPTY` | 204 | No record found / empty result | Create default |
| `UF_AI_PENDING` | 202 | AI processing in progress | Poll again |
| `UF_AI_PROCESSING` | 102 | AI request still processing | Wait and poll |
| `UF_CLIENTERROR` | 400 | Client-side error | Check request |
| `UF_UNAUTHORIZED` | 401 | Auth failed | Check token |
| `UF_NOTFOUND` | 404 | Resource not found | Check ID |
| `UF_JSONERROR` | 406 | JSON parse failed | Check data format |
| `UF_TIMEOUT` | 408 | Request timed out | Retry |
| `UF_ERROR` | 418 | General error | Log and handle |
| `UF_NOTSETUP` | 424 | Service not configured (Discord) | Check config |
| `UF_TOOEARLY` | 425 | Request too early | Wait and retry |
| `UF_SERVERERROR` | 500 | Server-side error | Check service |

## Standard Callback Pattern

```enforce
void OnDataLoaded(int cid, int status, string oid, MyClass data) {
    switch (status) {
        case UF_SUCCESS:
            if (data) {
                // Use data
            }
            break;
        case UF_EMPTY:
            // No record - create default
            break;
        default:
            Print("Error: " + status);
            break;
    }
}
```

## Database Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `PLAYER_DB` | 100 | Per-player database |
| `OBJECT_DB` | 101 | Shared object database |

## Update Operations

| Constant | Value | Description |
|----------|-------|-------------|
| `UpdateOpts.SET` | "set" | Set field value |
| `UpdateOpts.PUSH` | "push" | Add to array |
| `UpdateOpts.PULL` | "pull" | Remove from array |
| `UpdateOpts.UNSET` | "unset" | Delete field |

## Quick Status Check

```enforce
// Success with data
if (status == UF_SUCCESS && data) { ... }

// Empty result
if (status == UF_EMPTY || !data) { ... }

// Any error
if (status != UF_SUCCESS) { ... }
```
