#!/bin/bash
#
# Universal Framework Service - Linux Installation Script
# This script installs the UF Server Service as a systemd service
#

set -e

# Configuration
SERVICE_NAME="ufserverservice"
INSTALL_DIR="/opt/ufserverservice"
DATA_DIR="/var/lib/ufserverservice"
LOG_DIR="/var/log/ufserverservice"
CONFIG_DIR="/etc/ufserverservice"
SERVICE_USER="ufservice"
BINARY_NAME="ufserverservice-linux"
AUTO_YES=false

# MongoDB credentials (populated by setup_mongodb_security)
MONGO_USER=""
MONGO_PASS=""
MONGO_DB="DayZ"
MONGO_AUTH_URI=""

# Configuration wizard results (populated by configure_wizard)
CFG_PORT="443"
CFG_DB_NAME="DayZ"
CFG_SERVER_COUNT="1"
CFG_SERVER_AUTH_TOKENS=()    # array of tokens
CFG_SERVER_AUTH_LABELS=()    # array of labels
CFG_SSL_MODE=""              # letsencrypt | selfsigned | proxy | tunnel | none
CFG_LE_DOMAIN=""
CFG_LE_EMAIL=""
CFG_LE_ALTNAMES=""
CFG_CERT_PATH=""
CFG_CERT_KEY_PATH=""
CFG_PROXY_SUBDOMAIN=""
CFG_PROXY_TOKEN=""
CFG_PROXY_PRIMARY_DOMAIN=""
CFG_TUNNEL_TOKEN=""
CFG_DISCORD_BOT_TOKEN=""
CFG_DISCORD_CLIENT_ID=""
CFG_DISCORD_CLIENT_SECRET=""
CFG_DISCORD_GUILD_ID=""
CFG_OPENAI_KEY=""
FORCE_RECONFIG=false
IS_RECONFIGURE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Escape a string for safe embedding in JSON values
json_escape() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g; s/\t/\\t/g'
}

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}==== $1 ====${NC}"
}

print_step() {
    local current=$1
    local total=$2
    local desc=$3
    echo
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Step ${current}/${total}: ${desc}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

# Display welcome banner
print_welcome() {
    echo
    echo -e "${GREEN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║   Universal Framework Service - Linux Installer  ║${NC}"
    echo -e "${GREEN}╠══════════════════════════════════════════════════╣${NC}"
    echo -e "${GREEN}║                                                  ║${NC}"
    echo -e "${GREEN}║  This installer will:                            ║${NC}"
    echo -e "${GREEN}║                                                  ║${NC}"
    echo -e "${GREEN}║   1. Install dependencies (MongoDB, FFmpeg, etc) ║${NC}"
    echo -e "${GREEN}║   2. Create a dedicated service user             ║${NC}"
    echo -e "${GREEN}║   3. Set up directories & install the binary     ║${NC}"
    echo -e "${GREEN}║   4. Secure MongoDB with authentication          ║${NC}"
    echo -e "${GREEN}║   5. Walk you through service configuration      ║${NC}"
    echo -e "${GREEN}║   6. Generate configuration & systemd service    ║${NC}"
    echo -e "${GREEN}║   7. Verify the installation                     ║${NC}"
    echo -e "${GREEN}║                                                  ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════╝${NC}"
    echo
    if $AUTO_YES; then
        echo -e "${YELLOW}  Running in unattended mode (-y). Prompts will be auto-accepted.${NC}"
        echo
    fi
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Detect package manager
detect_package_manager() {
    if command -v apt-get &> /dev/null; then
        PKG_MANAGER="apt"
        PKG_UPDATE="apt-get update"
        PKG_INSTALL="apt-get install -y"
    elif command -v dnf &> /dev/null; then
        PKG_MANAGER="dnf"
        PKG_UPDATE="dnf check-update || true"
        PKG_INSTALL="dnf install -y"
    elif command -v yum &> /dev/null; then
        PKG_MANAGER="yum"
        PKG_UPDATE="yum check-update || true"
        PKG_INSTALL="yum install -y"
    elif command -v pacman &> /dev/null; then
        PKG_MANAGER="pacman"
        PKG_UPDATE="pacman -Sy"
        PKG_INSTALL="pacman -S --noconfirm"
    elif command -v zypper &> /dev/null; then
        PKG_MANAGER="zypper"
        PKG_UPDATE="zypper refresh"
        PKG_INSTALL="zypper install -y"
    else
        PKG_MANAGER="unknown"
    fi
    print_status "Detected package manager: $PKG_MANAGER"
}

# Install system dependencies
install_dependencies() {
    print_header "Installing Dependencies"
    
    if [[ "$PKG_MANAGER" == "unknown" ]]; then
        print_warning "Unknown package manager. Please install dependencies manually:"
        print_warning "  - ffmpeg (for TTS audio processing)"
        print_warning "  - imagemagick (for image DDS conversion)"
        print_warning "  - mongodb-org (database) — see https://www.mongodb.com/docs/manual/installation/"
        return
    fi
    
    local install_ffmpeg=false
    local install_imagemagick=false
    local install_mongodb=false
    
    # Check for FFmpeg
    if ! command -v ffmpeg &> /dev/null; then
        print_status "FFmpeg not found, will install..."
        install_ffmpeg=true
    else
        print_status "FFmpeg already installed: $(ffmpeg -version 2>&1 | head -n1)"
    fi
    
    # Check for ImageMagick
    if ! command -v convert &> /dev/null && ! command -v magick &> /dev/null; then
        print_status "ImageMagick not found, will install..."
        install_imagemagick=true
    else
        print_status "ImageMagick already installed"
    fi
    
    # Check for MongoDB
    if ! command -v mongod &> /dev/null && ! systemctl is-active --quiet mongod 2>/dev/null; then
        print_status "MongoDB not found, will install..."
        install_mongodb=true
    else
        print_status "MongoDB already installed"
    fi
    
    if $install_ffmpeg || $install_imagemagick; then
        local do_media=true
        if ! $AUTO_YES; then
            read -p "Install missing media dependencies (ffmpeg, imagemagick)? [Y/n] " -n 1 -r
            echo
            [[ $REPLY =~ ^[Nn]$ ]] && do_media=false || true
        fi
        if $do_media; then
            print_status "Updating package lists..."
            $PKG_UPDATE || print_warning "Package list update had issues, continuing anyway..."
            
            case $PKG_MANAGER in
                apt)
                    $install_ffmpeg && { $PKG_INSTALL ffmpeg || print_warning "FFmpeg installation failed."; }
                    $install_imagemagick && { $PKG_INSTALL imagemagick || print_warning "ImageMagick installation failed."; }
                    ;;
                dnf|yum)
                    # Enable RPM Fusion for ffmpeg on Fedora/RHEL
                    if $install_ffmpeg; then
                        if [[ "$PKG_MANAGER" == "dnf" ]]; then
                            dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm 2>/dev/null || true
                        fi
                        $PKG_INSTALL ffmpeg || print_warning "FFmpeg installation failed. You may need to enable RPM Fusion."
                    fi
                    $install_imagemagick && $PKG_INSTALL ImageMagick
                    ;;
                pacman)
                    $install_ffmpeg && { $PKG_INSTALL ffmpeg || print_warning "FFmpeg installation failed."; }
                    $install_imagemagick && { $PKG_INSTALL imagemagick || print_warning "ImageMagick installation failed."; }
                    ;;
                zypper)
                    $install_ffmpeg && { $PKG_INSTALL ffmpeg || print_warning "FFmpeg installation failed."; }
                    $install_imagemagick && { $PKG_INSTALL ImageMagick || print_warning "ImageMagick installation failed."; }
                    ;;
            esac
        fi
    fi
    
    # MongoDB requires special handling — official repo needed on Debian/Ubuntu
    if $install_mongodb; then
        local do_mongo=true
        if ! $AUTO_YES; then
            read -p "Install MongoDB from official repository? [Y/n] " -n 1 -r
            echo
            [[ $REPLY =~ ^[Nn]$ ]] && do_mongo=false || true
        fi
        if $do_mongo; then
            install_mongodb_from_repo || print_warning "MongoDB installation had issues. You may need to install it manually."
        fi
    fi
}

# Install MongoDB from the official MongoDB repository
# Supports Debian, Ubuntu, RHEL/Fedora, and Arch
install_mongodb_from_repo() {
    local MONGO_VERSION="8.0"
    
    print_status "Installing MongoDB $MONGO_VERSION from official repository..."
    
    case $PKG_MANAGER in
        apt)
            # Install prerequisites
            $PKG_INSTALL gnupg curl
            
            # Detect distro and codename
            local distro=""
            local codename=""
            if [[ -f /etc/os-release ]]; then
                . /etc/os-release
                distro="$ID"
                codename="$VERSION_CODENAME"
            fi
            
            # Determine repo URL based on distro
            local repo_url=""
            case "$distro" in
                debian)
                    repo_url="https://repo.mongodb.org/apt/debian ${codename}/mongodb-org/${MONGO_VERSION} main"
                    ;;
                ubuntu)
                    repo_url="https://repo.mongodb.org/apt/ubuntu ${codename}/mongodb-org/${MONGO_VERSION} multiverse"
                    ;;
                *)
                    # Try Debian as fallback for Debian-based distros
                    if [[ -n "$codename" ]]; then
                        repo_url="https://repo.mongodb.org/apt/debian ${codename}/mongodb-org/${MONGO_VERSION} main"
                        print_warning "Unrecognized apt-based distro '$distro', trying Debian repo..."
                    else
                        print_error "Cannot determine distro codename. Please install MongoDB manually."
                        print_error "See: https://www.mongodb.com/docs/manual/installation/"
                        return 1
                    fi
                    ;;
            esac
            
            # Import MongoDB GPG key
            print_status "Importing MongoDB GPG key..."
            curl -fsSL "https://www.mongodb.org/static/pgp/server-${MONGO_VERSION}.asc" | \
                gpg --dearmor -o "/usr/share/keyrings/mongodb-server-${MONGO_VERSION}.gpg" 2>/dev/null || {
                    print_error "Failed to import MongoDB GPG key"
                    return 1
                }
            
            # Add MongoDB repository
            print_status "Adding MongoDB repository for $distro $codename..."
            echo "deb [ signed-by=/usr/share/keyrings/mongodb-server-${MONGO_VERSION}.gpg ] ${repo_url}" | \
                tee "/etc/apt/sources.list.d/mongodb-org-${MONGO_VERSION}.list" > /dev/null
            
            # Update and install
            print_status "Updating package lists..."
            apt-get update
            
            print_status "Installing mongodb-org..."
            $PKG_INSTALL mongodb-org || {
                print_error "MongoDB installation failed."
                print_error "Your distro/version ($distro $codename) may not be supported by MongoDB ${MONGO_VERSION}."
                print_error "See: https://www.mongodb.com/docs/manual/installation/"
                return 1
            }
            ;;
        dnf|yum)
            # Create MongoDB repo file for RHEL/Fedora
            print_status "Adding MongoDB YUM/DNF repository..."
            cat > /etc/yum.repos.d/mongodb-org-${MONGO_VERSION}.repo << MONGOEOF
[mongodb-org-${MONGO_VERSION}]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/\$releasever/mongodb-org/${MONGO_VERSION}/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-${MONGO_VERSION}.asc
MONGOEOF
            
            print_status "Installing mongodb-org..."
            $PKG_INSTALL mongodb-org || {
                print_error "MongoDB installation failed. See: https://www.mongodb.com/docs/manual/installation/"
                return 1
            }
            ;;
        pacman)
            # Arch Linux — MongoDB is in AUR, suggest manual install
            print_warning "MongoDB is not in the official Arch repos."
            print_warning "Install from AUR: yay -S mongodb-bin"
            print_warning "Or see: https://wiki.archlinux.org/title/MongoDB"
            return 1
            ;;
        *)
            print_error "Automatic MongoDB installation not supported for $PKG_MANAGER."
            print_error "See: https://www.mongodb.com/docs/manual/installation/"
            return 1
            ;;
    esac
    
    # Start and enable MongoDB
    print_status "Starting MongoDB service..."
    systemctl daemon-reload
    systemctl start mongod || {
        print_error "Failed to start MongoDB. Check: systemctl status mongod"
        return 1
    }
    systemctl enable mongod
    print_status "MongoDB installed and running."
}

