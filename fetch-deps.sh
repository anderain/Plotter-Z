#!/bin/bash
# fetch-deps.sh
# Clones/updates dependency projects into the 'deps' folder at project root.

set -e  # Exit immediately on error

# Get the directory where this script resides (project root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/deps"

# ==================== User configuration ====================
# List of GitHub repositories to clone.
# Format: each line "repo_url [custom_subdir]"
# If no custom subdir given, use the repo name (without .git suffix)
REPOS=(
    "https://github.com/anderain/salvia89.git salvia89"
    # Add more dependencies as needed...
)
# ============================================================

# Create deps directory if it doesn't exist
mkdir -p "$DEPS_DIR"

# Function to clone or update a single dependency
clone_or_update() {
    local url="$1"
    local target="$2"
    local target_path="$DEPS_DIR/$target"

    if [ -d "$target_path/.git" ]; then
        echo "Dependency already exists: $target, skipping clone."
        # Uncomment the next three lines to automatically git pull instead
        # echo "   Running git pull..."
        # (cd "$target_path" && git pull --rebase)
    else
        echo "Cloning $url -> $target"
        git clone "$url" "$target_path"
        echo "   Done."
    fi
}

# Process each repository entry
for repo in "${REPOS[@]}"; do
    # Parse URL and optional custom target directory name
    if [[ "$repo" =~ ^[[:space:]]*([^[:space:]]+)[[:space:]]+(.+)[[:space:]]*$ ]]; then
        url="${BASH_REMATCH[1]}"
        target="${BASH_REMATCH[2]}"
    else
        url="$repo"
        # Extract repo name from URL (strip .git suffix and trailing slash)
        target=$(basename "$url" .git)
    fi
    clone_or_update "$url" "$target"
done

echo "All dependencies processed."