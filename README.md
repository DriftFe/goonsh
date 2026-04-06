# dgsh
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
yay -S dgsh

# Using paru
paru -S dgsh
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
g++ -std=c++17 -Wall -g *.cpp -lreadline -o dgsh
sudo mv dgsh /usr/local/bin/
```

## Start -

1. **Start dgsh:**
   ```bash
   dgsh
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
   dgsh myscript.sh
   ```

## Configuration

### Configuration File: `~/.dgshrc`
```bash
# Set custom prompt
prompt="\[\033[36m\]\u@\h\[\033[0m\]:\[\033[32m\]\w\[\033[0m\]$ "

# Define aliases
alias ll="ls -la"
alias grep="grep --color=auto"
alias ..="cd .."

# Custom commands (run on startup)
echo "Welcome to your customized dgsh!"
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
dgsh> ls -la
dgsh> cd ~/Documents
dgsh> echo "Hello, Zain!"
```

### Pipelines and Redirection
```bash
dgsh> ls -la | grep ".txt" > text_files.log
dgsh> cat file.txt | head -10 | tail -5
dgsh> sort names.txt >> sorted_names.txt
```

### Here Documents
```bash
dgsh> cat << EOF
> This is a thingi
> very cool
> EOF
```

### Job Control
```bash
dgsh> long_running_command &    # Run in background
dgsh> jobs                      # List background jobs
dgsh> fg                        # Bring to foreground
```

### Aliases
```bash
dgsh> alias ll="ls -la"
dgsh> ll                        # Runs 'ls -la'
dgsh> unalias ll               # Remove alias
```

## Advanced Features

### Custom Prompt Variables
- `\u` - Username
- `\h` - Hostname  
- `\w` - Current working directory
- `\$` - $ for regular user, # for root

### Environment Variables
```bash
dgsh> export MY_VAR="value"
dgsh> echo $MY_VAR
dgsh> env                       # Show all variables
```

### History Features
- Persistent command history
- History-based autosuggestions
- Search through history with arrow keys

## Troubleshooting

### Common Issues

**Command not found:**
```bash
# Make sure dgsh is in your PATH
which dgsh
```

**Autosuggestions not working:**
- Ensure you have command history (`~/.dgsh_history`)
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
g++ -std=c++17 -Wall -g -DDEBUG *.cpp -lreadline -o dgsh
./dgsh
```


This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

ty for reading, youre very cool :3