# Secure MongoDB with authentication and localhost binding
# Generates a random password, creates a database user, and enables auth
setup_mongodb_security() {
    local db_name="${1:-DayZ}"
    local db_user="ufservice"
    
    print_header "Securing MongoDB"
    
    # Check if MongoDB is running
    if ! systemctl is-active --quiet mongod 2>/dev/null; then
        print_warning "MongoDB is not running. Starting it..."
        systemctl start mongod || {
            print_error "Could not start MongoDB. Skipping security setup."
            return 1
        }
    fi
    
    # Determine which shell command is available (mongosh vs legacy mongo)
    local MONGO_CMD=""
    if command -v mongosh &> /dev/null; then
        MONGO_CMD="mongosh"
    elif command -v mongo &> /dev/null; then
        MONGO_CMD="mongo"
    else
        print_warning "Neither mongosh nor mongo shell found. Skipping security setup."
        print_warning "You should manually secure MongoDB. See: https://www.mongodb.com/docs/manual/administration/security-checklist/"
        return 1
    fi
    
    # Check if auth is already enabled
    local auth_enabled=false
    if grep -qE '^\s*authorization:\s*enabled' /etc/mongod.conf 2>/dev/null; then
        auth_enabled=true
    fi
    if grep -qE '^\s*security:' /etc/mongod.conf 2>/dev/null && grep -qE '^\s*authorization:\s*enabled' /etc/mongod.conf 2>/dev/null; then
        auth_enabled=true
    fi
    
    if $auth_enabled; then
        print_status "MongoDB authentication is already enabled."
        print_status "Skipping security setup. Ensure your config.json has the correct connection string."
        return 0
    fi
    
    # --- Step 1: Ensure bindIp is 127.0.0.1 ---
    print_status "Ensuring MongoDB binds to localhost only..."
    if grep -qE '^\s*bindIp:' /etc/mongod.conf 2>/dev/null; then
        # Verify it's localhost
        if ! grep -qE '^\s*bindIp:\s*127\.0\.0\.1' /etc/mongod.conf 2>/dev/null; then
            print_warning "MongoDB bindIp is not 127.0.0.1 — updating..."
            sed -i 's/^\(\s*bindIp:\).*/\1 127.0.0.1/' /etc/mongod.conf
        else
            print_status "MongoDB already bound to 127.0.0.1 (good)"
        fi
    else
        print_status "bindIp not found in config, adding..."
        # Add under net: section or create it
        if grep -qE '^\s*net:' /etc/mongod.conf 2>/dev/null; then
            sed -i '/^\s*net:/a\  bindIp: 127.0.0.1' /etc/mongod.conf
        else
            echo -e "\nnet:\n  bindIp: 127.0.0.1" >> /etc/mongod.conf
        fi
    fi
    
    # --- Step 2: Generate a strong random password ---
    # 48 chars, alphanumeric + URI-safe special chars (no @, :, /, #, ?)
    local db_pass
    db_pass=$(tr -dc 'A-Za-z0-9_+~.!*-' < /dev/urandom | head -c 48)
    
    # Save credentials to file IMMEDIATELY so they survive a crash
    local cred_file="$CONFIG_DIR/.mongodb_credentials"
    mkdir -p "$CONFIG_DIR" 2>/dev/null || true
    cat > "$cred_file" << CREDEOF
# MongoDB credentials generated by UF Service installer
# Date: $(date -Iseconds 2>/dev/null || date)
# These are saved here as a safety net. You can delete this file
# after confirming your config.json has the correct DBServer URI.
MONGO_USER=${db_user}
MONGO_PASS=${db_pass}
MONGO_DB=${db_name}
MONGO_AUTH_URI=mongodb://${db_user}:${db_pass}@127.0.0.1:27017/${db_name}
CREDEOF
    chmod 600 "$cred_file" 2>/dev/null || true
    chown root:root "$cred_file" 2>/dev/null || true
    print_status "Credentials saved to $cred_file (as safety backup)"
    
    print_status "Creating MongoDB user '$db_user' for database '$db_name'..."
    
    # --- Step 3: Create the database user (while auth is still off) ---
    $MONGO_CMD --quiet --eval "
        // Switch to the target database
        db = db.getSiblingDB('${db_name}');
        
        // Remove existing user if present (idempotent)
        try { db.dropUser('${db_user}'); } catch(e) {}
        
        // Create user with readWrite on the service database
        db.createUser({
            user: '${db_user}',
            pwd: '${db_pass}',
            roles: [{ role: 'readWrite', db: '${db_name}' }]
        });
        
        print('User created successfully');
    " 2>/dev/null || {
        print_error "Failed to create MongoDB user. You may need to set up auth manually."
        print_warning "See: https://www.mongodb.com/docs/manual/tutorial/enable-authentication/"
        return 1
    }
    
    # --- Step 4: Enable authentication in mongod.conf ---
    print_status "Enabling MongoDB authentication..."
    if grep -qE '^\s*security:' /etc/mongod.conf 2>/dev/null; then
        # security: section exists
        if grep -qE '^\s*authorization:' /etc/mongod.conf 2>/dev/null; then
            # authorization line exists — ensure it says "enabled" (may be "disabled" from resecure flow)
            sed -i 's/^\(\s*authorization:\).*/\1 enabled/' /etc/mongod.conf
        else
            # No authorization line yet — add it under security:
            sed -i '/^\s*security:/a\  authorization: enabled' /etc/mongod.conf
        fi
    else
        # Add security section
        echo -e "\nsecurity:\n  authorization: enabled" >> /etc/mongod.conf
    fi
    
    # --- Step 5: Restart MongoDB with auth ---
    print_status "Restarting MongoDB with authentication enabled..."
    systemctl restart mongod || {
        print_error "Failed to restart MongoDB after enabling auth."
        print_error "Check config: /etc/mongod.conf"
        print_error "Check status: systemctl status mongod"
        return 1
    }
    
    # --- Step 6: Verify auth works ---
    sleep 2
    $MONGO_CMD --quiet "mongodb://${db_user}:${db_pass}@127.0.0.1:27017/${db_name}" --eval "db.runCommand({ping:1})" &>/dev/null && {
        print_status "MongoDB authentication verified successfully!"
    } || {
        print_warning "Could not verify auth connection, but user was created."
        print_warning "You may need to check your connection string manually."
    }
    
    # Store credentials for config generation
    MONGO_USER="$db_user"
    MONGO_PASS="$db_pass"
    MONGO_DB="$db_name"
    MONGO_AUTH_URI="mongodb://${db_user}:${db_pass}@127.0.0.1:27017/${db_name}"
    
    echo
    echo -e "${GREEN}┌──────────────────────────────────────────────────┐${NC}"
    echo -e "${GREEN}│          MongoDB Credentials (SAVE THESE!)       │${NC}"
    echo -e "${GREEN}├──────────────────────────────────────────────────┤${NC}"
    echo -e "${GREEN}│${NC}  Database:  ${BLUE}${db_name}${NC}"
    echo -e "${GREEN}│${NC}  Username:  ${BLUE}${db_user}${NC}"
    echo -e "${GREEN}│${NC}  Password:  ${BLUE}${db_pass}${NC}"
    echo -e "${GREEN}│${NC}  URI:       ${BLUE}mongodb://${db_user}:****@127.0.0.1:27017/${db_name}${NC}"
    echo -e "${GREEN}└──────────────────────────────────────────────────┘${NC}"
    echo
    print_status "These credentials will be saved in your config.json automatically."
    print_status "Backup copy saved to: $cred_file"
}

