#!/bin/bash
set -e

# Add all changes, including ServiceMock.h
git add .

# Commit with a clear message
git commit -m "Responder/mock wiring fixes, ServiceMock.h update, and L0 run preparations"

# Check if remote exists and push to current branch
remote_name=$(git remote)
if [ -z "$remote_name" ]; then
  echo "No remote configured for this repo."
  echo "Please add a remote (e.g., git remote add origin <url>) and retry."
  exit 1
fi

current_branch=$(git branch --show-current)
git push "$remote_name" "$current_branch"
