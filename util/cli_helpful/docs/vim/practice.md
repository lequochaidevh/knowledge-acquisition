a/A: is append at cursor/endline.
r/R: replace/insert at cursor. s = r.
o/O: below/above at cursor.
d/y/p: cut/copy/past -> dd/yy apply for all of a line.

w/b/e: next/back/end word.
0/$ : start/end line.  (/) : start/stop paragraph
gg/G : start/end file
Number Motion: 2w, 3e or :+2 / :-2 :jump next/back to 2 lines.

:/KEY / :?KEY: Forward/Backard searching.
	:set ic : ignore Upper/Lower case.
	:set is : show partical matches.
	:set hls : highlight.

%: Jump from "{ [ (" to "} ] )"
:% : all file.

Replace:
ce = x + i  |  cc = dd + i  |  c + Number + Motion
:s/OLD/NEW/g : replace.
: FromLine,ToLine s/OLD/NEW/g : replace. ("." Present line )
:%s/OLD/NEW/gc : replace all.
:s/^/\t/ : Insert multi line.

:!CMD

File Content Operation.
:w FileName : Write to file.
:r FileName : Copy from file.