# Check for required dependencies
check_dependencies() {
    print_header "Checking Dependencies"
    
    local missing_critical=()
    local missing_optional=()
    
    # Check for MongoDB
    if ! command -v mongod &> /dev/null && ! systemctl is-active --quiet mongod 2>/dev/null; then
        missing_critical+=("mongodb")
    else
        print_status "MongoDB: OK"
    fi
    
    # Check for FFmpeg (required for TTS)
    if ! command -v ffmpeg &> /dev/null; then
        missing_optional+=("ffmpeg")
        print_warning "FFmpeg: NOT FOUND (TTS features will not work)"
    else
        print_status "FFmpeg: OK ($(ffmpeg -version 2>&1 | head -n1 | cut -d' ' -f3))"
    fi
    
    # Check for ImageMagick (required for image DDS conversion)
    if ! command -v convert &> /dev/null && ! command -v magick &> /dev/null; then
        missing_optional+=("imagemagick")
        print_warning "ImageMagick: NOT FOUND (Image DDS conversion will not work)"
    else
        # Check if DDS format is supported
        local dds_support=false
        if command -v magick &> /dev/null; then
            magick identify -list format 2>/dev/null | grep -q "DDS" && dds_support=true
        elif command -v identify &> /dev/null; then
            identify -list format 2>/dev/null | grep -q "DDS" && dds_support=true
        fi
        
        if $dds_support; then
            print_status "ImageMagick: OK (with DDS support)"
        else
            print_warning "ImageMagick: Installed but DDS format not supported"
            missing_optional+=("imagemagick-dds")
        fi
    fi
    
    if [[ ${#missing_critical[@]} -gt 0 ]]; then
        print_error "Missing critical dependencies: ${missing_critical[*]}"
        print_error "MongoDB is required for this service to work."
        echo
        print_status "Re-run the installer to auto-install MongoDB from the official repo:"
        echo "  sudo $0 -y ./ufserverservice-linux"
        echo
        print_status "Or install manually — see: https://www.mongodb.com/docs/manual/installation/"
        echo
        if $AUTO_YES; then
            print_warning "Continuing without MongoDB (unattended mode)..."
        else
            read -p "Continue without MongoDB? [y/N] " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
        fi
    fi
    
    if [[ ${#missing_optional[@]} -gt 0 ]]; then
        print_warning "Missing optional dependencies: ${missing_optional[*]}"
        print_warning "Some features may not work without these."
    fi
}

# Create service user
create_user() {
    if id "$SERVICE_USER" &>/dev/null; then
        print_status "Service user '$SERVICE_USER' already exists"
    else
        print_status "Creating service user '$SERVICE_USER'..."
        useradd --system --no-create-home --shell /usr/sbin/nologin "$SERVICE_USER"
    fi
}

# Create directories
create_directories() {
    print_status "Creating directories..."
    
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$INSTALL_DIR/bin"
    mkdir -p "$DATA_DIR"/{logs,greenlock,audioCache,temp}
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$LOG_DIR"
    
    # Create symlinks for data directories
    ln -sfn "$DATA_DIR/logs" "$INSTALL_DIR/logs"
    ln -sfn "$DATA_DIR/greenlock" "$INSTALL_DIR/greenlock"
    ln -sfn "$DATA_DIR/audioCache" "$INSTALL_DIR/audioCache"
    ln -sfn "$DATA_DIR/temp" "$INSTALL_DIR/temp"
}

# Install binary
install_binary() {
    local binary_path="$1"
    
    if [[ ! -f "$binary_path" ]]; then
        print_error "Binary not found at: $binary_path"
        print_error "Please provide the path to the $BINARY_NAME binary"
        exit 1
    fi
    
    # Validate binary before installing
    validate_binary "$binary_path"
    
    print_status "Installing binary..."
    cp "$binary_path" "$INSTALL_DIR/$BINARY_NAME"
    chmod +x "$INSTALL_DIR/$BINARY_NAME"
}

# Validate that a binary is a proper ELF executable for this architecture
validate_binary() {
    local bin_path="$1"
    
    # Check file size (pkg binaries are typically 50MB+)
    local file_size
    file_size=$(stat -c%s "$bin_path" 2>/dev/null || echo 0)
    if [[ $file_size -lt 1000000 ]]; then
        print_error "Binary is suspiciously small (${file_size} bytes). It may be corrupted or incomplete."
        print_error "Expected a pkg binary of 50MB+ for a Node.js application."
        exit 1
    fi
    
    # Check it's an ELF binary
    if command -v file &>/dev/null; then
        local file_type
        file_type=$(file -b "$bin_path" 2>/dev/null)
        if [[ ! "$file_type" =~ ELF ]]; then
            print_error "Binary is not a Linux ELF executable."
            print_error "  Detected type: $file_type"
            print_error "  This may be a Windows binary, a text file, or a corrupted download."
            exit 1
        fi
        
        # Check architecture matches
        local server_arch
        server_arch=$(uname -m)
        if [[ "$file_type" =~ "x86-64" ]] && [[ "$server_arch" != "x86_64" ]]; then
            print_error "Architecture mismatch: binary is x86-64 but this server is $server_arch"
            exit 1
        fi
        if [[ "$file_type" =~ "aarch64" ]] && [[ "$server_arch" != "aarch64" ]]; then
            print_error "Architecture mismatch: binary is aarch64 but this server is $server_arch"
            exit 1
        fi
        print_status "Binary validated: ELF $server_arch, $(numfmt --to=iec $file_size 2>/dev/null || echo "${file_size} bytes")"
    fi
    
    # Check glibc version compatibility (node22 needs glibc >= 2.28)
    if command -v ldd &>/dev/null; then
        local glibc_version
        glibc_version=$(ldd --version 2>&1 | head -1 | grep -oP '[0-9]+\.[0-9]+$' || echo "")
        if [[ -n "$glibc_version" ]]; then
            local glibc_major glibc_minor
            glibc_major=$(echo "$glibc_version" | cut -d. -f1)
            glibc_minor=$(echo "$glibc_version" | cut -d. -f2)
            if [[ $glibc_major -lt 2 ]] || [[ $glibc_major -eq 2 && $glibc_minor -lt 28 ]]; then
                print_error "glibc $glibc_version is too old. Node.js 22 requires glibc >= 2.28."
                print_error "Upgrade your OS or use a newer distribution (Debian 10+, Ubuntu 20.04+, RHEL 8+)."
                exit 1
            fi
            print_status "glibc version: $glibc_version (>= 2.28 required) ✓"
        fi
    fi
    
    # Check for missing shared libraries
    if command -v ldd &>/dev/null; then
        local missing_libs
        missing_libs=$(ldd "$bin_path" 2>&1 | grep "not found" || true)
        if [[ -n "$missing_libs" ]]; then
            print_error "Missing shared libraries detected:"
            echo "$missing_libs" | while read -r line; do
                print_error "  $line"
            done
            print_error "Install the missing libraries before continuing."
            exit 1
        fi
    fi
}

# Install FFmpeg binary (if bundled)
install_ffmpeg() {
    local ffmpeg_path="$1"
    
    if [[ -f "$ffmpeg_path" ]]; then
        print_status "Installing bundled FFmpeg..."
        mkdir -p "$INSTALL_DIR/bin"
        cp "$ffmpeg_path" "$INSTALL_DIR/bin/ffmpeg"
        chmod +x "$INSTALL_DIR/bin/ffmpeg"
        print_status "Bundled FFmpeg installed to $INSTALL_DIR/bin/ffmpeg"
    fi
}

# Install ImageMagick binary (if bundled)
install_magick() {
    local magick_path="$1"
    
    if [[ -f "$magick_path" ]]; then
        print_status "Installing bundled ImageMagick..."
        mkdir -p "$INSTALL_DIR/bin"
        cp "$magick_path" "$INSTALL_DIR/bin/magick"
        chmod +x "$INSTALL_DIR/bin/magick"
        print_status "Bundled ImageMagick installed to $INSTALL_DIR/bin/magick"
    fi
}

# ============================================================
# Interactive Configuration Wizard
# ============================================================

# Helper: prompt with default value
# Usage: result=$(prompt_with_default "Question" "default")
prompt_with_default() {
    local prompt_text="$1"
    local default_val="$2"
    local result=""
    
    if $AUTO_YES; then
        echo "$default_val"
        return
    fi
    
    if [[ -n "$default_val" ]]; then
        read -r -p "  $prompt_text [$default_val]: " result
        echo "${result:-$default_val}"
    else
        read -r -p "  $prompt_text: " result
        echo "$result"
    fi
}

# Helper: yes/no prompt (default yes)
prompt_yes_no() {
    local prompt_text="$1"
    local default="${2:-y}"  # y or n
    
    if $AUTO_YES; then
        [[ "$default" == "y" ]] && return 0 || return 1
    fi
    
    local hint="[Y/n]"
    if [[ "$default" == "n" ]]; then
        hint="[y/N]"
    fi
    
    read -r -p "  $prompt_text $hint " -n 1 reply
    echo
    
    if [[ "$default" == "y" ]]; then
        [[ "$reply" =~ ^[Nn]$ ]] && return 1 || return 0
    else
        [[ "$reply" =~ ^[Yy]$ ]] && return 0 || return 1
    fi
}

# Generate a secure random token
# Uses alphanumeric + safe special chars, suitable for HTTP auth headers
generate_token() {
    local length="${1:-64}"
    tr -dc 'A-Za-z0-9_+~!*.-' < /dev/urandom | head -c "$length"
}

# --- Section: Database name ---
wizard_database() {
    echo
    echo -e "${BLUE}  ┌─ Database Name ─────────────────────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} The MongoDB database name for this service.      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Each service instance should use a unique name.  ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    if $IS_RECONFIGURE && [[ -n "$CFG_DB_NAME" ]]; then
        echo -e "  Current database: ${YELLOW}${CFG_DB_NAME}${NC}"
        if prompt_yes_no "  Keep current database name?" "y"; then
            return
        fi
    fi
    
    CFG_DB_NAME=$(prompt_with_default "Database name" "${CFG_DB_NAME:-DayZ}")
    # Also update MONGO_DB so MongoDB security creates the right user
    MONGO_DB="$CFG_DB_NAME"
}

# --- Section: Port ---
wizard_port() {
    echo
    echo -e "${BLUE}  ┌─ Service Port ──────────────────────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} Port 443 is the standard HTTPS port.             ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} The systemd service has permission to bind to    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} port 443 without running as root.                ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Use 8443 if another service uses 443.            ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    if $IS_RECONFIGURE && [[ -n "$CFG_PORT" ]]; then
        echo -e "  Current port: ${YELLOW}${CFG_PORT}${NC}"
        if prompt_yes_no "  Keep current port?" "y"; then
            return
        fi
    fi
    
    CFG_PORT=$(prompt_with_default "Port" "${CFG_PORT:-443}")
}

# --- Section: Server auth keys ---
wizard_server_auth() {
    echo
    echo -e "${BLUE}  ┌─ DayZ Server Authentication ────────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} Each DayZ game server that connects to this      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} service needs its own unique auth token.         ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} The token goes into each server's mod config:    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}   ${YELLOW}\$profile:UF/UFramework.json → \"ServerAuth\"${NC}      ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    # ── Reconfigure mode: show existing tokens and offer to keep ──
    if $IS_RECONFIGURE && [[ ${#CFG_SERVER_AUTH_TOKENS[@]} -gt 0 ]]; then
        echo
        echo -e "  ${BLUE}Current server auth tokens:${NC}"
        for i in "${!CFG_SERVER_AUTH_TOKENS[@]}"; do
            local label="${CFG_SERVER_AUTH_LABELS[$i]:-Server $((i+1))}"
            echo -e "    ${GREEN}${label}${NC}: ${YELLOW}${CFG_SERVER_AUTH_TOKENS[$i]}${NC}"
        done
        echo
        
        if prompt_yes_no "  Keep existing auth tokens?" "y"; then
            # Ask if they want to add more
            if prompt_yes_no "  Add additional server auth tokens?" "n"; then
                local add_count_input
                add_count_input=$(prompt_with_default "  How many tokens to add?" "1")
                local add_count=1
                if [[ "$add_count_input" =~ ^[0-9]+$ ]] && [[ "$add_count_input" -ge 1 ]] && [[ "$add_count_input" -le 50 ]]; then
                    add_count="$add_count_input"
                fi
                
                local existing_count=${#CFG_SERVER_AUTH_TOKENS[@]}
                for i in $(seq 1 "$add_count"); do
                    local idx=$((existing_count + i))
                    local token
                    token=$(generate_token 64)
                    local label
                    label=$(prompt_with_default "  Label for new server $idx" "DayZ Server $idx")
                    CFG_SERVER_AUTH_TOKENS+=("$token")
                    CFG_SERVER_AUTH_LABELS+=("$label")
                done
                CFG_SERVER_COUNT=${#CFG_SERVER_AUTH_TOKENS[@]}
                print_status "Added $add_count new token(s). Total: $CFG_SERVER_COUNT"
            fi
            return
        fi
        echo
        print_status "Replacing all existing tokens with new ones."
    fi
    
    # ── Fresh generation ──
    if $AUTO_YES; then
        CFG_SERVER_COUNT=1
    else
        local count_input
        count_input=$(prompt_with_default "How many DayZ servers will connect to this service?" "${CFG_SERVER_COUNT:-1}")
        # Validate it's a number between 1-50
        if [[ "$count_input" =~ ^[0-9]+$ ]] && [[ "$count_input" -ge 1 ]] && [[ "$count_input" -le 50 ]]; then
            CFG_SERVER_COUNT="$count_input"
        else
            print_warning "Invalid number. Defaulting to 1."
            CFG_SERVER_COUNT=1
        fi
    fi
    
    CFG_SERVER_AUTH_TOKENS=()
    CFG_SERVER_AUTH_LABELS=()
    
    for i in $(seq 1 "$CFG_SERVER_COUNT"); do
        local token
        token=$(generate_token 64)
        local default_label="DayZ Server $i"
        
        if $AUTO_YES; then
            CFG_SERVER_AUTH_LABELS+=("$default_label")
        else
            local label
            label=$(prompt_with_default "  Label for server $i" "$default_label")
            CFG_SERVER_AUTH_LABELS+=("$label")
        fi
        
        CFG_SERVER_AUTH_TOKENS+=("$token")
    done
    
    echo
    print_status "Generated $CFG_SERVER_COUNT auth token(s)."
}

# --- Section: SSL/Connection mode ---
wizard_ssl_mode() {
    echo
    echo -e "${BLUE}  ┌─ Connection Security ───────────────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} How should DayZ servers reach this service?      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${GREEN}1)${NC} Let's Encrypt  — Free auto-renewing HTTPS    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    Requires a domain pointing    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    to this server + port 80/443  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${GREEN}2)${NC} Self-Signed    — Provide your own cert files  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    or generate a self-signed one ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${GREEN}3)${NC} DaemonForge Proxy — Free subdomain via        ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    ufapi.daemonforge.dev         ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    No domain purchase needed     ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${GREEN}4)${NC} Cloudflare Tunnel — Route through Cloudflare  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    Requires Cloudflare account   ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                    & tunnel token                ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${GREEN}5)${NC} None / Skip    — Configure later manually     ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    if $AUTO_YES; then
        CFG_SSL_MODE="none"
        print_status "Skipping SSL configuration (unattended mode). Configure later."
        return
    fi
    
    # Show current setting and offer to keep during reconfigure
    if $IS_RECONFIGURE && [[ -n "$CFG_SSL_MODE" ]] && [[ "$CFG_SSL_MODE" != "none" ]]; then
        local mode_display="$CFG_SSL_MODE"
        case "$CFG_SSL_MODE" in
            letsencrypt) mode_display="Let's Encrypt ($CFG_LE_DOMAIN)" ;;
            selfsigned)  mode_display="Self-Signed Certificate" ;;
            proxy)       mode_display="DaemonForge Proxy (${CFG_PROXY_SUBDOMAIN}.${CFG_PROXY_PRIMARY_DOMAIN})" ;;
            tunnel)      mode_display="Cloudflare Tunnel" ;;
        esac
        echo -e "  Current mode: ${YELLOW}${mode_display}${NC}"
        if prompt_yes_no "  Keep current connection mode?" "y"; then
            return
        fi
    fi
    
    local choice
    read -r -p "  Choose connection mode [1-5]: " choice
    
    case "$choice" in
        1) CFG_SSL_MODE="letsencrypt" ;;
        2) CFG_SSL_MODE="selfsigned"  ;;
        3) CFG_SSL_MODE="proxy"       ;;
        4) CFG_SSL_MODE="tunnel"      ;;
        *) CFG_SSL_MODE="none"        ;;
    esac
}

# --- Section: Let's Encrypt config ---
wizard_letsencrypt() {
    echo
    echo -e "${BLUE}  ┌─ Let's Encrypt Configuration ──────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} You need a domain name pointing to this server   ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} (A record or CNAME in DNS).                      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Ports 80 and 443 must be open and reachable      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} from the internet for certificate validation.    ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    CFG_LE_DOMAIN=$(prompt_with_default "Primary domain (e.g., uf.example.com)" "${CFG_LE_DOMAIN:-}")
    
    if [[ -z "$CFG_LE_DOMAIN" ]]; then
        print_error "Domain is required for Let's Encrypt. Falling back to self-signed."
        CFG_SSL_MODE="none"
        return
    fi
    
    CFG_LE_EMAIL=$(prompt_with_default "Email for certificate notifications" "${CFG_LE_EMAIL:-}")
    
    if [[ -z "$CFG_LE_EMAIL" ]]; then
        print_error "Email is required for Let's Encrypt. Falling back to self-signed."
        CFG_SSL_MODE="none"
        return
    fi
    
    local altnames_default="n"
    if [[ -n "$CFG_LE_ALTNAMES" ]]; then
        altnames_default="y"
    fi
    if prompt_yes_no "Add alternate domain names (SANs)?" "$altnames_default"; then
        CFG_LE_ALTNAMES=$(prompt_with_default "Comma-separated alt names (e.g., api.example.com,uf2.example.com)" "${CFG_LE_ALTNAMES:-}")
    fi
    
    # Force port 443 for Let's Encrypt
    CFG_PORT="443"
    
    echo
    print_status "Let's Encrypt will be enabled for ${CFG_LE_DOMAIN}"
    print_status "Certificates will auto-renew via Greenlock."
    print_warning "Ensure DNS points to this server and ports 80+443 are open!"
}

# --- Section: Self-signed cert config ---
wizard_selfsigned() {
    echo
    echo -e "${BLUE}  ┌─ Self-Signed / Custom Certificate ─────────────┐${NC}"
    echo -e "${BLUE}  │${NC} Provide paths to an existing certificate, or    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} the installer can generate a self-signed one.   ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    if prompt_yes_no "Generate a new self-signed certificate?" "y"; then
        # Generate self-signed cert using openssl
        if ! command -v openssl &>/dev/null; then
            print_error "OpenSSL not found. Cannot generate certificate."
            print_warning "Install openssl and re-run, or provide existing cert paths."
            CFG_SSL_MODE="none"
            return
        fi
        
        local cert_dir="$CONFIG_DIR/certs"
        mkdir -p "$cert_dir"
        
        local cert_cn
        cert_cn=$(prompt_with_default "Certificate Common Name (hostname or IP)" "$(hostname -f 2>/dev/null || echo 'localhost')")
        
        print_status "Generating self-signed certificate..."
        openssl req -x509 -newkey rsa:4096 -keyout "$cert_dir/server.key" \
            -out "$cert_dir/server.crt" -days 365 -nodes \
            -subj "/CN=${cert_cn}" \
            -addext "subjectAltName=DNS:${cert_cn},DNS:localhost,IP:127.0.0.1" \
            2>/dev/null || {
                print_error "Failed to generate certificate."
                CFG_SSL_MODE="none"
                return
            }
        
        # Set proper permissions
        chmod 640 "$cert_dir/server.key" "$cert_dir/server.crt"
        chown "$SERVICE_USER:$SERVICE_USER" "$cert_dir/server.key" "$cert_dir/server.crt" 2>/dev/null || true
        
        CFG_CERT_PATH="$cert_dir/server.crt"
        CFG_CERT_KEY_PATH="$cert_dir/server.key"
        
        print_status "Certificate generated:"
        echo "    Cert: $CFG_CERT_PATH"
        echo "    Key:  $CFG_CERT_KEY_PATH"
        echo "    Valid for 365 days. CN=$cert_cn"
        print_warning "Self-signed certs will show browser warnings. DayZ mods accept them fine."
    else
        CFG_CERT_PATH=$(prompt_with_default "Path to certificate file (.crt / .pem)" "")
        CFG_CERT_KEY_PATH=$(prompt_with_default "Path to private key file (.key)" "")
        
        if [[ ! -f "$CFG_CERT_PATH" ]]; then
            print_warning "Certificate file not found at: $CFG_CERT_PATH"
            print_warning "You'll need to fix this in config.json before starting."
        fi
        if [[ ! -f "$CFG_CERT_KEY_PATH" ]]; then
            print_warning "Key file not found at: $CFG_CERT_KEY_PATH"
        fi
    fi
}

# --- Section: DaemonForge Proxy config ---
wizard_proxy() {
    echo
    echo -e "${BLUE}  ┌─ DaemonForge Proxy ─────────────────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} This gives you a free HTTPS subdomain like:      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}   ${GREEN}yourname.ufapi.daemonforge.dev${NC}                 ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                   ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} No domain purchase needed. Traffic is routed      ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} through the DaemonForge proxy service.             ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Port forwarding is still required.                 ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                   ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} The proxy token auto-renews every 24 hours.       ${BLUE}│${NC}"
    echo -e "${BLUE}  └───────────────────────────────────────────────────┘${NC}"
    
    # Fetch available domains
    print_status "Fetching available proxy domains..."
    local domains_json=""
    
    if command -v curl &>/dev/null; then
        domains_json=$(curl -s --max-time 10 "https://ufapi.daemonforge.dev/available" 2>/dev/null) || true
    fi
    
    if [[ -z "$domains_json" ]] || [[ "$domains_json" == "null" ]]; then
        print_warning "Could not reach DaemonForge proxy service."
        print_warning "You can configure proxy settings later in config.json."
        print_warning "  Proxy.primaryDomain, Proxy.subdomain, Proxy.token"
        CFG_SSL_MODE="none"
        return
    fi
    
    # Parse domain list (expecting JSON array like ["domain1.dev","domain2.dev"])
    local domains=()
    # Simple parsing: remove brackets and quotes, split by comma
    local cleaned
    cleaned=$(echo "$domains_json" | tr -d '[]"' | tr ',' '\n')
    
    local i=1
    echo
    echo -e "  ${BLUE}Available proxy domains:${NC}"
    while IFS= read -r domain; do
        domain=$(echo "$domain" | tr -d ' ')
        [[ -z "$domain" ]] && continue
        domains+=("$domain")
        echo "    $i) $domain"
        ((i++))
    done <<< "$cleaned"
    
    if [[ ${#domains[@]} -eq 0 ]]; then
        print_warning "No proxy domains available. Configure manually later."
        CFG_SSL_MODE="none"
        return
    fi
    
    echo
    local domain_choice
    read -r -p "  Pick a domain [1-${#domains[@]}]: " domain_choice
    
    if [[ "$domain_choice" =~ ^[0-9]+$ ]] && [[ "$domain_choice" -ge 1 ]] && [[ "$domain_choice" -le ${#domains[@]} ]]; then
        CFG_PROXY_PRIMARY_DOMAIN="${domains[$((domain_choice-1))]}"
    else
        CFG_PROXY_PRIMARY_DOMAIN="${domains[0]}"
    fi
    
    print_status "Selected domain: $CFG_PROXY_PRIMARY_DOMAIN"
    
    # Register subdomain
    local subdomain_input
    subdomain_input=$(prompt_with_default "Choose a subdomain name (yourname.${CFG_PROXY_PRIMARY_DOMAIN})" "")
    
    if [[ -z "$subdomain_input" ]]; then
        print_warning "No subdomain provided. You can register one later via config."
        CFG_SSL_MODE="none"
        return
    fi
    
    print_status "Registering ${subdomain_input}.${CFG_PROXY_PRIMARY_DOMAIN}..."
    
    local register_response=""
    register_response=$(curl -s --max-time 15 -X POST "https://ufapi.daemonforge.dev/register" \
        -H "Content-Type: application/json" \
        -d "{\"domain\": \"${CFG_PROXY_PRIMARY_DOMAIN}\", \"subdomain\": \"${subdomain_input}\"}" 2>/dev/null) || true
    
    if [[ -z "$register_response" ]]; then
        print_error "Registration failed — no response from server."
        print_warning "You can register manually later via the DaemonForge API."
        CFG_SSL_MODE="none"
        return
    fi
    
    # Parse response (expecting { "subdomain": "...", "token": "..." })
    local reg_subdomain=""
    local reg_token=""
    
    if command -v python3 &>/dev/null; then
        reg_subdomain=$(python3 -c "import json,sys; d=json.loads(sys.stdin.read()); print(d.get('subdomain',''))" <<< "$register_response" 2>/dev/null)
        reg_token=$(python3 -c "import json,sys; d=json.loads(sys.stdin.read()); print(d.get('token',''))" <<< "$register_response" 2>/dev/null)
    else
        # Fallback: crude grep
        reg_subdomain=$(echo "$register_response" | grep -o '"subdomain"[[:space:]]*:[[:space:]]*"[^"]*"' | head -1 | sed 's/.*"subdomain"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
        reg_token=$(echo "$register_response" | grep -o '"token"[[:space:]]*:[[:space:]]*"[^"]*"' | head -1 | sed 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')
    fi
    
    if [[ -n "$reg_subdomain" && -n "$reg_token" ]]; then
        CFG_PROXY_SUBDOMAIN="$reg_subdomain"
        CFG_PROXY_TOKEN="$reg_token"
        
        echo
        echo -e "  ${GREEN}✓ Proxy registered successfully!${NC}"
        echo -e "    URL:   ${BLUE}https://${reg_subdomain}.${CFG_PROXY_PRIMARY_DOMAIN}${NC}"
        echo -e "    Token: ${BLUE}${reg_token}${NC}"
    else
        print_error "Failed to parse registration response:"
        echo "    $register_response"
        print_warning "You can register manually later."
        CFG_SSL_MODE="none"
    fi
}

# --- Section: Cloudflare Tunnel config ---
wizard_tunnel() {
    echo
    echo -e "${BLUE}  ┌─ Cloudflare Tunnel ─────────────────────────────┐${NC}"
    echo -e "${BLUE}  │${NC} Routes traffic through Cloudflare's network.     ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Requirements:                                    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  • Cloudflare account (free tier works)           ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  • A tunnel token from the Cloudflare dashboard  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Get your token at:                               ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${YELLOW}https://one.dash.cloudflare.com → Networks →${NC}    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  ${YELLOW}Tunnels → Create a tunnel → get token${NC}           ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} The service auto-downloads cloudflared and       ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} starts/updates the tunnel automatically.         ${BLUE}│${NC}"
    echo -e "${BLUE}  └──────────────────────────────────────────────────┘${NC}"
    
    if $IS_RECONFIGURE && [[ -n "$CFG_TUNNEL_TOKEN" ]]; then
        echo -e "  Current tunnel token: ${YELLOW}${CFG_TUNNEL_TOKEN}${NC}"
        if prompt_yes_no "  Keep current tunnel token?" "y"; then
            return
        fi
    fi
    
    CFG_TUNNEL_TOKEN=$(prompt_with_default "Cloudflare tunnel token" "${CFG_TUNNEL_TOKEN:-}")
    
    if [[ -z "$CFG_TUNNEL_TOKEN" ]]; then
        print_warning "No tunnel token provided. You can add it to config.json later."
        print_warning "  Set Tunnel.enabled=true, Tunnel.token=\"your-token\""
        CFG_SSL_MODE="none"
        return
    fi
    
    print_status "Tunnel will auto-start on service launch."
    print_status "cloudflared binary will be downloaded automatically on first run."
}

# --- Section: Discord bot (optional) ---
wizard_discord() {
    echo
    echo -e "${BLUE}  ┌─ Discord Integration (Optional) ───────────────┐${NC}"
    echo -e "${BLUE}  │${NC} Connect your DayZ server to Discord for:        ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  • Role management & player verification        ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  • DM and channel messaging from in-game        ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}  • Voice channel integration                    ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                 ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Create a bot at: ${YELLOW}https://discord.com/developers${NC}  ${BLUE}│${NC}"
    echo -e "${BLUE}  └─────────────────────────────────────────────────┘${NC}"
    
    # During reconfigure with existing Discord config, default to "configure" (keep)
    local discord_default="n"
    if $IS_RECONFIGURE && [[ -n "$CFG_DISCORD_BOT_TOKEN" ]]; then
        discord_default="y"
    fi
    
    if ! prompt_yes_no "Configure Discord bot now?" "$discord_default"; then
        # In reconfigure mode, keep existing values when skipping
        if $IS_RECONFIGURE && [[ -n "$CFG_DISCORD_BOT_TOKEN" ]]; then
            echo -e "  Current bot token: ${YELLOW}${CFG_DISCORD_BOT_TOKEN}${NC}"
            if prompt_yes_no "  Keep current Discord settings?" "y"; then
                return
            fi
        fi
        print_status "Skipping Discord. You can add it to config.json later."
        CFG_DISCORD_BOT_TOKEN=""
        CFG_DISCORD_GUILD_ID=""
        CFG_DISCORD_CLIENT_ID=""
        CFG_DISCORD_CLIENT_SECRET=""
        return
    fi
    
    CFG_DISCORD_BOT_TOKEN=$(prompt_with_default "Discord Bot Token" "${CFG_DISCORD_BOT_TOKEN:-}")
    if [[ -z "$CFG_DISCORD_BOT_TOKEN" ]]; then
        print_warning "No bot token provided, skipping Discord setup."
        return
    fi
    
    CFG_DISCORD_GUILD_ID=$(prompt_with_default "Discord Server (Guild) ID" "${CFG_DISCORD_GUILD_ID:-}")
    CFG_DISCORD_CLIENT_ID=$(prompt_with_default "Discord Application Client ID" "${CFG_DISCORD_CLIENT_ID:-}")
    CFG_DISCORD_CLIENT_SECRET=$(prompt_with_default "Discord Application Client Secret" "${CFG_DISCORD_CLIENT_SECRET:-}")
    
    echo
    print_status "Discord bot configured."
    if [[ -z "$CFG_DISCORD_GUILD_ID" ]]; then
        print_warning "Guild ID is empty — some features won't work until you set it."
    fi
}

# --- Section: OpenAI (optional) ---
wizard_openai() {
    echo
    echo -e "${BLUE}  ┌─ OpenAI / AI Features (Optional) ──────────────┐${NC}"
    echo -e "${BLUE}  │${NC} Powers AI chat agents and NPC conversations.     ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Requires an OpenAI API key with billing enabled. ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC}                                                  ${BLUE}│${NC}"
    echo -e "${BLUE}  │${NC} Get your key at: ${YELLOW}https://platform.openai.com${NC}     ${BLUE}│${NC}"
    echo -e "${BLUE}  └─────────────────────────────────────────────────┘${NC}"
    
    # During reconfigure with existing OpenAI config, default to "configure" (keep)
    local openai_default="n"
    if $IS_RECONFIGURE && [[ -n "$CFG_OPENAI_KEY" ]]; then
        openai_default="y"
    fi
    
    if ! prompt_yes_no "Configure OpenAI API key now?" "$openai_default"; then
        # In reconfigure mode, keep existing key when skipping
        if $IS_RECONFIGURE && [[ -n "$CFG_OPENAI_KEY" ]]; then
            echo -e "  Current API key: ${YELLOW}${CFG_OPENAI_KEY}${NC}"
            if prompt_yes_no "  Keep current OpenAI key?" "y"; then
                return
            fi
        fi
        print_status "Skipping OpenAI. You can add it to config.json later."
        CFG_OPENAI_KEY=""
        return
    fi
    
    CFG_OPENAI_KEY=$(prompt_with_default "OpenAI API Key (sk-...)" "${CFG_OPENAI_KEY:-}")
    
    if [[ -n "$CFG_OPENAI_KEY" ]]; then
        print_status "OpenAI API key configured."
    fi
}

# --- Run the full wizard ---
configure_wizard() {
    echo
    echo -e "${GREEN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║         Configuration Wizard                     ║${NC}"
    echo -e "${GREEN}╠══════════════════════════════════════════════════╣${NC}"
    echo -e "${GREEN}║ We'll walk you through the essential settings.   ║${NC}"
    echo -e "${GREEN}║ Press Enter to accept [defaults] shown in        ║${NC}"
    echo -e "${GREEN}║ brackets. You can change everything later in:    ║${NC}"
    echo -e "${GREEN}║   ${YELLOW}$CONFIG_DIR/config.json${NC}${GREEN}       ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════╝${NC}"
    
    wizard_database
    wizard_port
    wizard_server_auth
    wizard_ssl_mode
    
    case "$CFG_SSL_MODE" in
        letsencrypt) wizard_letsencrypt ;;
        selfsigned)  wizard_selfsigned  ;;
        proxy)       wizard_proxy       ;;
        tunnel)      wizard_tunnel      ;;
        *)           print_status "No SSL/connection mode selected. Using built-in self-signed cert." ;;
    esac
    
    wizard_discord
    wizard_openai
}

# ============================================================
# Load MongoDB credentials from the backup file if globals empty
# ============================================================
load_mongodb_credentials() {
    local cred_file="$CONFIG_DIR/.mongodb_credentials"
    
    # Already loaded from globals
    if [[ -n "$MONGO_AUTH_URI" ]]; then
        return
    fi
    
    if [[ -f "$cred_file" ]]; then
        print_status "Loading MongoDB credentials from $cred_file"
        # Source the file — it contains KEY=VALUE lines
        while IFS='=' read -r key value; do
            # Skip comments and empty lines
            [[ "$key" =~ ^#.*$ ]] && continue || true
            [[ -z "$key" ]] && continue
            key=$(echo "$key" | xargs)
            case "$key" in
                MONGO_USER)     MONGO_USER="$value" ;;
                MONGO_PASS)     MONGO_PASS="$value" ;;
                MONGO_DB)       MONGO_DB="$value" ;;
                MONGO_AUTH_URI) MONGO_AUTH_URI="$value" ;;
            esac
        done < "$cred_file"
        
        if [[ -n "$MONGO_AUTH_URI" ]]; then
            print_status "Restored MongoDB URI from credential backup"
        fi
    fi
}

# ============================================================
# Re-run MongoDB security setup independently
# ============================================================
resecure_mongodb() {
    echo
    echo -e "${CYAN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║         MongoDB Security Setup                   ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════╝${NC}"
    echo
    
    if ! systemctl is-active --quiet mongod 2>/dev/null; then
        print_error "MongoDB is not running."
        echo "  Start it first:  sudo systemctl start mongod"
        exit 1
    fi
    
    # Check if auth is already enabled
    if grep -qE '^\s*authorization:\s*enabled' /etc/mongod.conf 2>/dev/null; then
        print_warning "MongoDB authentication is already enabled."
        echo
        echo "  Options:"
        echo "    1) Create a NEW user with a new password (drops existing ufservice user)"
        echo "    2) Cancel"
        echo
        local choice
        read -r -p "  Choose [1-2]: " choice
        if [[ "$choice" != "1" ]]; then
            print_status "Cancelled. No changes made."
            
            # Check if credential file exists
            local cred_file="$CONFIG_DIR/.mongodb_credentials"
            if [[ -f "$cred_file" ]]; then
                echo
                print_status "Your existing credentials are saved in:"
                echo "  $cred_file"
                echo
                grep -v '^#' "$cred_file" || true
            fi
            exit 0
        fi
        
        echo
        print_warning "This will drop the existing 'ufservice' user and create a new one."
        print_warning "You'll need to update config.json with the new connection string."
        echo
        
        # Temporarily disable auth to recreate user
        print_status "Temporarily disabling auth to recreate user..."
        sed -i 's/^\(\s*authorization:\).*/\1 disabled/' /etc/mongod.conf 2>/dev/null || true

        # Trap to re-enable auth if interrupted during this vulnerable window
        trap 'sed -i "s/^\(\s*authorization:\).*/\1 enabled/" /etc/mongod.conf 2>/dev/null; systemctl restart mongod 2>/dev/null; print_error "Interrupted — MongoDB auth re-enabled"; exit 1' INT TERM

        systemctl restart mongod || {
            print_error "Failed to restart MongoDB after disabling auth. Re-enabling..."
            sed -i 's/^\(\s*authorization:\).*/\1 enabled/' /etc/mongod.conf 2>/dev/null || true
            systemctl restart mongod 2>/dev/null || true
            trap - INT TERM
            return 1
        }
        sleep 2
    fi
    
    # Ask for database name
    local db_name
    db_name=$(prompt_with_default "MongoDB database name" "${MONGO_DB:-DayZ}")
    
    # Run the security setup
    setup_mongodb_security "$db_name"

    # Clear the interrupt trap now that auth is re-enabled
    trap - INT TERM
    
    # Update config.json if it exists
    local config_file="$CONFIG_DIR/config.json"
    if [[ -f "$config_file" ]] && [[ -n "$MONGO_AUTH_URI" ]]; then
        echo
        if prompt_yes_no "Update config.json with new MongoDB credentials?" "y"; then
            if command -v python3 &>/dev/null; then
                UF_CONFIG_FILE="$config_file" UF_MONGO_URI="$MONGO_AUTH_URI" UF_MONGO_DB="$MONGO_DB" python3 -c '
import json, os
cfg = os.environ["UF_CONFIG_FILE"]
with open(cfg, "r") as f:
    config = json.load(f)
config["DBServer"] = os.environ["UF_MONGO_URI"]
config["DB"] = os.environ["UF_MONGO_DB"]
with open(cfg, "w") as f:
    json.dump(config, f, indent=4)
print("Updated successfully")
' 2>/dev/null && print_status "Config updated with new MongoDB credentials." || {
                    print_warning "Could not auto-update config. Update manually:"
                    echo "  DBServer: $MONGO_AUTH_URI"
                }
            else
                sed -i "s|\"DBServer\":.*|\"DBServer\": \"${MONGO_AUTH_URI}\",|" "$config_file" 2>/dev/null || true
                sed -i "s|\"DB\":.*|\"DB\": \"${MONGO_DB}\",|" "$config_file" 2>/dev/null || true
                print_status "Config updated."
            fi
            
            # Offer restart
            if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
                if prompt_yes_no "Restart service to apply changes?" "y"; then
                    systemctl restart "$SERVICE_NAME"
                    sleep 2
                    if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
                        print_status "Service restarted successfully!"
                    else
                        print_warning "Service failed to start. Check: journalctl -u $SERVICE_NAME -n 30"
                    fi
                fi
            fi
        fi
    fi
    
    # Ensure symlink exists so the binary finds updated config
    ln -sfn "$CONFIG_DIR/config.json" "$DATA_DIR/config.json" 2>/dev/null || true
    
    echo
    print_status "MongoDB security setup complete."
    print_status "Credential backup: $CONFIG_DIR/.mongodb_credentials"
}

# ============================================================
# Load all values from an existing config.json into globals
# ============================================================
load_existing_config() {
    local config_file="$CONFIG_DIR/config.json"
    [[ ! -f "$config_file" ]] && return
    
    if ! command -v python3 &>/dev/null; then
        print_warning "python3 not found — cannot read existing config values."
        return
    fi
    
    # Read all values in a single python3 invocation to avoid many subshells
    local json_dump
    json_dump=$(UF_CONFIG_FILE="$config_file" python3 -c "
import json, sys, os
try:
    with open(os.environ['UF_CONFIG_FILE'], 'r') as f:
        c = json.load(f)
    
    def g(key, default=''):
        v = c.get(key, default)
        return str(v) if v else ''
    
    def nested(keys, default=''):
        d = c
        for k in keys:
            if isinstance(d, dict):
                d = d.get(k, {})
            else:
                return default
        return str(d) if d else default
    
    # Output key=value lines for bash to eval
    print('CFG_DB_NAME=' + g('DB', 'DayZ'))
    print('CFG_PORT=' + g('Port', '443'))
    print('MONGO_AUTH_URI=' + g('DBServer', ''))
    print('MONGO_DB=' + g('DB', 'DayZ'))
    
    # ServerAuth tokens and labels
    tokens = c.get('ServerAuth', [])
    labels = c.get('ServerAuthLabels', [])
    if isinstance(tokens, list):
        for i, t in enumerate(tokens):
            print('AUTH_TOKEN_' + str(i) + '=' + str(t))
        print('AUTH_TOKEN_COUNT=' + str(len(tokens)))
    if isinstance(labels, list):
        for i, l in enumerate(labels):
            print('AUTH_LABEL_' + str(i) + '=' + str(l))
    
    # SSL mode detection
    le = c.get('LetsEncypt', {}) or {}
    proxy = c.get('Proxy', {}) or {}
    tunnel = c.get('Tunnel', {}) or {}
    cert = g('Certificate', '')
    
    if le.get('Enabled'):
        print('CFG_SSL_MODE=letsencrypt')
        print('CFG_LE_DOMAIN=' + str(le.get('Domain', '')))
        print('CFG_LE_EMAIL=' + str(le.get('Email', '')))
        altnames = le.get('AltNames', [])
        if isinstance(altnames, list):
            print('CFG_LE_ALTNAMES=' + ','.join(str(a) for a in altnames))
    elif tunnel.get('enabled') and tunnel.get('token'):
        print('CFG_SSL_MODE=tunnel')
        print('CFG_TUNNEL_TOKEN=' + str(tunnel.get('token', '')))
    elif proxy.get('subdomain') and proxy.get('token'):
        print('CFG_SSL_MODE=proxy')
        print('CFG_PROXY_SUBDOMAIN=' + str(proxy.get('subdomain', '')))
        print('CFG_PROXY_TOKEN=' + str(proxy.get('token', '')))
        print('CFG_PROXY_PRIMARY_DOMAIN=' + str(proxy.get('primaryDomain', '')))
    elif cert:
        print('CFG_SSL_MODE=selfsigned')
        print('CFG_CERT_PATH=' + cert)
        print('CFG_CERT_KEY_PATH=' + g('CertificateKey', ''))
    else:
        print('CFG_SSL_MODE=none')
    
    # Discord
    d = c.get('Discord', {}) or {}
    print('CFG_DISCORD_BOT_TOKEN=' + str(d.get('Bot_Token', '')))
    print('CFG_DISCORD_CLIENT_ID=' + str(d.get('Client_Id', '')))
    print('CFG_DISCORD_CLIENT_SECRET=' + str(d.get('Client_Secret', '')))
    print('CFG_DISCORD_GUILD_ID=' + str(d.get('Guild_Id', '')))
    
    # OpenAI
    ai = c.get('OpenAIApi', {}) or {}
    print('CFG_OPENAI_KEY=' + str(ai.get('ApiKey', '')))
except Exception as e:
    print('# ERROR: ' + str(e), file=sys.stderr)
" 2>/dev/null) || return
    
    # Parse the output into bash variables
    CFG_SERVER_AUTH_TOKENS=()
    CFG_SERVER_AUTH_LABELS=()
    local auth_count=0
    
    while IFS='=' read -r key value; do
        [[ -z "$key" ]] && continue
        case "$key" in
            CFG_DB_NAME)              CFG_DB_NAME="$value" ;;
            CFG_PORT)                 CFG_PORT="$value" ;;
            MONGO_AUTH_URI)           MONGO_AUTH_URI="$value" ;;
            MONGO_DB)                 MONGO_DB="$value" ;;
            AUTH_TOKEN_COUNT)         auth_count="$value" ;;
            AUTH_TOKEN_*)             CFG_SERVER_AUTH_TOKENS+=("$value") ;;
            AUTH_LABEL_*)             CFG_SERVER_AUTH_LABELS+=("$value") ;;
            CFG_SSL_MODE)             CFG_SSL_MODE="$value" ;;
            CFG_LE_DOMAIN)            CFG_LE_DOMAIN="$value" ;;
            CFG_LE_EMAIL)             CFG_LE_EMAIL="$value" ;;
            CFG_LE_ALTNAMES)          CFG_LE_ALTNAMES="$value" ;;
            CFG_CERT_PATH)            CFG_CERT_PATH="$value" ;;
            CFG_CERT_KEY_PATH)        CFG_CERT_KEY_PATH="$value" ;;
            CFG_PROXY_SUBDOMAIN)      CFG_PROXY_SUBDOMAIN="$value" ;;
            CFG_PROXY_TOKEN)          CFG_PROXY_TOKEN="$value" ;;
            CFG_PROXY_PRIMARY_DOMAIN) CFG_PROXY_PRIMARY_DOMAIN="$value" ;;
            CFG_TUNNEL_TOKEN)         CFG_TUNNEL_TOKEN="$value" ;;
            CFG_DISCORD_BOT_TOKEN)    CFG_DISCORD_BOT_TOKEN="$value" ;;
            CFG_DISCORD_CLIENT_ID)    CFG_DISCORD_CLIENT_ID="$value" ;;
            CFG_DISCORD_CLIENT_SECRET) CFG_DISCORD_CLIENT_SECRET="$value" ;;
            CFG_DISCORD_GUILD_ID)     CFG_DISCORD_GUILD_ID="$value" ;;
            CFG_OPENAI_KEY)           CFG_OPENAI_KEY="$value" ;;
        esac
    done <<< "$json_dump"
    
    CFG_SERVER_COUNT="${#CFG_SERVER_AUTH_TOKENS[@]}"
    if [[ "$CFG_SERVER_COUNT" -lt 1 ]]; then
        CFG_SERVER_COUNT=1
    fi
    
    print_status "Loaded ${CFG_SERVER_COUNT} server token(s), SSL mode: ${CFG_SSL_MODE:-none}"
}

