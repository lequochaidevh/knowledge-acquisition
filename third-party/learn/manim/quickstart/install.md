### Ubuntu 20.04 or newer.

```sh
sudo apt install manim
pip3 install manim
```

### Package requirement
```sh
sudo apt install texlive-full dvisvgm
```

```sh
mkdir my_manim_project
cd my_manim_project
python3 -m venv venv
```

### New terminal session
```sh
source venv/bin/activate
# or: source my_manim_project/venv/bin/activate - when at 'quickstart'
```

### CREATE capture:
```pkg
pip freeze > requirements.txt
```
