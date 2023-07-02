#!/bin/bash

# Go to the data folder.
cd "$(dirname -- "$( readlink -f -- "$0"; )";)";

# MS-DOS.
cd msdos


# Download the Wolfenstein 3D shareware episode.
mkdir -p wolf3dsw
cd wolf3dsw
wget -nc https://archive.org/download/Wolfenstein3d/Wolfenstein3dV14sw.ZIP
cd ..

# Download the Doom 1 shareware episode.
mkdir -p doom1sw
cd doom1sw
wget -nc https://archive.org/download/DoomsharewareEpisode/DoomV1.9sw1995idSoftwareInc.action.zip
# Add sound settings.
zip DoomV1.9sw1995idSoftwareInc.action.zip DEFAULT.CFG
cd ..

# Download The Incredible Machine.
mkdir -p tim
cd tim
wget -nc https://archive.org/download/TheIncredibleMachine_201901/TIM.zip
cd ..

# Download Captain Comic.
mkdir -p comic1
cd comic1
wget -nc https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeathR4sw1989michaelA.Denioaction.zip
cd ..


cd ..


