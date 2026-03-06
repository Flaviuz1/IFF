#!/bin/bash
echo "Downloading IFF compiler..."
ARCH=$(uname -s)
if [ "$ARCH" = "Darwin" ]; then
    curl -L "https://github.com/Flaviuz1/IFF/releases/latest/download/iff-macOS" -o /tmp/iff
else
    curl -L "https://github.com/Flaviuz1/IFF/releases/latest/download/iff-Linux" -o /tmp/iff
fi
sudo mv /tmp/iff /usr/local/bin/iff
sudo chmod +x /usr/local/bin/iff
echo "Done! Run 'iff yourfile.iff' to get started."