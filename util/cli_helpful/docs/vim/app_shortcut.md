# Essential Developer Shortcuts and Commands

A curated list of essential Neovim / Vim commands, shortcuts, and plugins for everyday development.

---

### Neotree (File Tree Navigation)

| Shortcut | Action |
| :--- | :--- |
| `space` + `v` | Open / Toggle Neotree |
| `a` | Create a new file or directory |
| `r` | Rename a file or directory |
| `o` | Revert modifications on a file |
| `f` | Search for a file, open it, and toggle Neotree (`space` + `v`) |
| `c` | Copy a file |
| `Ctrl` + `Alt` + `<w, h, l>` | Switch focus between Neotree panel and the Editor window |

---

### Text Editing & Deletion (Normal/Insert Mode Operations)

#### Text Objects & Target Deletions (Normal Mode)
* `di"`: Delete all text inside quotes `""` (keeps the quotes).
* `di<`: Delete all text inside angle brackets `<>` (keeps the brackets).
* `da"`: Delete the entire string, including the surrounding quotes `""`.
* `dt"`: Delete everything from the current cursor position up to (but not including) the next quote `"`.

#### Editor Control
* `Ctrl` + `Alt` + `s`: Save the current file.
* `<C-w>`: Multi-functional window/buffer management prefix or word deletion in Insert mode.
* `<C-u>`: Clear the line backward from the current cursor position.

---

### Commenting Code
Toggle code comments efficiently using native search-and-replace or via plugins.

#### Native Vim Method (Prepend comments to lines)
```vim
:%s/^/\/\//
```

#### Plugin Method (e.g., mini.comment or Comment.nvim)
* Press `V` to enter **Visual Line mode**, select your lines, then press `gc` to toggle the comment block.

---

### Remote Editing via SCP
Edit files directly on a remote server without fully SSHing into a shell.

```bash
nvim scp://user@remote-ip//path/to/folder/
```
> **Note:** The double trailing forward slash `//` is strict syntax required to specify absolute paths on the remote server.

---

### Multi-Line Parallel Text Insertion

#### Method 1: Using Visual Block & Normal Execution
1. Press `Ctrl` + `v` to enter **Visual Block mode**.
2. Use navigation keys `h`, `j`, `k`, `l` to highlight the targeted text block or column.
3. Run the normal execution command to insert text globally:
   ```vim
   :normal i"your_string"
   ```

#### Method 2: Global Line Ingestion
* Append text to the start of targeted lines:
  ```vim
  :s/^/"string"/
  ```

#### Multi-Line Deletion
* Highlight your target vertical lines using block mode, then execute the deletion sequence (e.g., delete 2 characters):
  ```vim
  :norm xx
  ```

---

### Search and Replace Operations

#### Replace String within the Current Line
* Replace all occurrences of a string in the active line:
  ```vim
  :s/old/new/g
  ```
  *(Append `c` to prompt a confirmation dialogue `y/n` for each match: `:s/old/new/gc`)*
* Replace only the first occurrence of a string in the active line:
  ```vim
  :s/old/new/
  ```

#### Replace String across the Entire File
* Replace every matching instance globally throughout the open document:
  ```vim
  :%s/old/new/g
  ```

#### Replace within a Targeted Selection (Visual Mode)
1. Highlight your specific string block using `v`.
2. Automatically target your selection bounds and trigger the replacement command:
   ```vim
   :s/old/new/g
   ```

> **Pro Tip:** Append `i` to the flag sequence to make your search-and-replace queries completely **case-insensitive** (ignores uppercase/lowercase variations).
> 
> *Example (Replacing Newlines with literal `\n` placeholders):*
> ```vim
> :%s/\n/ \\n /g
> ```