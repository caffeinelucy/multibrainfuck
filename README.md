# multibrainfuck
you've heard of [brainfuck?](https://en.wikipedia.org/wiki/Brainfuck)

welcome to multibrainfuck! multibrainfack supports all brainfuck instructions, plus additional useful commands and MULTI-THREADING!

# multibrainfuck commands
```
/ this is a a comment /
/ comments can stretch
across multiple lines
in fact, the interpreter ignores all unspecified characters! /
```

```
+ / inc current cell /
```
```
- / dec current cell /
```
```
> / move pointer to right cell /
```
```
< / move pointer to left cell /
```
```
^ / like + but thread-safe /
```
```
v / like - but thread-safe /
```
```
. / print cell as ascii character /
```
```
, / read from stdin into cell /
```
```
[ / if cell is 0, go past matching ] /
```
```
] / go back to matching [ /
```
```
b / print linebreak /
```
```
: / print cell as number /
```
```
z / yield current thread /
```
```
! / print all cells, up to the last non-zero cell /
```
```
"hello, world" / multibrainfuck can put string literals into cells, starting from current cell moving to the last. notice that strings in multibrainfuck are DOUBLE-NULL_TERMINATED!! having a null-terminator on both ends makes it easier to use in brainfuck. trust me /
```
```
$12$ / the same way, number literals can be used /
```
```
? / wait until current cell is 0. you can de-facto use any cell like a mutex. look at the example, it's very intuitive /
```
```
{ / start new thread. the calling thread will jump past the matching } /
```
```
} / return from current thread /
```

# compile and run
only tested on linux. in fact, it uses pthreads and linux specific code. 
```
gcc multibrainfuck.c -lpthread -o multibrainfuck
```
then run like so:
```
./multibrainfuck test.mbf
```

enjoy! <3