# ============================================================
# Re-run configuration wizard on an existing installation
# ============================================================
reconfigure() {
    local config_file="$CONFIG_DIR/config.json"
    
    # Verify the service is installed
    if [[ ! -d "$INSTALL_DIR" ]] || [[ ! -f "$INSTALL_DIR/$BINARY_NAME" ]]; then
        print_error "Service does not appear to be installed at $INSTALL_DIR"
        echo "  Run the installer first:  sudo $0 <binary-path>"
        exit 1
    fi
    
    echo
    echo -e "${CYAN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║       Configuration Wizard (Reconfigure)        ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════╝${NC}"
    echo
    
    IS_RECONFIGURE=true
    
    # If existing config exists, back it up and load all values
    if [[ -f "$config_file" ]]; then
        local backup_file="${config_file}.bak.$(date +%Y%m%d%H%M%S)"
        cp "$config_file" "$backup_file"
        print_status "Backed up existing config to $backup_file"
        
        # Load every setting from existing config into globals
        load_existing_config
    else
        print_warning "No existing configuration found — starting fresh."
    fi
    
    # Also try the credential backup file as a fallback for MongoDB creds
    load_mongodb_credentials
    
    echo
    echo "Each section will show your current settings."
    echo "Choose to keep them or enter new values."
    echo
    
    # Run the full wizard (each section checks IS_RECONFIGURE)
    configure_wizard
    
    # Write new config (FORCE_RECONFIG skips the existing-file guard)
    FORCE_RECONFIG=true
    create_config
    
    # Fix permissions
    if id "$SERVICE_USER" &>/dev/null; then
        chown "$SERVICE_USER:$SERVICE_USER" "$config_file" 2>/dev/null
        chmod 600 "$config_file" 2>/dev/null
    fi
    
    # Offer to restart the service
    if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo
        if prompt_yes_no "Service is running. Restart now to apply changes?" "y"; then
            systemctl restart "$SERVICE_NAME"
            sleep 2
            if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
                print_status "Service restarted successfully!"
            else
                print_warning "Service failed to start. Check logs: journalctl -u $SERVICE_NAME -n 30"
            fi
        else
            print_status "Config saved. Restart manually when ready:"
            echo "  sudo systemctl restart $SERVICE_NAME"
        fi
    else
        echo
        print_status "Configuration saved. Start the service with:"
        echo "  sudo systemctl start $SERVICE_NAME"
    fi
    
    echo
    # Print the credential summary
    print_completion
}

