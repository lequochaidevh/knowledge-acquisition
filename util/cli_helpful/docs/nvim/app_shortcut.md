### Neotree
```shortcut
space v : open
a : create file
r : rename file
o : revert change file before
f : search file, open and space v
c : cp file
ctrl + alt + <w,h.l> switch neotree and editor
```

### Insert mode
```shortcut
di" : delete all word in "..."
di< : delete all word in <...>
da" : detele string include "
da" : detele string include "
dt" : delete all word from CURSOR to next "

ctrl+ alt+ s : save file
<C-w>
<C-u>
<C-w>
```

### Comment code
```shortcut
:%s/^/\/\//
#or use plugin
V -> gc
```

### Remote editor
```sh
nvim scp://user@remote-ip//path/to/folder/
```
**Note**: Need "//"

### Multi insert word in multi line parrallel

**normal**
```shortcut
ctrl + v : visual block
h j k l : choose string
:normal i"yrstring"

or: :s/^/"string"/

delete: norm xx : remove 2 charactor
```

### Replace string at line
```shortcut
:s/old/new/g : replace all old string by new. can add gc to pick y/n.
:s/old/new/ : replace first string.
```

### Replace string all file.
```shortcut
:%s/old/new/g : replace old by new string all file.
```

### Replace with visual mode
```shortcut
v and choice string
:s/old/new/g
```

**Note:** `i` : upcase and lowcase (ignore case)
**Ex:** Replace Enter

```shortcut
:%s/\n/ \\n /g
```

