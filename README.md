## Qt Hyprpaper GUI

A High-Performance GUI wallpaper selector for Hyprpaper, built with C++ using QT Framework.

## About author and this Project

Qt-Hyprpaper-GUI, I created this in my free time, so it’s my exploration stuff. There are some others who created the same concept; some of them used Python Pygame, which in my experience uses high CPU and opens a little bit slowly, then I created my own version with Rust (https://github.com/Kokuse557/Hyprpaper-GUI) then i expand my exploration to C++ and QT framework. This QT version, has transparent baground so its kinda fit nicely especially inside Hyprland.

## Why QT framework?

One of the reason was, im using linux with Hyprland desktop and there is a bunch of QT apps already, such as DaVinci Resolve Studio (video/audio editing, compositing), Strawberry (music player), OBS Studio (Video/Audio Recording), VLC Player (Video/Audio Player) so im thinking about sort of QT-Ecosystem inside Hyprland.

## High-Performance GUI

Instead of targeting the main folder and rendering everything, this app basically:
- Tracks both ~/Pictures/Wallpapers and ~/.local/cache/thumbnails
- Lists all MD5s and renders cached images linked to ~/Pictures/Wallpapers, so it has a really responsive GUI

## Features
- High Performance and clean transparent GUI with C++ and QT6
- GPU Accelerated, with CPU fallback using flag --cpu when having issue with GPU render
- Minimalistic design, Gives user full control of the app's sizing for their own Hyprland ricings via hyprland.conf
- Browse and preview image thumbnails efficiently via md5 and local thumbnail cache
- Select which monitor to apply the wallpaper to
- Scans folder ~/Pictures/Wallpapers and any folders underneath and gives user album separation in the app 
- Wallpaper changes loads immediatelly and send changes to hyprpaper.conf, that means the wallpaper persists EVEN AFTER RESTART!!! 
- Supports up to 10 monitors with clean changes in theory
- Supports Kvantum theme, but because the app basically transparent, it only applies to combobox and scrollbar

## NOTICE
- Make sure you set ~/config/hypr/hyprpaper.conf "ipc = on" so the application can call "hyprctl hyprpaper ...". Otherwise, the command won’t find the Hyprpaper socket.
- It runs automatically with GPU acceleration. If there is some artifacts, maybe nvidia, u can try use flag --cpu to use software render.
- For convenience, place all of your wallpapers in ~/Pictures/Wallpapers and then you can add more wallpaper folders underneath.
- This app generates text to preload and load entries inside hyprpaper.conf via lockdown per lines, line 8-30 (if u're using 10 monitors) so users can add more config from line 1-7
- If you need clean hyprpaper.conf, u can grab from /docs/hyprpaper.conf and then overwrite the existing one at ~/.config/hypr/

## Screenshots
![Qt Hyprpaper GUI Screenshot](docs/Qt-Hyprpaper-GUI_1_hyprshot.png)
![Qt Hyprpaper GUI Screenshot](docs/Qt-Hyprpaper-GUI_2_hyprshot.png)

## Building & Running

```bash
git clone https://github.com/Kokuse557/Qt-Hyprpaper-GUI.git
cd Qt-Hyprpaper-GUI

# Debug build (inside build folder)
cmake .. 
make 

# Release build (optimized build, run inside root /Qt-Hyprpaper-GUI folder)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