# Create default configuration
create_config() {
    local config_file="$CONFIG_DIR/config.json"
    
    if [[ -f "$config_file" ]] && ! $FORCE_RECONFIG; then
        print_warning "Configuration file already exists at $config_file"
        # If MongoDB was secured this run, update the connection string
        if [[ -n "$MONGO_AUTH_URI" ]]; then
            print_status "Updating MongoDB connection string with new credentials..."
            if command -v python3 &>/dev/null; then
                UF_CONFIG_FILE="$config_file" UF_MONGO_URI="$MONGO_AUTH_URI" UF_MONGO_DB="$MONGO_DB" python3 -c '
import json, os
cfg = os.environ["UF_CONFIG_FILE"]
with open(cfg, "r") as f:
    config = json.load(f)
config["DBServer"] = os.environ["UF_MONGO_URI"]
config["DB"] = os.environ["UF_MONGO_DB"]
with open(cfg, "w") as f:
    json.dump(config, f, indent=4)
print("Updated successfully")
' 2>/dev/null && print_status "Config updated with MongoDB credentials." || {
                    print_warning "Could not auto-update config. Please manually set DBServer in $config_file"
                    print_warning "  DBServer: $MONGO_AUTH_URI"
                }
            else
                sed -i "s|\"DBServer\":.*|\"DBServer\": \"${MONGO_AUTH_URI}\",|" "$config_file" 2>/dev/null
                print_status "Config updated with MongoDB credentials."
            fi
        fi
        # Ensure symlink exists even when config was pre-existing
        ln -sfn "$config_file" "$DATA_DIR/config.json" 2>/dev/null || true
        return
    fi
    
    print_status "Writing configuration file..."
    
    # Ensure MongoDB credentials are loaded from backup if globals are empty
    load_mongodb_credentials
    
    # Build values (JSON-escape anything that goes into the heredoc)
    local db_server
    db_server=$(json_escape "${MONGO_AUTH_URI:-mongodb://localhost:27017}")
    local db_name
    db_name=$(json_escape "${CFG_DB_NAME:-DayZ}")
    local port="${CFG_PORT:-443}"
    
    # Build ServerAuth JSON arrays
    local auth_tokens_json=""
    local auth_labels_json=""
    for i in "${!CFG_SERVER_AUTH_TOKENS[@]}"; do
        [[ $i -gt 0 ]] && auth_tokens_json+=", " || true
        auth_tokens_json+="\"${CFG_SERVER_AUTH_TOKENS[$i]}\""
    done
    for i in "${!CFG_SERVER_AUTH_LABELS[@]}"; do
        [[ $i -gt 0 ]] && auth_labels_json+=", " || true
        local esc_label
        esc_label=$(json_escape "${CFG_SERVER_AUTH_LABELS[$i]}")
        auth_labels_json+="\"${esc_label}\""
    done
    
    # Fallback if empty (shouldn't happen, but safety)
    if [[ -z "$auth_tokens_json" ]]; then
        local fallback_token
        fallback_token=$(generate_token 64)
        auth_tokens_json="\"$fallback_token\""
        auth_labels_json="\"DayZ Server 1\""
        CFG_SERVER_AUTH_TOKENS=("$fallback_token")
        CFG_SERVER_AUTH_LABELS=("DayZ Server 1")
    fi
    
    # Build Let's Encrypt altnames JSON array
    local le_altnames_json=""
    if [[ -n "$CFG_LE_ALTNAMES" ]]; then
        IFS=',' read -ra altarr <<< "$CFG_LE_ALTNAMES"
        for i in "${!altarr[@]}"; do
            local alt
            alt=$(echo "${altarr[$i]}" | xargs)  # trim whitespace
            local esc_alt
            esc_alt=$(json_escape "$alt")
            [[ $i -gt 0 ]] && le_altnames_json+=", " || true
            le_altnames_json+="\"$esc_alt\""
        done
    fi
    
    # Determine Let's Encrypt enabled
    local le_enabled="false"
    if [[ "$CFG_SSL_MODE" == "letsencrypt" ]]; then
        le_enabled="true"
    fi
    
    # Determine cert paths
    local cert_path
    cert_path=$(json_escape "${CFG_CERT_PATH}")
    local cert_key_path
    cert_key_path=$(json_escape "${CFG_CERT_KEY_PATH}")
    
    # Determine proxy settings
    local proxy_subdomain
    proxy_subdomain=$(json_escape "${CFG_PROXY_SUBDOMAIN}")
    local proxy_token
    proxy_token=$(json_escape "${CFG_PROXY_TOKEN}")
    local proxy_domain
    proxy_domain=$(json_escape "${CFG_PROXY_PRIMARY_DOMAIN}")
    local proxy_auto_renew="false"
    if [[ -n "$proxy_token" ]]; then
        proxy_auto_renew="true"
    fi
    
    # Determine tunnel settings
    local tunnel_enabled="false"
    local tunnel_token
    tunnel_token=$(json_escape "${CFG_TUNNEL_TOKEN}")
    if [[ "$CFG_SSL_MODE" == "tunnel" && -n "$tunnel_token" ]]; then
        tunnel_enabled="true"
    fi

    # Escape user-entered strings for safe JSON embedding
    local esc_discord_bot_token esc_discord_client_id esc_discord_client_secret esc_discord_guild_id esc_openai_key esc_le_domain esc_le_email
    esc_discord_bot_token=$(json_escape "${CFG_DISCORD_BOT_TOKEN}")
    esc_discord_client_id=$(json_escape "${CFG_DISCORD_CLIENT_ID}")
    esc_discord_client_secret=$(json_escape "${CFG_DISCORD_CLIENT_SECRET}")
    esc_discord_guild_id=$(json_escape "${CFG_DISCORD_GUILD_ID}")
    esc_openai_key=$(json_escape "${CFG_OPENAI_KEY}")
    esc_le_domain=$(json_escape "${CFG_LE_DOMAIN}")
    esc_le_email=$(json_escape "${CFG_LE_EMAIL}")
    
    cat > "$config_file" << EOF
{
    "DBServer": "${db_server}",
    "DB": "${db_name}",
    "AllowClientWrite": false,
    "IP": "0.0.0.0",
    "Port": ${port},
    "CreateIndexes": true,
    "LogToFile": true,
    "CheckForNewVersion": true,
    "RequestLimit": 500,
    "RequestLimitQuery": 400,
    "RequestLimitStatus": 100,
    "RequestLimitServerQuery": 200,
    "RequestLimitTranslate": 200,
    "RequestLimitLogger": 500,
    "RequestLimitCrypto": 150,
    "RateLimitWhiteList": ["127.0.0.1"],
    "ServerAuth": [${auth_tokens_json}],
    "ServerAuthLabels": [${auth_labels_json}],
    "Certificate": "${cert_path}",
    "CertificateKey": "${cert_key_path}",
    "Discord": {
        "Client_Id": "${esc_discord_client_id}",
        "Client_Secret": "${esc_discord_client_secret}",
        "Bot_Token": "${esc_discord_bot_token}",
        "Guild_Id": "${esc_discord_guild_id}",
        "AllowToReRegister": false,
        "Restrict_Sign_Up": false,
        "Required_Role": "",
        "BlackList_Role": "",
        "Restrict_Sign_Up_Countries": []
    },
    "OpenAIApi": {
        "ApiKey": "${esc_openai_key}",
        "enablePromptProtection": true
    },
    "Functions": {},
    "LetsEncypt": {
        "Enabled": ${le_enabled},
        "Domain": "${esc_le_domain}",
        "Email": "${esc_le_email}",
        "AltNames": [${le_altnames_json}]
    },
    "Proxy": {
        "primaryDomain": "${proxy_domain}",
        "subdomain": "${proxy_subdomain}",
        "token": "${proxy_token}",
        "lastRenew": "",
        "autoRenew": ${proxy_auto_renew}
    },
    "Tunnel": {
        "enabled": ${tunnel_enabled},
        "token": "${tunnel_token}",
        "autoStart": true
    }
}
EOF
    
    # Create symlink in data directory
    ln -sfn "$config_file" "$DATA_DIR/config.json"
    
    print_status "Configuration saved to $config_file"
}

