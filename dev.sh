#!/bin/bash
# Save as dev.sh in project root

set -e  # Exit on error

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

case "$1" in
    clean)
        echo -e "${BLUE}🧹 Cleaning build...${NC}"
        rm -rf build
        ;;
    
    config)
        echo -e "${BLUE}⚙️  Configuring...${NC}"
        cmake -S . -B build -G Ninja
        ;;
    
    build)
        echo -e "${BLUE}🔨 Building...${NC}"
        cmake --build build
        echo -e "${GREEN}✅ Build successful!${NC}"
        ;;
    
    run)
        echo -e "${BLUE}🚀 Running game...${NC}"
        ./build/game
        ;;
    
    dev)
        # Quick rebuild and run
        cmake --build build && ./build/game
        ;;
    
    fresh)
        # Full clean rebuild
        echo -e "${BLUE}🔄 Fresh build...${NC}"
        rm -rf build
        cmake -S . -B build -G Ninja
        cmake --build build
        echo -e "${GREEN}✅ Ready!${NC}"
        ./build/game
        ;;
    
    *)
        echo "Usage: ./dev.sh {clean|config|build|run|dev|fresh}"
        echo ""
        echo "  clean  - Remove build directory"
        echo "  config - Run CMake configuration"
        echo "  build  - Compile the project"
        echo "  run    - Run the game"
        echo "  dev    - Quick build + run (use this most!)"
        echo "  fresh  - Clean + config + build + run"
        exit 1
        ;;
esac
