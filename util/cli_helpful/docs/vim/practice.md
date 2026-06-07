### PRACTICE VIM EDITOR

## vimtutor

```txt
vim -W filename
```

```txt
x	: remove char at cursor

--- insert mode ---
a	: append at cursor
A	: append at last line
s	: insert replace at cursor
p	: past
R	: same s

o/O	: open at below/above of the cursor

--- operator ---
d + motion
ex: d$	-> delete to from cursor to last line.

--- movement ---
w	: to start next word.
b	: to back before word.
e	: to end of cursor word.
$	: to end of the line.
0	: to start of the line.

2w	: move to start of 2 next word
3e	: move to end of 2 next word

U	: recording for undo/redo

-line jump-
:+2	: jump to next 2 line
:-4	: jump to before 4 line

--- put command ---
y	: coppy (yw,y$)
p	: past

--- replace command ---
r	: replace charactor

ce	: insert and replace mode
cc	: replace all line

c [number] movement

--- movement ---

ctrl+g	: to view file info

gg	: to start file
G	: to end the page

--- search command ---

:/<key>	: search key (direction)
:?<key>	: search key (back direction)

n	: next search
N	: back search

-matching search-				HELPFUL

===Jump to in scope===
%	: with "{,[,(" jump to "},],)"

--- substitue command ---

:s/old/new/g	: replace old by new

replace from line start to line end.
can use "." for current line.
:#/#s/old/new/g	: replace "#" by line number (from-to)

:%s/old/new/gc	: replace all file - c: confirm

--- command line called ---

:!<COMMAND>					HELPFUL

--- selecting text to write ---

step:
v 		: select line-to-line
:		: appear :'<,'>
w		: save file
w TEST		: write temp to TEST file

--- retrieving and merge files ---

:r TEST		: to coppy context 
		of the TEST file 
		and past here.

:set number
:set nonumber

--- find string option ---
 
:set ic		: ignore upper/lower case
		when searching.
:set is		: show partical matches
		for search phrase.
:set hls	: highlight all
		matching phrases.

--- help command ---
:help CMD
:help CTRL-W

```