# Create systemd service
create_systemd_service() {
    print_status "Creating systemd service..."
    
    cat > "/etc/systemd/system/${SERVICE_NAME}.service" << EOF
[Unit]
Description=Universal Framework Service for DayZ
Documentation=https://github.com/daemonforge/DayZ-UniveralApi
After=network.target mongod.service
Wants=mongod.service

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=$DATA_DIR
Environment="UF_SAVE_PATH=$DATA_DIR/"
Environment="NODE_ENV=production"
ExecStart=$INSTALL_DIR/$BINARY_NAME
Restart=always
RestartSec=10
StandardOutput=append:$LOG_DIR/stdout.log
StandardError=append:$LOG_DIR/stderr.log

# Security hardening
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
PrivateTmp=true
ReadWritePaths=$DATA_DIR $LOG_DIR $CONFIG_DIR $INSTALL_DIR

# Allow binding to port 443 as non-root
AmbientCapabilities=CAP_NET_BIND_SERVICE
CapabilityBoundingSet=CAP_NET_BIND_SERVICE

# Additional hardening
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true
RestrictSUIDSGID=true
RemoveIPC=true
PrivateDevices=true

# Resource limits
LimitNOFILE=65535
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
}

# Set permissions
set_permissions() {
    print_status "Setting permissions..."
    
    chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"
    chown -R "$SERVICE_USER:$SERVICE_USER" "$DATA_DIR"
    chown -R "$SERVICE_USER:$SERVICE_USER" "$LOG_DIR"
    chown -R "$SERVICE_USER:$SERVICE_USER" "$CONFIG_DIR"
    
    chmod 750 "$INSTALL_DIR"
    chmod 750 "$DATA_DIR"
    chmod 750 "$LOG_DIR"
    chmod 750 "$CONFIG_DIR"
    chmod 640 "$CONFIG_DIR/config.json" 2>/dev/null || true

    # Restore root-only ownership on credential backup (recursive chown above changes it)
    if [[ -f "$CONFIG_DIR/.mongodb_credentials" ]]; then
        chown root:root "$CONFIG_DIR/.mongodb_credentials" 2>/dev/null || true
        chmod 600 "$CONFIG_DIR/.mongodb_credentials" 2>/dev/null || true
    fi
}

