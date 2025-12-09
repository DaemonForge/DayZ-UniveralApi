# Building Universal Framework Service

This document covers how to build the Electron application with Azure Trusted Signing for code signing.

## Prerequisites

### Required Software

1. **Node.js** (v18 or later)
   - Download from https://nodejs.org/

2. **.NET 8.0 Runtime**
   - Required by Azure Trusted Signing client
   - Install via winget:
     ```powershell
     winget install Microsoft.DotNet.Runtime.8
     ```

3. **TrustedSigning PowerShell Module**
   - Automatically installed on first build, but you can pre-install:
     ```powershell
     Install-Module -Name TrustedSigning -Scope CurrentUser -Force
     ```

### Azure Trusted Signing Setup

You need an Azure Trusted Signing account configured. The following must be set up in Azure:

1. **Azure Trusted Signing Account** with:
   - Account Name: `DaemonForge`
   - Endpoint: `https://eus.codesigning.azure.net/`
   - Certificate Profile: `DaemonForge`

2. **Azure AD App Registration** (Service Principal) with:
   - `Trusted Signing Certificate Profile Signer` role assigned
   - Client ID and Client Secret generated

### Environment Configuration

Create a `.env.trustedsigning` file in the repository root (`DayZ-UniveralApi/.env.trustedsigning`) with:

```env
AZURE_CLIENT_ID=your-client-id-guid
AZURE_CLIENT_SECRET=your-client-secret
AZURE_TENANT_ID=your-tenant-id-guid
```

> ⚠️ **Security Note**: This file is in `.gitignore` and should NEVER be committed to the repository.

## Install Dependencies

```powershell
cd UFServerServiceV2
npm install
```

## Build Commands

### Signed Build (Recommended for Release)

Builds the Electron app and signs all executables with Azure Trusted Signing:

```powershell
npm run build
```

This command:
1. Loads Azure credentials from `../.env.trustedsigning`
2. Packages the Electron app for Windows
3. Signs all `.exe` files (main app, ffmpeg, ffprobe, winsw, sudo, elevate)
4. Creates and signs the NSIS installer
5. Outputs to `../Build/UFService/`

### Unsigned Build (For Development/Testing)

Builds without code signing:

```powershell
npm run build:unsigned
```

## Build Output

After a successful build, you'll find:

```
Build/UFService/
├── UniversalFrameworkService-Setup-2.0.0.exe    # Signed installer
├── UniversalFrameworkService-Setup-2.0.0.exe.blockmap
├── builder-effective-config.yaml
├── latest.yml
└── win-unpacked/                                 # Unpacked app files
    ├── UniversalFrameworkService.exe             # Main signed executable
    ├── ffmpeg.exe
    ├── texconv.exe
    └── resources/
        └── app.asar.unpacked/
            └── node_modules/
                ├── ffmpeg-static/ffmpeg.exe
                ├── ffprobe-static/bin/...
                └── node-windows/bin/...
```

## Verifying Code Signature

After building, verify the signature:

### PowerShell
```powershell
Get-AuthenticodeSignature "D:\Github\DayZ-UniveralApi\Build\UFService\UniversalFrameworkService-Setup-2.0.0.exe" | Format-List *
```

### Windows Explorer
1. Right-click the `.exe` file
2. Select **Properties**
3. Go to **Digital Signatures** tab
4. You should see "Kevin Hoddinott" as the signer
5. Click **Details** → should say "This digital signature is OK"

## Troubleshooting

### "AZURE_TENANT_ID not found" Error

The environment variables aren't being loaded. Check:
1. `.env.trustedsigning` file exists in the repository root
2. File contains all three variables: `AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`, `AZURE_TENANT_ID`
3. No extra spaces or quotes around values

### "Invoke-TrustedSigning not found" Error

The TrustedSigning PowerShell module isn't installed or accessible:

```powershell
# Check if module is available
Get-Module -ListAvailable TrustedSigning

# If not found, install it
Install-Module -Name TrustedSigning -Scope CurrentUser -Force

# Add local module path to PSModulePath if needed
$localModulePath = "$env:LOCALAPPDATA\WindowsPowerShell\Modules"
[Environment]::SetEnvironmentVariable("PSModulePath", "$localModulePath;" + [Environment]::GetEnvironmentVariable("PSModulePath", "User"), "User")
```

### "SignTool failed with exit code 3" Error

Usually means .NET 8.0 runtime is missing:

```powershell
winget install Microsoft.DotNet.Runtime.8
```

### Paths with Spaces Cause Signing Failures

The TrustedSigning module has issues with paths containing spaces. The `productName` in `package.json` is set to `UniversalFrameworkService` (no spaces) to avoid this.

### Certificate Validity

Azure Trusted Signing certificates are short-lived (typically 3 days) but automatically renew each time you sign. This is by design and doesn't affect the validity of signed applications.

## Configuration Reference

All build configuration is in `package.json` under the `build` key:

```json
{
  "build": {
    "appId": "com.daemonforge.ufserverservice",
    "productName": "UniversalFrameworkService",
    "icon": "public/icon256X256.png",
    "win": {
      "icon": "public/icon.ico",
      "azureSignOptions": {
        "publisherName": "Kevin Hoddinott",
        "endpoint": "https://eus.codesigning.azure.net/",
        "certificateProfileName": "DaemonForge",
        "codeSigningAccountName": "DaemonForge"
      }
    },
    "nsis": {
      "installerIcon": "public/icon.ico",
      "installerSidebar": "public/installerSidebar.bmp"
      // ... other NSIS options
    }
  }
}
```

## Other Build Targets

### Standalone Executables (pkg)

For building standalone Node.js executables (without Electron):

```powershell
# Windows only
npm run pkg

# Linux only
npm run pkg:linux

# Both platforms
npm run pkg:all
```

These are NOT code-signed and output to `../Build/Service/`.
