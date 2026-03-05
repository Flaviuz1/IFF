#!/bin/bash
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "Downloading IFF compiler for $OS..."
curl -L "https://github.com/Flaviuz1/iff/releases/latest/download/iff-$OS" -o /usr/local/bin/iff
chmod +x /usr/local/bin/iff
echo "Done! Run 'iff yourfile.iff' to get started."