# Verify the installation
verify_installation() {
    print_header "Verifying Installation"
    
    local issues=0
    
    # Check binary exists and is executable
    if [[ -x "$INSTALL_DIR/$BINARY_NAME" ]]; then
        print_status "Binary:    ✓ Installed and executable"
    else
        print_error "Binary:    ✗ Missing or not executable"
        issues=$((issues + 1))
    fi
    
    # Check config exists
    if [[ -f "$CONFIG_DIR/config.json" ]]; then
        print_status "Config:    ✓ Present"
        # Ensure the data-dir symlink exists so the binary can find the config
        if [[ ! -e "$DATA_DIR/config.json" ]]; then
            ln -sfn "$CONFIG_DIR/config.json" "$DATA_DIR/config.json" 2>/dev/null || true
            print_warning "Config:    Recreated missing symlink $DATA_DIR/config.json → $CONFIG_DIR/config.json"
        fi
    else
        print_error "Config:    ✗ Missing at $CONFIG_DIR/config.json"
        issues=$((issues + 1))
    fi
    
    # Check service user exists
    if id "$SERVICE_USER" &>/dev/null; then
        print_status "User:      ✓ '$SERVICE_USER' exists"
    else
        print_error "User:      ✗ '$SERVICE_USER' not found"
        issues=$((issues + 1))
    fi
    
    # Check systemd service is loaded
    if systemctl is-enabled "$SERVICE_NAME" &>/dev/null || systemctl list-unit-files | grep -q "$SERVICE_NAME"; then
        print_status "Service:   ✓ Systemd unit installed"
    else
        print_error "Service:   ✗ Systemd unit not found"
        issues=$((issues + 1))
    fi
    
    # Check MongoDB
    if systemctl is-active --quiet mongod 2>/dev/null; then
        print_status "MongoDB:   ✓ Running"
        # Check if auth is enabled
        if grep -qE '^\s*authorization:\s*enabled' /etc/mongod.conf 2>/dev/null; then
            print_status "MongoDB:   ✓ Authentication enabled"
        else
            print_warning "MongoDB:   ⚠ Authentication NOT enabled (insecure)"
        fi
    else
        print_warning "MongoDB:   ⚠ Not running (start with: sudo systemctl start mongod)"
    fi
    
    # Check FFmpeg
    if [[ -x "$INSTALL_DIR/bin/ffmpeg" ]] || command -v ffmpeg &>/dev/null; then
        print_status "FFmpeg:    ✓ Available"
    else
        print_warning "FFmpeg:    ⚠ Not found (TTS features will not work)"
    fi
    
    # Check ImageMagick
    if [[ -x "$INSTALL_DIR/bin/magick" ]] || command -v magick &>/dev/null || command -v convert &>/dev/null; then
        print_status "Magick:    ✓ Available"
    else
        print_warning "Magick:    ⚠ Not found (DDS conversion will not work)"
    fi
    
    # Check directory permissions
    if [[ -d "$DATA_DIR" ]] && [[ "$(stat -c '%U' "$DATA_DIR" 2>/dev/null)" == "$SERVICE_USER" ]]; then
        print_status "Perms:     ✓ Correct ownership"
    else
        print_warning "Perms:     ⚠ Check ownership on $DATA_DIR"
    fi
    
    # Smoke test: try running the binary briefly to check it starts
    if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        print_status "Smoke:     ⊘ Skipped (service already running on port)"
    else
    print_status "Running binary smoke test..."
    local smoke_output
    local smoke_exit=0
    # Run as the service user to mimic real conditions; fall back to root if user doesn't exist
    if id "$SERVICE_USER" &>/dev/null; then
        smoke_output=$(timeout 8 sudo -u "$SERVICE_USER" \
            env UF_SAVE_PATH="$DATA_DIR/" NODE_ENV=production \
            "$INSTALL_DIR/$BINARY_NAME" 2>&1) || smoke_exit=$?
    else
        smoke_output=$(timeout 8 \
            env UF_SAVE_PATH="$DATA_DIR/" NODE_ENV=production \
            "$INSTALL_DIR/$BINARY_NAME" 2>&1) || smoke_exit=$?
    fi
    
    # Exit code 124 = timeout (binary ran long enough = SUCCESS, it started)
    # Exit code 0 = clean exit (unlikely but fine)
    # Anything else = crash
    if [[ $smoke_exit -eq 124 || $smoke_exit -eq 0 ]]; then
        print_status "Smoke:     ✓ Binary starts successfully"
    else
        print_error "Smoke:     ✗ Binary crashed (exit code $smoke_exit)"
        issues=$((issues + 1))
        if [[ -n "$smoke_output" ]]; then
            echo -e "  ${RED}Output:${NC}"
            echo "$smoke_output" | head -20 | sed 's/^/    /'
        elif [[ $smoke_exit -eq 4 ]]; then
            echo -e "  ${RED}Exit code 4 = JavaScript evaluation failure.${NC}"
            echo "    This typically means a V8 bytecode version mismatch."
            echo "    The binary was likely built with a different Node.js version"
            echo "    than the one embedded by pkg."
            echo "    Fix: rebuild with --no-bytecode --public --public-packages '*'"
        else
            echo -e "  ${RED}No output produced — possible causes:${NC}"
            echo "    • Wrong CPU architecture (run: uname -m)"
            echo "    • Missing shared library (run: sudo ldd $INSTALL_DIR/$BINARY_NAME)"
            echo "    • Incompatible glibc version (run: ldd --version)"
            echo "    • Corrupted binary (re-download and reinstall)"
        fi
    fi
    fi  # end smoke test skip
    
    echo
    if [[ $issues -eq 0 ]]; then
        print_status "All checks passed! Installation looks good."
    else
        print_error "$issues issue(s) found. Please fix them before starting the service."
    fi
}

