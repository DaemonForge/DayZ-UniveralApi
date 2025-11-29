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

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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
        print_warning "  - mongodb or mongodb-org (database)"
        return
    fi
    
    local install_ffmpeg=false
    local install_imagemagick=false
    
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
    
    if $install_ffmpeg || $install_imagemagick; then
        read -p "Install missing dependencies (ffmpeg, imagemagick)? [Y/n] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            print_status "Updating package lists..."
            $PKG_UPDATE
            
            case $PKG_MANAGER in
                apt)
                    $install_ffmpeg && $PKG_INSTALL ffmpeg
                    $install_imagemagick && $PKG_INSTALL imagemagick
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
                    $install_ffmpeg && $PKG_INSTALL ffmpeg
                    $install_imagemagick && $PKG_INSTALL imagemagick
                    ;;
                zypper)
                    $install_ffmpeg && $PKG_INSTALL ffmpeg
                    $install_imagemagick && $PKG_INSTALL ImageMagick
                    ;;
            esac
        fi
    fi
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
        print_status "Install MongoDB with one of these commands:"
        echo "  Debian/Ubuntu: sudo apt install mongodb"
        echo "  Fedora/RHEL:   sudo dnf install mongodb-org"
        echo "  Arch Linux:    sudo pacman -S mongodb"
        echo
        read -p "Continue without MongoDB? [y/N] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
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
    
    print_status "Installing binary..."
    cp "$binary_path" "$INSTALL_DIR/$BINARY_NAME"
    chmod +x "$INSTALL_DIR/$BINARY_NAME"
}

# Install FFmpeg binary (if bundled)
install_ffmpeg() {
    local ffmpeg_path="$1"
    
    if [[ -f "$ffmpeg_path" ]]; then
        print_status "Installing bundled FFmpeg..."
        mkdir -p "$INSTALL_DIR/bin"
        cp "$ffmpeg_path" "$INSTALL_DIR/bin/ffmpeg"
        chmod +x "$INSTALL_DIR/bin/ffmpeg"
    fi
}

# Create default configuration
create_config() {
    local config_file="$CONFIG_DIR/config.json"
    
    if [[ -f "$config_file" ]]; then
        print_warning "Configuration file already exists at $config_file"
        print_warning "Skipping configuration creation. Edit manually if needed."
        return
    fi
    
    print_status "Creating default configuration..."
    cat > "$config_file" << 'EOF'
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "AllowClientWrite": false,
    "IP": "0.0.0.0",
    "Port": 8443,
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
    "ServerAuth": [],
    "ServerAuthLabels": [],
    "Certificate": "",
    "CertificateKey": "",
    "Discord": {
        "Client_Id": "",
        "Client_Secret": "",
        "Bot_Token": "",
        "Guild_Id": "",
        "AllowToReRegister": false,
        "Restrict_Sign_Up": false,
        "Required_Role": "",
        "BlackList_Role": "",
        "Restrict_Sign_Up_Countries": []
    },
    "OpenAIApi": {
        "ApiKey": "",
        "enablePromptProtection": true
    },
    "Functions": {},
    "LetsEncypt": {
        "Enabled": false,
        "Domain": "",
        "Email": "",
        "AltNames": []
    }
}
EOF
    
    # Create symlink in data directory
    ln -sfn "$config_file" "$DATA_DIR/config.json"
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
ReadWritePaths=$DATA_DIR $LOG_DIR $CONFIG_DIR

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
}

# Print completion message
print_completion() {
    echo
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Installation Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo
    echo "Installation paths:"
    echo "  Binary:        $INSTALL_DIR/$BINARY_NAME"
    echo "  Configuration: $CONFIG_DIR/config.json"
    echo "  Data:          $DATA_DIR"
    echo "  Logs:          $LOG_DIR"
    echo
    echo -e "${BLUE}Next steps:${NC}"
    echo "  1. Edit the configuration file:"
    echo "     sudo nano $CONFIG_DIR/config.json"
    echo
    echo "  2. Ensure MongoDB is running:"
    echo "     sudo systemctl start mongod"
    echo "     sudo systemctl enable mongod"
    echo
    echo "  3. Start the service:"
    echo "     sudo systemctl start $SERVICE_NAME"
    echo
    echo "  4. Enable auto-start on boot:"
    echo "     sudo systemctl enable $SERVICE_NAME"
    echo
    echo "  5. Check service status:"
    echo "     sudo systemctl status $SERVICE_NAME"
    echo
    echo "  6. View logs:"
    echo "     sudo journalctl -u $SERVICE_NAME -f"
    echo "     tail -f $LOG_DIR/stdout.log"
    echo
    echo -e "${BLUE}Feature Requirements:${NC}"
    echo "  - TTS (Text-to-Speech): Requires FFmpeg"
    echo "    Install: apt install ffmpeg (Debian/Ubuntu)"
    echo "            dnf install ffmpeg (Fedora)"
    echo
    echo "  - Image DDS Conversion: Requires ImageMagick with DDS support"
    echo "    Install: apt install imagemagick (Debian/Ubuntu)"
    echo "            dnf install ImageMagick (Fedora)"
    echo
}

# Uninstall function
uninstall() {
    print_status "Uninstalling Universal Framework Service..."
    
    # Stop and disable service
    systemctl stop "$SERVICE_NAME" 2>/dev/null || true
    systemctl disable "$SERVICE_NAME" 2>/dev/null || true
    
    # Remove systemd service file
    rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
    systemctl daemon-reload
    
    # Remove installation directory
    rm -rf "$INSTALL_DIR"
    
    # Optionally remove data (ask user)
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
    
    # Remove user (optional)
    read -p "Remove service user ($SERVICE_USER)? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        userdel "$SERVICE_USER" 2>/dev/null || true
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
    echo "  -h, --help        Show this help message"
    echo "  -u, --uninstall   Uninstall the service"
    echo "  -f, --ffmpeg      Path to FFmpeg binary to bundle"
    echo "  -i, --install-deps  Automatically install dependencies"
    echo
    echo "Examples:"
    echo "  $0 ./ufserverservice-linux"
    echo "  $0 -i ./ufserverservice-linux     # Auto-install dependencies"
    echo "  $0 -f ./ffmpeg ./ufserverservice-linux"
    echo "  $0 --uninstall"
}

# Main
main() {
    local binary_path=""
    local ffmpeg_path=""
    local do_uninstall=false
    local auto_install_deps=false
    
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
            -f|--ffmpeg)
                ffmpeg_path="$2"
                shift 2
                ;;
            -i|--install-deps)
                auto_install_deps=true
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
    
    # Install dependencies if requested
    if $auto_install_deps; then
        install_dependencies
    fi
    
    check_dependencies
    create_user
    create_directories
    install_binary "$binary_path"
    install_ffmpeg "$ffmpeg_path"
    create_config
    create_systemd_service
    set_permissions
    print_completion
}

main "$@"
