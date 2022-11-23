#!/usr/bin/python3
import aspell
import sys
import getopt

def deb(s):
    print(s, file=sys.stderr)
    
def Usage(f=sys.stderr):
    print("Usage: rclaspell-sugg.py --lang=lang --encoding=encoding "
          "--master=pathtodict --sug-mode=mode pipe", file=f)
    sys.exit(1)
    
options=[]
opts, args = getopt.getopt(sys.argv[1:], "h",
                           ["help", "lang=", "encoding=", "master=", "sug-mode=", "mode="])
for opt, arg in opts:
    if opt in ['-h', '--help']:
        Usage(sys.stdout)
    elif opt in ['--lang']:
        options.append(("lang", arg))
    elif opt in ['--encoding']:
        options.append(("encoding", arg))
    elif opt in ['--master']:
        options.append(("master", arg))
    elif opt in ['--sug-mode']:
        options.append(("sug-mode", arg))
    elif opt in ['--mode']:
        options.append(("mode", arg))
    else:
        print(f"Unknown option {opt}", file=sys.stderr)
        Usage()

if len(args) != 1 or args[0] != "pipe":
    Usage()

#[("lang","en"), ("encoding","utf-8"), ("master","/home/dockes/.recoll-prod/aspdict.en.rws"),
# ("sug-mode", "fast")]

speller = aspell.Speller(*options)

print("@(#) International Ispell Version 3.1.20 (but really rclaspell-sugg 0.60.8)")
sys.stdout.flush()

while True:
    line = sys.stdin.readline()
    if not line:
        break
    term = line.strip(" \t\n\r")
    words = speller.suggest(term)
    if words:
        print(f"& {term} {len(words)} 0: " + ", ".join(words))
    else:
        print("*")
    print()
    sys.stdout.flush()
