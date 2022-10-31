# audacious-plugin-rpc
A Discord Rich Presence plugin for the Audacious music player!

# Usage
1. Download the current release from the [releases page](https://github.com/crackheadakira/audacious-plugin-rpc/releases).
2. Extract `libaudacious-plugin-rpc.so` into the folder `/usr/lib/audacious/General/`.
3. Open Audacious, go to Settings > Plugins and enable the `Discord RPC` plugin.
4. Go to settings, and enable the album art option if you want to see the album art in your Discord status.

# Screenshots
![Screenshot 1](https://i.imgur.com/0l4zP8h.png)
![Screenshot 2](https://i.imgur.com/UmfeJzt.png)

# Compilation
1. Clone the repository.
2. Compile and install the plugin:
```
mkdir build
cd build
cmake ..
make install
```
