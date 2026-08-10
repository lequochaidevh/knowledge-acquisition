# Exif
### Command
```sh
sudo apt-get update && sudo apt-get install -y exiftool
cd build
ffmpeg -i ../HarisName.png -map_metadata 0 ../HarisName.jpg
# ffmpeg -y -i ../HarisName.png -map_metadata 0 ../HarisName.jpg
```

### Run
```sh
./sandbox ../HarisName.jpg
```