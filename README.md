# gsh
A really cool and goated frfr shell

## Features
- **Autosuggestions**
- **Tab completion**
- **Job control** 
- **Customizable prompts** 
- **Command history** 
- **Aliases support** 
- **Pipeline support** 
- **Built-in commands** 
- **Configuration files** 
- **Scripting support**

## Installation

### Arch Linux (AUR)
```bash
# Using yay
yay -S gsh

# Using paru
paru -S gsh
```

### Manual Installation

#### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential libreadline-dev

# Fedora/RHEL
sudo dnf install gcc-c++ readline-devel

# Arch Linux
sudo pacman -S base-devel readline
```

#### Build from Source
```bash
git clone https://github.com/yourusername/goonsh.git
cd goonsh
g++ -std=c++17 -Wall -g *.cpp -lreadline -o gsh
sudo mv gsh /usr/local/bin/
```

## Start -

1. **Start gsh:**
   ```bash
   gsh
   ```

2. **Autosuggestions thingi (kinda weird):**
   - Type part of a previous command
   - See gray suggestions appear
   - Press → (right arrow) to accept

3. **Tab Completion:**
   ```bash
   ls ~/Doc<TAB>        # Completes to ~/Documents/
   git che<TAB>         # Completes to git checkout
   ```

4. **Run a script:**
   ```bash
   gsh myscript.sh
   ```

## Configuration

### Configuration File: `~/.gshrc`
```bash
# Set custom prompt
prompt="\[\033[36m\]\u@\h\[\033[0m\]:\[\033[32m\]\w\[\033[0m\]$ "

# Define aliases
alias ll="ls -la"
alias grep="grep --color=auto"
alias ..="cd .."

# Custom commands (run on startup)
echo "Welcome to your customized gsh!"
```

### Built-in Commands
- `cd` - Change directory
- `ls` - List directory contents  
- `pwd` - Print working directory
- `echo` - Display text
- `cat` - Display file contents
- `touch` - Create/update files
- `rm` - Remove files
- `mkdir` - Create directories
- `cp`, `mv` - Copy and move files
- `grep`, `head`, `tail`, `wc` - Text processing
- `history` - Command history
- `alias`, `unalias` - Manage aliases
- `jobs`, `fg`, `bg` - Job control
- `help` - Show available commands
- `exit`, `quit` - Exit shell

## Usage Examples

### Basic Commands
```bash
gsh> ls -la
gsh> cd ~/Documents
gsh> echo "Hello, Zain!"
```

### Pipelines and Redirection
```bash
gsh> ls -la | grep ".txt" > text_files.log
gsh> cat file.txt | head -10 | tail -5
gsh> sort names.txt >> sorted_names.txt
```

### Here Documents
```bash
gsh> cat << EOF
> This is a thingi
> very cool
> EOF
```

### Job Control
```bash
gsh> long_running_command &    # Run in background
gsh> jobs                      # List background jobs
gsh> fg                        # Bring to foreground
```

### Aliases
```bash
gsh> alias ll="ls -la"
gsh> ll                        # Runs 'ls -la'
gsh> unalias ll               # Remove alias
```

## Advanced Features

### Custom Prompt Variables
- `\u` - Username
- `\h` - Hostname  
- `\w` - Current working directory
- `\$` - $ for regular user, # for root

### Environment Variables
```bash
gsh> export MY_VAR="value"
gsh> echo $MY_VAR
gsh> env                       # Show all variables
```

### History Features
- Persistent command history
- History-based autosuggestions
- Search through history with arrow keys

## Troubleshooting

### Common Issues

**Command not found:**
```bash
# Make sure gsh is in your PATH
which gsh
```

**Autosuggestions not working:**
- Ensure you have command history (`~/.gsh_history`)
- Try typing commands to build up history

**Tab completion issues:**
- Verify readline library is properly linked
- Check file permissions in current directory

**Build errors:**
```bash
# Missing readline development headers
sudo apt install libreadline-dev  # Ubuntu/Debian
sudo dnf install readline-devel    # Fedora/RHEL
```

### Development Setup
```bash
git clone https://github.com/yourusername/goonsh.git
cd goonsh
g++ -std=c++17 -Wall -g -DDEBUG *.cpp -lreadline -o gsh
./gsh
```


This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

ty for reading, youre very cool :3