# Print completion message
print_completion() {
    echo
    echo -e "${GREEN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║          Installation Complete!                  ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════╝${NC}"
    echo
    echo -e "${BLUE}Installation paths:${NC}"
    echo "  Binary:        $INSTALL_DIR/$BINARY_NAME"
    echo "  Configuration: $CONFIG_DIR/config.json"
    echo "  Data:          $DATA_DIR"
    echo "  Logs:          $LOG_DIR"
    
    # ── Credential Summary ──────────────────────────────────────
    echo
    echo -e "${GREEN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║     SAVE THESE CREDENTIALS — SHOWN ONCE ONLY    ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════╝${NC}"
    
    # MongoDB
    echo
    echo -e "${BLUE}── MongoDB ────────────────────────────────────────${NC}"
    if [[ -n "$MONGO_AUTH_URI" ]]; then
        echo -e "  Status:    ${GREEN}✓ Secured with authentication${NC}"
        echo -e "  Database:  ${YELLOW}${MONGO_DB}${NC}"
        echo -e "  Username:  ${YELLOW}${MONGO_USER}${NC}"
        echo -e "  Password:  ${YELLOW}${MONGO_PASS}${NC}"
        if [[ -f "$CONFIG_DIR/.mongodb_credentials" ]]; then
            echo -e "  Backup:    ${YELLOW}$CONFIG_DIR/.mongodb_credentials${NC}"
        fi
    else
        echo -e "  Status:    ${RED}⚠ No authentication (insecure)${NC}"
        echo -e "  URI:       mongodb://localhost:27017"
        echo -e "  ${YELLOW}Run: sudo $0 --secure-mongodb${NC}"
    fi
    
    # Server Auth Tokens
    echo
    echo -e "${BLUE}── DayZ Server Auth Tokens ────────────────────────${NC}"
    echo -e "  Put each token in the matching server's mod config:"
    echo -e "    ${YELLOW}\$profile:UF/UFramework.json${NC} → ${YELLOW}\"ServerAuth\"${NC}"
    echo
    for i in "${!CFG_SERVER_AUTH_TOKENS[@]}"; do
        local label="${CFG_SERVER_AUTH_LABELS[$i]:-Server $((i+1))}"
        echo -e "  ${GREEN}${label}${NC}"
        echo -e "    Token: ${YELLOW}${CFG_SERVER_AUTH_TOKENS[$i]}${NC}"
    done
    
    # SSL / Connection mode
    echo
    echo -e "${BLUE}── Connection Mode ────────────────────────────────${NC}"
    case "$CFG_SSL_MODE" in
        letsencrypt)
            echo -e "  Mode:     ${GREEN}Let's Encrypt (auto-renewing HTTPS)${NC}"
            echo -e "  Domain:   ${YELLOW}${CFG_LE_DOMAIN}${NC}"
            echo -e "  Email:    ${YELLOW}${CFG_LE_EMAIL}${NC}"
            if [[ -n "$CFG_LE_ALTNAMES" ]]; then
                echo -e "  AltNames: ${YELLOW}${CFG_LE_ALTNAMES}${NC}"
            fi
            echo
            echo -e "  ${YELLOW}DayZ mod ServerURL:${NC}  https://${CFG_LE_DOMAIN}:${CFG_PORT}"
            ;;
        selfsigned)
            echo -e "  Mode:     ${GREEN}Self-Signed Certificate${NC}"
            echo -e "  Cert:     ${YELLOW}${CFG_CERT_PATH}${NC}"
            echo -e "  Key:      ${YELLOW}${CFG_CERT_KEY_PATH}${NC}"
            echo
            echo -e "  ${YELLOW}DayZ mod ServerURL:${NC}  https://<your-ip>:${CFG_PORT}"
            ;;
        proxy)
            echo -e "  Mode:     ${GREEN}DaemonForge Proxy${NC}"
            if [[ -n "$CFG_PROXY_SUBDOMAIN" ]]; then
                echo -e "  URL:      ${YELLOW}https://${CFG_PROXY_SUBDOMAIN}.${CFG_PROXY_PRIMARY_DOMAIN}${NC}"
                echo -e "  Token:    ${YELLOW}${CFG_PROXY_TOKEN}${NC}"
                echo -e "  Renew:    Auto (every 24 hours)"
                echo
                echo -e "  ${YELLOW}DayZ mod ServerURL:${NC}  https://${CFG_PROXY_SUBDOMAIN}.${CFG_PROXY_PRIMARY_DOMAIN}"
            else
                echo -e "  Status:   ${YELLOW}Not yet registered — configure in config.json${NC}"
            fi
            ;;
        tunnel)
            echo -e "  Mode:     ${GREEN}Cloudflare Tunnel${NC}"
            echo -e "  Token:    ${YELLOW}${CFG_TUNNEL_TOKEN}${NC}"
            echo -e "  Status:   Auto-start enabled"
            echo
            echo -e "  ${YELLOW}DayZ mod ServerURL:${NC}  Set to your Cloudflare tunnel hostname"
            ;;
        *)
            echo -e "  Mode:     ${YELLOW}Built-in self-signed certificate (default)${NC}"
            echo -e "  ${YELLOW}DayZ mod ServerURL:${NC}  https://<your-ip>:${CFG_PORT}"
            ;;
    esac
    
    # Discord (if configured)
    if [[ -n "$CFG_DISCORD_BOT_TOKEN" ]]; then
        echo
        echo -e "${BLUE}── Discord ────────────────────────────────────────${NC}"
        echo -e "  Bot Token:     ${YELLOW}${CFG_DISCORD_BOT_TOKEN}${NC}"
        if [[ -n "$CFG_DISCORD_GUILD_ID" ]]; then
            echo -e "  Guild ID:      ${YELLOW}${CFG_DISCORD_GUILD_ID}${NC}"
        fi
        if [[ -n "$CFG_DISCORD_CLIENT_ID" ]]; then
            echo -e "  Client ID:     ${YELLOW}${CFG_DISCORD_CLIENT_ID}${NC}"
        fi
        if [[ -n "$CFG_DISCORD_CLIENT_SECRET" ]]; then
            echo -e "  Client Secret: ${YELLOW}${CFG_DISCORD_CLIENT_SECRET}${NC}"
        fi
    fi
    
    # OpenAI (if configured)
    if [[ -n "$CFG_OPENAI_KEY" ]]; then
        echo
        echo -e "${BLUE}── OpenAI ─────────────────────────────────────────${NC}"
        echo -e "  API Key:  ${YELLOW}${CFG_OPENAI_KEY}${NC}"
    fi
    
    # ── Next Steps ──────────────────────────────────────────────
    echo
    echo -e "${BLUE}━━━ Next Steps ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo
    
    local needs_edit=false
    
    # Check what still needs manual config
    if [[ ${#CFG_SERVER_AUTH_TOKENS[@]} -gt 0 ]]; then
        echo -e "  ${GREEN}✓${NC} ServerAuth tokens generated and saved"
    fi
    
    if [[ "$CFG_SSL_MODE" == "letsencrypt" || "$CFG_SSL_MODE" == "proxy" || "$CFG_SSL_MODE" == "tunnel" || "$CFG_SSL_MODE" == "selfsigned" ]]; then
        echo -e "  ${GREEN}✓${NC} Connection security configured ($CFG_SSL_MODE)"
    else
        echo -e "  ${YELLOW}!${NC} No connection security configured — edit config.json to set up HTTPS"
        needs_edit=true
    fi
    
    if [[ -n "$MONGO_AUTH_URI" ]]; then
        echo -e "  ${GREEN}✓${NC} MongoDB secured with authentication"
    else
        echo -e "  ${RED}⚠${NC} MongoDB has NO authentication — secure it manually!"
        needs_edit=true
    fi
    
    if $needs_edit; then
        echo
        echo -e "  ${YELLOW}Edit the configuration:${NC}"
        echo "    sudo nano $CONFIG_DIR/config.json"
    fi
    
    echo
    echo -e "  ${YELLOW}Starting the service...${NC}"
    systemctl enable "$SERVICE_NAME" 2>/dev/null || true
    systemctl start "$SERVICE_NAME" 2>/dev/null || true
    sleep 3
    if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo -e "  ${GREEN}✓ Service is running!${NC}"
    else
        echo -e "  ${RED}✗ Service failed to start.${NC}"
        echo -e "  Check logs: ${YELLOW}sudo journalctl -u $SERVICE_NAME -n 30 --no-pager${NC}"
        echo -e "  Or run directly: ${YELLOW}sudo UF_SAVE_PATH=$DATA_DIR/ $INSTALL_DIR/$BINARY_NAME${NC}"
    fi
    echo
    echo -e "  ${BLUE}Useful commands:${NC}"
    echo "    sudo systemctl status $SERVICE_NAME    # check if running"
    echo "    sudo journalctl -u $SERVICE_NAME -f    # live log stream"
    echo "    sudo $0 --status                       # installation health check"
    echo "    tail -f $LOG_DIR/stdout.log            # application logs"
    echo
    echo -e "${BLUE}Full documentation:${NC} https://github.com/daemonforge/DayZ-UniveralApi"
    echo
}

# Uninstall function
uninstall() {
    print_status "Uninstalling Universal Framework Service..."
    
    if ! $AUTO_YES; then
        read -p "This will remove the service and binary. Continue? [y/N] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_status "Uninstall cancelled."
            exit 0
        fi
    fi
    
    # Stop and disable service
    systemctl stop "$SERVICE_NAME" 2>/dev/null || true
    systemctl disable "$SERVICE_NAME" 2>/dev/null || true
    
    # Remove systemd service file
    rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
    systemctl daemon-reload
    
    # Remove installation directory
    rm -rf "$INSTALL_DIR"
    
    # Optionally remove data (ask user)
    if $AUTO_YES; then
        print_warning "Keeping data ($DATA_DIR), config ($CONFIG_DIR), and logs ($LOG_DIR)."
        print_warning "Delete them manually if no longer needed."
    else
        read -p "Remove data directory ($DATA_DIR)? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$DATA_DIR"
        fi
        
        read -p "Remove configuration ($CONFIG_DIR)? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$CONFIG_DIR"
        fi
        
        read -p "Remove logs ($LOG_DIR)? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$LOG_DIR"
        fi
    fi
    
    # Remove user (optional)
    if $AUTO_YES; then
        print_status "Keeping service user '$SERVICE_USER'."
    else
        read -p "Remove service user ($SERVICE_USER)? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            userdel "$SERVICE_USER" 2>/dev/null || true
        fi
    fi
    
    print_status "Uninstallation complete"
}

# Show usage
usage() {
    echo "Universal Framework Service - Linux Installer"
    echo
    echo "Usage: $0 [OPTIONS] <binary-path>"
    echo
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -u, --uninstall     Uninstall the service"
    echo "  -s, --status        Check installation health"
    echo "  -c, --configure     Re-run the configuration wizard"
    echo "  --secure-mongodb    Re-run MongoDB security setup"
    echo "  -f, --ffmpeg        Path to FFmpeg binary to bundle"
    echo "  -m, --magick        Path to ImageMagick binary to bundle"
    echo "  -y, --yes           Skip confirmation prompts (auto-yes)"
    echo
    echo "Examples:"
    echo "  $0 ./ufserverservice-linux                              # Install interactively"
    echo "  $0 -y ./ufserverservice-linux                           # Auto-install without prompts"
    echo "  $0 -f ./bin/ffmpeg -m ./bin/magick ./ufserverservice-linux"
    echo "  $0 --status                                             # Check installation health"
    echo "  $0 --configure                                          # Re-run configuration wizard"
    echo "  $0 --secure-mongodb                                     # Re-run MongoDB security"
    echo "  $0 --uninstall                                          # Remove the service"
}

# Main
main() {
    local binary_path=""
    local ffmpeg_path=""
    local magick_path=""
    local do_uninstall=false
    local do_status=false
    local do_configure=false
    local do_secure_mongo=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
                ;;
            -u|--uninstall)
                do_uninstall=true
                shift
                ;;
            -s|--status)
                do_status=true
                shift
                ;;
            -c|--configure|--reconfigure|--wizard)
                do_configure=true
                shift
                ;;
            --secure-mongodb|--mongo-security)
                do_secure_mongo=true
                shift
                ;;
            -f|--ffmpeg)
                if [[ -z "${2:-}" ]]; then
                    print_error "--ffmpeg requires a path argument"
                    exit 1
                fi
                ffmpeg_path="$2"
                shift 2
                ;;
            -m|--magick)
                if [[ -z "${2:-}" ]]; then
                    print_error "--magick requires a path argument"
                    exit 1
                fi
                magick_path="$2"
                shift 2
                ;;
            -y|--yes)
                AUTO_YES=true
                shift
                ;;
            *)
                binary_path="$1"
                shift
                ;;
        esac
    done
    
    check_root
    detect_package_manager
    
    if $do_uninstall; then
        uninstall
        exit 0
    fi
    
    if $do_configure; then
        reconfigure
        exit 0
    fi
    
    if $do_secure_mongo; then
        resecure_mongodb
        exit 0
    fi
    
    if $do_status; then
        verify_installation
        echo
        # Also show service status if running
        if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
            echo -e "${GREEN}Service Status: RUNNING${NC}"
            systemctl status "$SERVICE_NAME" --no-pager -l 2>/dev/null | head -15
        else
            echo -e "${YELLOW}Service Status: STOPPED${NC}"
            echo "  Start with: sudo systemctl start $SERVICE_NAME"
        fi
        exit 0
    fi
    
    if [[ -z "$binary_path" ]]; then
        # Try to find binary in current directory
        if [[ -f "./$BINARY_NAME" ]]; then
            binary_path="./$BINARY_NAME"
        else
            print_error "No binary path provided"
            usage
            exit 1
        fi
    fi
    
    # Auto-detect bundled binaries in the same directory as the service binary
    local binary_dir
    binary_dir="$(dirname "$binary_path")"
    if [[ -z "$ffmpeg_path" && -f "$binary_dir/bin/ffmpeg" ]]; then
        ffmpeg_path="$binary_dir/bin/ffmpeg"
        print_status "Auto-detected bundled FFmpeg at $ffmpeg_path"
    fi
    if [[ -z "$magick_path" && -f "$binary_dir/bin/magick" ]]; then
        magick_path="$binary_dir/bin/magick"
        print_status "Auto-detected bundled ImageMagick at $magick_path"
    fi
    
    # Always install dependencies (use -y to skip prompts)
    print_welcome
    
    print_step 1 8 "Installing Dependencies"
    install_dependencies
    
    print_step 2 8 "Checking Dependencies"
    check_dependencies
    
    print_step 3 8 "Creating Service User & Directories"
    create_user
    create_directories
    
    print_step 4 8 "Installing Binaries"
    install_binary "$binary_path"
    install_ffmpeg "$ffmpeg_path"
    install_magick "$magick_path"
    
    # Secure MongoDB if it was freshly installed or has no auth
    print_step 5 8 "Securing MongoDB"
    if systemctl is-active --quiet mongod 2>/dev/null; then
        if ! grep -qE '^\s*authorization:\s*enabled' /etc/mongod.conf 2>/dev/null; then
            local do_secure=true
            if ! $AUTO_YES; then
                echo
                echo -e "${YELLOW}MongoDB does not have authentication enabled.${NC}"
                echo "  This means anyone with network access to port 27017 can read/write your database."
                echo "  The installer can automatically:"
                echo "    • Create a dedicated database user with a random password"
                echo "    • Enable authentication in MongoDB"
                echo "    • Bind MongoDB to localhost only"
                echo "    • Save the credentials to your config.json"
                echo
                read -p "Secure MongoDB now? (strongly recommended) [Y/n] " -n 1 -r
                echo
                [[ $REPLY =~ ^[Nn]$ ]] && do_secure=false || true
            fi
            if $do_secure; then
                setup_mongodb_security || print_warning "MongoDB security setup encountered issues. Review the output above."
            else
                print_warning "Skipping MongoDB security setup. Your database has NO password!"
            fi
        else
            print_status "MongoDB authentication is already enabled. Skipping."
        fi
    else
        print_warning "MongoDB is not running. Skipping security setup."
        print_warning "After starting MongoDB, secure it manually:"
        print_warning "  https://www.mongodb.com/docs/manual/tutorial/enable-authentication/"
    fi
    
    print_step 6 8 "Configuration Wizard"
    # Ensure MongoDB credentials are loaded (from backup file if globals empty)
    load_mongodb_credentials
    configure_wizard
    
    print_step 7 8 "Writing Config & Systemd Service"
    create_config
    create_systemd_service
    set_permissions
    
    print_step 8 8 "Verifying Installation"
    verify_installation
    
    print_completion
}

main "$@"
