#!/bin/bash
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "Downloading IFF compiler for $OS..."
curl -L "https://github.com/Flaviuz1/IFF/releases/latest/download/iff-Linux" -o /tmp/iff
sudo mv /tmp/iff /usr/local/bin/iff
sudo chmod +x /usr/local/bin/iff
echo "Done! Run 'iff yourfile.iff' to get started."