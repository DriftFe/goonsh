# dgsh ♡
a really cool and goated frfr shell that i made >~< it's got autosuggestions, tab completion, job control and a bunch of other stuff!! hope u like it hehe

## features ♡
- **autosuggestions** - it literally reads ur mind
- **tab completion** - less typing more vibing
- **job control** - multitasking swag
- **customizable prompts** - make it cute!!
- **command history** - it remembers everything (unlike me)
- **aliases support** - shortcut everything
- **pipeline support** - chain commands like a pro
- **built-in commands** - batteries included
- **configuration files** - tweak it till it feels like urs
- **scripting support** - automate the boring stuff

## installation ♡
### arch linux (aur)
obviously the best way >:3
```bash
# using yay
yay -S dgsh
# using paru
paru -S dgsh
```
### manual installation
if u wanna do it the hard way, which is valid!!
#### prerequisites
```bash
# ubuntu/debian
sudo apt update
sudo apt install build-essential libreadline-dev
# fedora/rhel
sudo dnf install gcc-c++ readline-devel
# arch linux
sudo pacman -S base-devel readline
```
#### build from source
```bash
git clone https://github.com/yourusername/goonsh.git
cd goonsh
g++ -std=c++17 -Wall -g *.cpp -lreadline -o dgsh
sudo mv dgsh /usr/local/bin/
```

## getting started ♡
1. **launch dgsh!!**
   ```bash
   dgsh
   ```
2. **autosuggestions thingi (kinda weird but i promise it's cool):**
   - type part of a previous command
   - see gray suggestions appear
   - press → to accept!! so satisfying
3. **tab completion:**
   ```bash
   ls ~/Doc<TAB>        # completes to ~/Documents/
   git che<TAB>         # completes to git checkout
   ```
4. **run a script:**
   ```bash
   dgsh myscript.sh
   ```

## configuration ♡
### config file: `~/.dgshrc`
```bash
# set custom prompt (make it cute!!)
prompt="\[\033[36m\]\u@\h\[\033[0m\]:\[\033[32m\]\w\[\033[0m\]$ "
# define aliases
alias ll="ls -la"
alias grep="grep --color=auto"
alias ..="cd .."
# custom commands (run on startup)
echo "welcome to your customized dgsh!!"
```
### built-in commands
- `cd` - change directory
- `ls` - list directory contents
- `pwd` - print working directory
- `echo` - display text
- `cat` - display file contents
- `touch` - create/update files
- `rm` - remove files (be careful omg)
- `mkdir` - create directories
- `cp`, `mv` - copy and move files
- `grep`, `head`, `tail`, `wc` - text processing
- `history` - command history
- `alias`, `unalias` - manage aliases
- `jobs`, `fg`, `bg` - job control
- `help` - show available commands
- `exit`, `quit` - leave dgsh (nooo)

## usage examples ♡
### basic commands
```bash
dgsh> ls -la
dgsh> cd ~/Documents
dgsh> echo "hello, zain!!"
```
### pipelines and redirection
```bash
dgsh> ls -la | grep ".txt" > text_files.log
dgsh> cat file.txt | head -10 | tail -5
dgsh> sort names.txt >> sorted_names.txt
```
### here documents
```bash
dgsh> cat << EOF
> this is a thingi
> very cool
> EOF
```
### job control
```bash
dgsh> long_running_command &    # run in background
dgsh> jobs                      # list background jobs
dgsh> fg                        # bring to foreground
```
### aliases
```bash
dgsh> alias ll="ls -la"
dgsh> ll                        # runs 'ls -la'
dgsh> unalias ll                # remove alias
```

## advanced stuff ♡
### custom prompt variables
- `\u` - username
- `\h` - hostname
- `\w` - current working directory
- `\$` - $ for regular user, # for root
### environment variables
```bash
dgsh> export MY_VAR="value"
dgsh> echo $MY_VAR
dgsh> env                       # show all variables
```
### history features
- persistent command history
- history-based autosuggestions
- search through history with arrow keys

## troubleshooting ♡
### common issues
**command not found:**
```bash
# make sure dgsh is in ur PATH!!
which dgsh
```
**autosuggestions not working:**
- make sure u have command history (`~/.dgsh_history`)
- just type some commands to build up history first >_<

**tab completion issues:**
- verify readline library is properly linked
- check file permissions in current directory

**build errors:**
```bash
# probably just missing readline headers!!
sudo apt install libreadline-dev  # ubuntu/debian
sudo dnf install readline-devel   # fedora/rhel
```
### dev setup
```bash
git clone https://github.com/yourusername/goonsh.git
cd goonsh
g++ -std=c++17 -Wall -g -DDEBUG *.cpp -lreadline -o dgsh
./dgsh
```

---
licensed under the MIT license - see the [LICENSE](LICENSE) file for details!!

ty for reading, ur very cool :3 ♡
