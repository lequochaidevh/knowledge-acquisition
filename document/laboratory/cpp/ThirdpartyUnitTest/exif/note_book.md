# Exif
### Command
```sh
sudo apt-get update && sudo apt-get install -y exiftool
cd build
ffmpeg -i ../HarisName.png -map_metadata 0 ../HarisName.jpg && mv ../HarisName.jpg ../build
# ffmpeg -y -i ../HarisName.png -map_metadata 0 ../HarisName.jpg && mv ../HarisName.jpg ../build
```

### Run
```sh
./sandbox ../HarisName.jpg
```