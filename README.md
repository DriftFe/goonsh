# GoonSH/GSH
A really cool and goated frfr shell

## Features
- **Autosuggestions** - Smart command completion from history
- **Tab completion** - Commands, files, and directories
- **Job control** - Background processes and process management
- **Customizable prompts** - Configure your shell appearance
- **Command history** - Persistent history across sessions
- **Aliases support** - Create shortcuts for frequently used commands
- **Pipeline support** - Full pipe, redirection, and here-document support
- **Built-in commands** - Essential shell utilities included
- **Configuration files** - Customize behavior with `.goonshrc`
- **Scripting support** - Run shell scripts with `goonsh script.sh`

## Installation

### Arch Linux (AUR)
```bash
# Using yay
yay -S goonsh

# Using paru
paru -S goonsh
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

#### Build from Source
```bash
git clone https://github.com/yourusername/goonsh.git
cd goonsh
g++ -std=c++17 -Wall -g *.cpp -lreadline -o goonsh
sudo mv goonsh /usr/local/bin/
```

## Start -

1. **Start Goonsh:**
   ```bash
   goonsh
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
   goonsh myscript.sh
   ```

## Configuration

### Configuration File: `~/.goonshrc`
```bash
# Set custom prompt
prompt="\[\033[36m\]\u@\h\[\033[0m\]:\[\033[32m\]\w\[\033[0m\]$ "

# Define aliases
alias ll="ls -la"
alias grep="grep --color=auto"
alias ..="cd .."

# Custom commands (run on startup)
echo "Welcome to your customized Goonsh!"
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
goonsh> ls -la
goonsh> cd ~/Documents
goonsh> echo "Hello, Zain!"
```

### Pipelines and Redirection
```bash
goonsh> ls -la | grep ".txt" > text_files.log
goonsh> cat file.txt | head -10 | tail -5
goonsh> sort names.txt >> sorted_names.txt
```

### Here Documents
```bash
goonsh> cat << EOF
> This is a thingi
> very cool
> EOF
```

### Job Control
```bash
goonsh> long_running_command &    # Run in background
goonsh> jobs                      # List background jobs
goonsh> fg                        # Bring to foreground
```

### Aliases
```bash
goonsh> alias ll="ls -la"
goonsh> ll                        # Runs 'ls -la'
goonsh> unalias ll               # Remove alias
```

## Advanced Features

### Custom Prompt Variables
- `\u` - Username
- `\h` - Hostname  
- `\w` - Current working directory
- `\$` - $ for regular user, # for root

### Environment Variables
```bash
goonsh> export MY_VAR="value"
goonsh> echo $MY_VAR
goonsh> env                       # Show all variables
```

### History Features
- Persistent command history
- History-based autosuggestions
- Search through history with arrow keys

## Troubleshooting

### Common Issues

**Command not found:**
```bash
# Make sure goonsh is in your PATH
which goonsh
```

**Autosuggestions not working:**
- Ensure you have command history (`~/.goonsh_history`)
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
g++ -std=c++17 -Wall -g -DDEBUG *.cpp -lreadline -o goonsh
./goonsh
```

## 📋 Roadmap

- [ ] Syntax highlighting
- [ ] Command validation
- [ ] Plugin system
- [ ] Themes support
- [ ] Better error handling
- [ ] Performance optimizations
- [ ] Windows support

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

ty for reading, youre very cool :3
