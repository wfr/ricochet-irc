#!/usr/bin/env bash

# Check if at least two arguments are provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 channel version [commit]"
    exit 1
fi

# Assign arguments to variables
channel=$1
version=$2
commit=${3:-HEAD}

# Check if the channel name is valid
valid_channels=("release" "alpha")
if ! [[ " ${valid_channels[@]} " =~ " $channel " ]]; then
    echo "Invalid channel name. Valid channel names are: ${valid_channels[*]}"
    exit 1
fi

# Validate semantic version format
if ! [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Invalid version format. Please provide semantic version (e.g., 1.2.3)"
    exit 1
fi

# Sign and tag the specified git commit
tag_name="v${version}-${channel}"
commit_message="tagging ${tag_name}"

echo "Signing and tagging commit $commit with tag name: ${tag_name}"
git tag -s "$tag_name" "$commit" -m "$commit_message"
